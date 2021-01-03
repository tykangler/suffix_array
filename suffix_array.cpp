#include "suffix_array.hpp"

#include <cmath>

/* -------------------- \
|      bucket_array     |
\ -------------------- */

void suffix_array::bucket_array::reset_heads() {
   int offset = 1;
   for (int i = 0; i < 95; i++) {
      std::get<bucket::head>(buckets[i]) = offset;
      offset += std::get<bucket::size>(buckets[i]);
   }
}

void suffix_array::bucket_array::reset_tails() {
   int offset = 1;
   for (int i = 0; i < 95; i++) {
      offset += std::get<bucket::size>(buckets[i]);
      std::get<bucket::tail>(buckets[i]) = offset - 1;
   }
}

/* -------------------- \
|      LMS helpers      |
\ -------------------- */

std::vector<char> suffix_array::map_char_types(const std::vector<int> &string) {
   std::vector<char> char_types;
   char_types.resize(string.size() + 1);
   char_types[string.size()] = s_type;
   char_types[string.size() - 1] = l_type;
   for (int i = string.size() - 2; i >= 0; i--) {
      if (string[i] == string[i + 1]) {
         char_types[i] = char_types[i + 1];
      } else if (string[i] > string[i + 1]) {
         char_types[i] = l_type;
      } else {
         char_types[i] = s_type;
      }
   }
   return char_types;
}

bool suffix_array::lms_strings_equal(int i, int j, 
                                     const std::vector<int> &string, 
                                     const std::vector<char> &char_types) {
   if (i == string.size() || j == string.size()) {
      return false;
   } else if (lms_char(char_types, i) && lms_char(char_types, j)) {
      while (!lms_char(char_types, ++i) && !lms_char(char_types, ++j)) {
         if (string[i] != string[j]) {
            return false;
         }
      }
   }
   return lms_char(char_types, i) && lms_char(char_types, j);
}

/* --------------------------------- \
|      suffix array construction     |
\ --------------------------------- */

std::vector<int> suffix_array::construct(const std::vector<int> &string) {
   bucket_array buckets(string.begin(), string.end());
   auto char_types = map_char_types(string);

   auto guessed_sa = guess_lms(string, char_types, buckets);
   induce_sort_ltype(string, guessed_sa, char_types, buckets);
   induce_sort_stype(string, guessed_sa, char_types, buckets);
   auto [sum_string, alpha_size, sum_string_offsets] = summarize(string, guessed_sa, char_types);
   auto sum_sa = make_summary_suffix_array(sum_string, alpha_size);
   auto final_sa = accurate_lms_sort(string, sum_sa, sum_string_offsets, char_types, buckets);
   induce_sort_ltype(string, final_sa, char_types, buckets);
   induce_sort_stype(string, final_sa, char_types, buckets);

   return final_sa;
}

std::vector<int> suffix_array::guess_lms(const std::vector<int> &string, 
                                         const std::vector<char> &char_types, 
                                         bucket_array &buckets) {
   std::vector<int> guessed_sa;
   guessed_sa.resize(string.size() + 1, -1);
   guessed_sa[0] = string.size();
   for (int i = 0; i < string.size(); i++) {
      if (lms_char(char_types, i)) {
         guessed_sa[buckets.next_tail(string[i])] = i;
      }
   }
   buckets.reset_tails();
   return guessed_sa;                       
}

void suffix_array::induce_sort_ltype(const std::vector<int> &string, 
                                     std::vector<int> &sa, 
                                     const std::vector<char> &char_types, 
                                     bucket_array &buckets) {
   for (int i = 0; i < sa.size(); i++) {
      int str_index = sa[i] - 1;
      if (str_index >= 0 && char_types[str_index] == l_type) {
         sa[buckets.next_head(string[i])] = str_index;
      }
   }
   buckets.reset_heads();
}

void suffix_array::induce_sort_stype(const std::vector<int> &string,
                                     std::vector<int> &sa,
                                     const std::vector<char> &char_types, 
                                     bucket_array &buckets) {
   for (int i = sa.size() - 1; i >= 0; i--) {
      int str_index = sa[i] - 1;
      if (str_index >= 0 && char_types[str_index] == s_type) {
         sa[buckets.next_tail(string[i])] = str_index;
      }
   }
   buckets.reset_tails();
}

auto suffix_array::summarize(const std::vector<int> &string,
                             const std::vector<int> &guessed_sa,
                             const std::vector<char> &char_types) 
-> std::tuple<std::vector<int>, int, std::vector<int>> {

   std::vector<int> sum_string_temp;
   sum_string_temp.resize(guessed_sa.size() + 1, -1);
   int name = 0;
   int last = guessed_sa[0];
   sum_string_temp[last] = name;
   for (int i = 1; i < guessed_sa.size(); i++) {
      int curr = guessed_sa[i];
      if (!lms_strings_equal(last, curr, string, char_types)) {
         name++;
      }
      sum_string_temp[curr] = name;
      last = curr;
   }

   std::vector<int> sum_string_offsets;
   std::vector<int> sum_string;
   for (int i = 0; i < sum_string_temp.size(); i++) {
      if (sum_string_temp[i] != -1) {
         sum_string_offsets.push_back(i);
         sum_string.push_back(sum_string_temp[i]);
      }
   }
   return std::make_tuple(sum_string, name + 1, sum_string_offsets);
}

std::vector<int> suffix_array::make_summary_suffix_array(const std::vector<int> &sum_string,
                                                         const int alpha_size) {
   std::vector<int> sum_sa;
   if (alpha_size < sum_string.size()) {
      sum_sa = construct(sum_string);
   } else {
      sum_sa.resize(sum_string.size() + 1, -1);
      sum_sa[0] = sum_string.size();
      for (int i = 0; i < sum_string.size(); i++) {
         sum_sa[sum_string[i] + 1] = i;
      }
   }
   return sum_sa;
}

std::vector<int> suffix_array::accurate_lms_sort(const std::vector<int> &string,
                                                 const std::vector<int> &sum_sa, 
                                                 const std::vector<int> &sum_sa_offsets,
                                                 const std::vector<char> &char_types, 
                                                 bucket_array &buckets) {
   std::vector<int> suffix_offsets;
   suffix_offsets.resize(string.size() + 1, -1);
   suffix_offsets[0] = string.size();
   for (int i = string.size() - 1; i > 0; i++) {
      int str_index = sum_sa_offsets[sum_sa[i]];
      suffix_offsets[buckets.next_tail(string[str_index])] = str_index;
   }
   buckets.reset_tails();
   return suffix_offsets;
}

/* ------------------------------ \
|      suffix array functions     |
\ ------------------------------ */

namespace {

template<typename First, typename Second>
int common_prefix(First *left, Second *right, const int num) {
   static_assert(std::is_integral_v<First> && std::is_integral_v<Second>);
   int num_viewed = 0;
   while (*left == *right && num_viewed < num) {
      left++, right++;
      num_viewed++;
   }
   return num_viewed;
}

template<typename First, typename Second>
int comp(const First *left, const Second *right, const int num) {
   static_assert(std::is_integral_v<First> && std::is_integral_v<Second>);
   int i = 0;
   while (i < num && left[i] != right[i]) {
      i++;
   }
   return left[i] - right[i];
}

} // unnamed
