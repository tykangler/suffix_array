#ifndef __ENIGMA_SUFFIX_ARRAYHPP__
#define __ENIGMA_SUFFIX_ARRAYHPP__

#include <vector>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <algorithm>

class lcp_lr_array {
   public:
      lcp_lr_array() {}
      lcp_lr_array(const lcp_lr_array&) =default;
      lcp_lr_array(lcp_lr_array&&) =default;
      lcp_lr_array &operator=(const lcp_lr_array&) =default;
      lcp_lr_array &operator=(lcp_lr_array&&) =default;
   private:
		std::vector<int> lcps;
};

/** 
 * sorted array of all suffixes in a string. 
*/
class suffix_array {
	friend class lcp_lr_array;
   static const char l_type = 'l';
   static const char s_type = 's';
   public:
      suffix_array() {}
      template<typename InputIt, typename Pred>
      suffix_array(InputIt first, InputIt last, Pred condition);
      suffix_array(const suffix_array&) =default;
      suffix_array(suffix_array&&) =default;
      suffix_array &operator=(const suffix_array&) =default;
      suffix_array &operator=(suffix_array&&) =default;

      using iterator = std::unordered_set<std::string>::iterator;
      using const_iterator = std::unordered_set<std::string>::const_iterator;

      const_iterator find(const unsigned char *substr, const int size);
      const_iterator begin() const noexcept { return words_found.begin(); }
      const_iterator end() const noexcept { return words_found.end(); }

   private:

      class bucket_array {
         static const int min_char = 32;
         static const int max_char = 126;
         enum bucket { head, size, tail };
         public:
            template<typename InputIt>
            bucket_array(InputIt first, InputIt last);
            bucket_array(const bucket_array&) =default;
            bucket_array(bucket_array&&) =default;
            bucket_array &operator=(const bucket_array&) =default;
            bucket_array &operator=(bucket_array&&) =default;

            void reset_heads();
            void reset_tails();

            int next_head(unsigned char ch) { 
               return std::get<bucket::head>(buckets[ch - min_char])++; 
            }

            int next_tail(unsigned char ch) { 
               return std::get<bucket::tail>(buckets[ch - min_char])--; 
            }
         private:
            // consider characters [32, 127)
            std::tuple<int, int, int> buckets[max_char - min_char + 1]; 
      };

      /** 
       * pre: words are composed of unsigned characters
       * 
       * inserts words from [first, last) into 'words,' character by character. 
      */
      template<typename InputIt, typename Pred>
      void insert_words(InputIt first, InputIt last, Pred condition);

      /** 
       * Constructs and returns the suffix array for the given string
      */
      std::vector<int> construct(const std::vector<int> &string);

		/** 
		 * returns true if the character at the given index of 'char_types' is a
		 * LMS character. returns false otherwise.
		*/
      bool lms_char(const std::vector<char> &char_types, int index) { 
         return index != 0 && 
                char_types[index] == s_type && 
                char_types[index - 1] == l_type;
      }

		/** 
		 * returns true if the substrings at index 'i' and 'j' of 'string' start with
		 * a LMS character, and are equal until the next LMS character
		*/
      bool lms_strings_equal(int i, int j, 
                             const std::vector<int> &string, 
                             const std::vector<char> &char_types);

		/** 
		 * returns a list mapping each string index to a character type (l-type or s-type)
		*/
      std::vector<char> map_char_types(const std::vector<int> &string);
      
		/** 
		 * returns an approximately correct suffix array for the given string
		*/
      std::vector<int> guess_lms(const std::vector<int> &string, 
                                 const std::vector<char> &char_types,
                                 bucket_array &buckets);

		/** 
		 * sorts l-type characters in 'string' into the approximate positions in 'guessed_sa'
		*/
      void induce_sort_ltype(const std::vector<int> &string, 
                             std::vector<int> &guessed_sa, 
                             const std::vector<char> &char_types, 
                             bucket_array &buckets);

		/** 
		 * sorts remaining s-type characters in 'string' into the approximate positions 
		 * in 'guessed_sa'
		*/
      void induce_sort_stype(const std::vector<int> &string,
                             std::vector<int> &guessed_sa,
                             const std::vector<char> &char_types, 
                             bucket_array &buckets);

		/** 
		 * summarizes the approximate suffix array into a string representing the rank
		 * of each LMS substring found in the suffix array. returns the summary string, the
		 * alphabet size of the summary string, and the positions in 'string' that correspond
		 * to each entry in the summary string
		*/
      auto summarize(const std::vector<int> &string,
                     const std::vector<int> &guessed_sa,
                     const std::vector<char> &char_types) 
      -> std::tuple<std::vector<int>, int, std::vector<int>>;

		/** 
		 * 
		*/
      std::vector<int> make_summary_suffix_array(const std::vector<int> &sum_string,
                                                 const int alpha_size);

		/** 
		 * 
		*/
      std::vector<int> accurate_lms_sort(const std::vector<int> &string,
                                         const std::vector<int> &sum_sa, 
                                         const std::vector<int> &sum_sa_offsets,
                                         const std::vector<char> &char_types, 
                                         bucket_array &buckets);

		/** 
		 * 
		*/
		int binarylcp_search(const unsigned char *substr, const int size);
   private:
      std::vector<int> words;
      std::vector<std::pair<int, int>> word_maps;
      std::vector<int> suffixes;
      std::unordered_set<std::string> words_found;
		lcp_lr_array lcps;
};

template<typename InputIt>
suffix_array::bucket_array::bucket_array(InputIt first, InputIt last) {
   static_assert(std::is_integral_v<typename std::iterator_traits<InputIt>::value_type>);
   while (first != last) {
      if (*first != 0 && (*first < min_char || *first > max_char)) {
         throw std::range_error{""};
      } 
      std::get<bucket::size>(buckets[*first - min_char])++;
      first++;
   }
   reset_heads();
   reset_tails();
}

template<typename InputIt, typename Pred>
suffix_array::suffix_array(InputIt first, InputIt last, Pred condition) {
   insert_words(first, last, condition);
   suffixes = construct(words);
}

template<typename InputIt, typename Pred>
void suffix_array::insert_words(InputIt first, InputIt last, Pred condition) {
   while (first != last) {
      if (condition(*first)) {
         words.insert(words.end(), first->begin(), first->end());
         int length = first->size();
         for (int i = 0; i < length; i++) {
            word_maps.emplace_back(i, length);
         }
      }
   }
}

/** 
 * builds a suffix array with the given string iterators. Iterators must point to strings
 * of unsigned characters (enigma::string). 
*/
template<typename InputIt, typename Pred>
suffix_array make_suffix_array(InputIt first, InputIt last, Pred condition) {
   return suffix_array(first, last, condition);
}

#endif