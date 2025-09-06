// worst-wordle.hpp

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "thread-pool.hpp"

// Forward-declared classes.
class Word;
class LetterSet;
class WorstWordle;

class Word {
 public:
  Word(std::string str) {
    for (size_t i = 0; i < str.size(); ++i) {
      wordArray[i] = str[i];
    }
  }
  friend std::ostream& operator<<(std::ostream& os, const Word& w) {
    os.write(w.wordArray.data(), w.wordArray.size());
    return os;
  }
  bool operator<(const Word& other) const {
    return wordArray < other.wordArray;
  }
  const std::array<char, 5> GetWordArray() const { return wordArray; }

 private:
  std::array<char, 5> wordArray;
};

class LetterSet {
 public:
  LetterSet() {}
  LetterSet(const std::bitset<26>& bitset) : set(bitset) {}
  LetterSet(const Word& word) {
    std::array<char, 5> wordArray = word.GetWordArray();
    for (size_t i = 0; i < wordArray.size(); ++i) {
      set |= (1 << (std::tolower(wordArray[i]) - 'a'));
    }
  }
  const std::bitset<26> GetSet() const { return set; }
  // Should be run before sorting a LetterSet.
  static void SetLetterFrequency(const std::vector<LetterSet>& setList,
                                 const bool& report = false) {
    size_t total = 0;
    for (const auto& letterSet : setList) {
      const std::bitset<26> currentSet = letterSet.GetSet();
      for (size_t i = 0; i < 26; ++i) {
        if (currentSet.test(i)) {
          char c = 'a' + i;
          letterFrequency[c]++;
          total++;
        }
      }
    }
    for (const auto& [letter, count] : letterFrequency) {
      // Calculate the percentage based on total unique letter instances.
      letterFrequency[letter] = (static_cast<double>(count) / total) * 100.0;
    }

    if (report) {
      // Just for fun, report these frequencies in the output.
      std::vector<std::pair<char, double>> letterFrequencyList(
          letterFrequency.begin(), letterFrequency.end());
      sort(letterFrequencyList.begin(), letterFrequencyList.end(),
           [](const auto& a, const auto& b) {
             // Sorts from high frequency to low.
             return a.second > b.second;
           });
      for (const auto& [letter, frequency] : letterFrequencyList) {
        std::cout << letter << ": " << frequency << std::endl;
      }
    }
  }
  bool operator==(const LetterSet& other) const {
    return set == other.GetSet();
  }
  struct Hash {
    size_t operator()(const LetterSet& ls) const {
      return std::hash<std::bitset<26>>{}(ls.set);
    }
  };
  struct Compare {
    // Our goal is to prune as much of the search space as we can, as quickly as
    // possible. To do this, we should always choose the most frequent letters
    // first, so they will prune out as many of the subtree branches as early
    // possible.
    bool operator()(const LetterSet& a, const LetterSet& b) const {
// If we do not prune on vowelless words first, then we should not handle vowels
// first and aggressively prune on letter occurance instead.
#ifndef DISABLE_VOWEL_OPTIMIZATION
      // Words without vowels are mandatory in valid solutions and are chosen
      // first.
      bool vowA = a.hasVowel();
      bool vowB = b.hasVowel();
      if (vowA != vowB) {
        return !vowA;
      }
#endif
#ifndef DISABLE_RARITY_SORT
      if (!letterFrequency.empty()) {
        // Most common letters first.
        double rareA = a.rarityScore();
        double rareB = b.rarityScore();
        if (rareA != rareB) return rareA > rareB;
      } else {
        std::cout << "WARNING: No letterFrequency provided before sorting."
                  << std::endl;
      }
#endif
      // Whichever has an earlier bit unset is the lower value.
      // This enforces linear ordering.
      for (size_t i = 0; i < 26; ++i) {
        if (a.set.test(i) != b.set.test(i)) {
          return b.set.test(i);
        }
      }
      // The bitsets are equal.
      return false;
    }
  };

  // Custom printing for letter sets.
  friend std::ostream& operator<<(std::ostream& os,
                                  const LetterSet& letterSet) {
    const std::bitset<26> set = letterSet.GetSet();
    for (size_t i = 0; i < set.size(); ++i) {
      if (set.test(i)) {
        os << static_cast<char>('a' + i);
      } else {
        os << '_';
      }
    }
    return os;
  }

  LetterSet operator|(const LetterSet& other) const {
    return LetterSet(this->set | other.GetSet());
  }
  LetterSet operator&(const LetterSet& other) const {
    return LetterSet(this->set & other.GetSet());
  }

  bool hasVowel() const {
    std::bitset<26> vowels;
    vowels.set('a' - 'a');
    vowels.set('e' - 'a');
    vowels.set('i' - 'a');
    vowels.set('o' - 'a');
    vowels.set('u' - 'a');
    vowels.set('y' - 'a');
    return (set & vowels).any();
  }

  // Higher score = less rare.
  double rarityScore() const {
    double score = 0.0;
    // A bit is set if the letter is in the word.
    for (size_t i = 0; i < 26; ++i) {
      if (set.test(i)) {
        char c = 'a' + i;
        auto it = letterFrequency.find(c);
        if (it != letterFrequency.end()) {
          score += it->second;
        } else {
          // Unknown letters treated as "average rarity".
          score += 3.85;
        }
      }
    }
    return score;
  }

 private:
  std::bitset<26> set;
  static std::unordered_map<char, double> letterFrequency;
};

class WorstWordle {
 public:
  WorstWordle(const std::filesystem::path& guessList,
              const std::filesystem::path& answerList)
      : guessWords(GetWordlist(guessList)),
        ansWords(GetWordlist(answerList)),
        guessSets(ToSetList(guessWords)),
        answerSets(ToSetList(ansWords)),
        setToWordsGuess(PopulateMultimap(guessWords)),
        setToWordsAns(PopulateMultimap(ansWords)) {
    startTime = std::chrono::steady_clock::now();
    LetterSet::SetLetterFrequency(guessSets);
    std::sort(guessSets.begin(), guessSets.end(), LetterSet::Compare());
  }

  // Find and enumerate all unique Wordle solutions that result in 0 matches.
  // A solution is considered unique if one or more words (guess or answer) is
  // different than another solution.
  // A solution is only considered correct if no letters are reused across
  // words.
  void FindWorstWordle(const bool useFutures = true) {
    std::vector<LetterSet> chosenSets;
    // We will always have exactly 6 sets per guess.
    chosenSets.reserve(6);
#ifdef DISABLE_VOWEL_OPTIMIZATION
    FindWorstWordleRecursive(chosenSets, LetterSet(), guessSets, useFutures);
    return;
#endif

    // Handle the initial, vowelless words first.
    // We can safely ignore all of the words with vowels from the first level of
    // our DFS due to the following properties:
    // - There are 6 guesses and 1 answer per solution.
    // - There are 6 vowels in the alphabet.
    // - All words in the answers wordlist contain vowels.
    // - There are words in the guesses wordlist that do not contain vowels.
    // - Because of this, we know that 1 of the guesses must be a vowelless
    // word.

    // Checking every set may cause a minor increase in run time, but it is
    // safer this way.
    size_t vowellessCount = 0;
    for (size_t i = 0; i < guessSets.size(); ++i) {
      if (!guessSets[i].hasVowel()) {
        ++vowellessCount;
        const LetterSet& usedLetters = guessSets[i];
        chosenSets.push_back(usedLetters);
        const std::vector<LetterSet> pruned =
            PruneSets(usedLetters, i + 1, guessSets);
        FindWorstWordleRecursive(chosenSets, usedLetters, pruned, useFutures);
        chosenSets.pop_back();
      }
    }
#ifdef DEBUG
    std::cout << "Found " << vowellessCount << " vowelless letter sets"
              << std::endl;
    // Check to make sure all vowelless sets come first.
    size_t prefix = 0, total = 0;
    for (size_t i = 0; i < guessSets.size(); ++i) {
      if (!hasVowel(guessSets[i])) {
        ++total;
        if (prefix == i) ++prefix;  // count leading block
      }
    }
    assert(prefix == total && "Comparator must place all vowelless sets first");
#endif

    // Wait for all parallel tasks to complete.
    for (auto& f : futures) {
      f.get();
    }
  }

 private:
  const std::vector<Word> GetWordlist(const std::filesystem::path& file) {
    std::vector<Word> words;
    std::ifstream in(file);
    if (!in) {
      throw std::runtime_error("Could not open file: " + file.string());
    }
    std::string line;
    while (std::getline(in, line)) {
      if (line.size() != 5) {
        std::cout << "Found invalid line: " << line << " of length "
                  << line.size() << std::endl;
        // Skip invalid lines.
        continue;
      }
      words.push_back(Word(line));
    }
    return words;
  }

  // Convert a list of words into a deduplicated list of sets.
  std::vector<LetterSet> ToSetList(const std::vector<Word>& wordlist,
                                   const bool& report = false) {
    std::unordered_set<LetterSet, LetterSet::Hash> setSet;
    for (const Word& word : wordlist) {
      setSet.insert(LetterSet(word));
    }
    std::vector<LetterSet> setList(setSet.begin(), setSet.end());
    if (report) {
      std::cout << "Converted a " << wordlist.size() << " word list into a "
                << setList.size() << " set list." << std::endl;
    }
    return setList;
  }

  // Create a mapping from sets to full words.
  const std::unordered_multimap<LetterSet, Word, LetterSet::Hash>
  PopulateMultimap(const std::vector<Word>& wordlist) {
    std::unordered_multimap<LetterSet, Word, LetterSet::Hash> multiMap;
    for (const Word& word : wordlist) {
      multiMap.emplace(LetterSet(word), word);
    }
    return multiMap;
  }

  int64_t GetElapsedMs() {
    auto elapsed = std::chrono::steady_clock::now() - startTime;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)
        .count();
  }

  // Remove words from the guess sets that share any letter with prune.
  const std::vector<LetterSet> PruneSets(
      const LetterSet& prune, const size_t& startIndex,
      const std::vector<LetterSet>& guessSets) {
    std::vector<LetterSet> pruned;
#ifdef DISABLE_PERMUTATION_DEDUP
    // Suppress the warning for the unused parameter.
    (void)startIndex;
    const size_t offsetIndex = 0;
#else
    const size_t offsetIndex = startIndex;
#endif
#ifndef DISABLE_PRUNING
    for (size_t i = offsetIndex; i < guessSets.size(); ++i) {
      const auto& guessSet = guessSets[i].GetSet();
      const auto& pruneSet = prune.GetSet();
      bool hasOverlap = (guessSet & pruneSet).any();
      if (!hasOverlap) {
        pruned.push_back(guessSet);
      }
    }
#else
    // Suppress the warning for the unused parameter.
    (void)prune;
    std::vector<LetterSet> slice(guessSets.begin() + offsetIndex,
                                 guessSets.end());
    return slice;
#endif
    return pruned;
  }

  void PrintSolution(const Word& answer, const std::vector<Word>& guesses,
                     const size_t& solutionCount) {
    // Sorting takes longer but keeps the output well-organized.
    // std::vector<Word> sortedGuesses = guesses;
    // sort(sortedGuesses.begin(), sortedGuesses.end());

    // auto ms = GetElapsedMs();

    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << answer << ":";
    for (size_t i = 0; i < guesses.size(); ++i) {
      std::cout << guesses[i];
      if (i + 1 != guesses.size()) {
        std::cout << ",";
      }
    }
    // std::cout << "\n";

    // NOTE: Not reporting this info makes the printing process faster.
    (void)solutionCount;
    // std::cout << "Elapsed time: " << ms << " ms" << "\n";
    // std::cout << "Found " << ++solutions << " solutions" << "\n";
    // std::cout << "Local solution count: " << solutionCount << "\n";
    std::cout << std::endl;
  }

  void FindWorstWordsRec(const std::vector<Word>& validAnswers,
                         const std::vector<std::vector<Word>>& combinations,
                         const size_t& idx, std::vector<Word>& solution) {
    // A full solution has been found.
    if (idx >= combinations.size()) {
#ifndef NO_PRINT
      size_t solutionCount = 0;
      // Report all matching answers.
      for (const auto& answer : validAnswers) {
        PrintSolution(answer, solution, ++solutionCount);
      }
#endif
      return;
    }
    const auto& wordList = combinations[idx];
    for (const auto& word : wordList) {
      solution.push_back(word);
      FindWorstWordsRec(validAnswers, combinations, idx + 1, solution);
      solution.pop_back();
    }
  }

  // Reconstitute real words from the letter sets to report full solutions.
  void FindWorstWords(const LetterSet& usedLetters,
                      const std::vector<LetterSet>& chosenSets) {
    std::bitset<26> usedLettersSet = usedLetters.GetSet();
    // Get all valid answers.
    std::vector<Word> validAnswers;
    for (const auto& answerSet : answerSets) {
      // If the answer set overlaps with the letters used in the guess set, then
      // the answer is not valid and should be skipped.
      if ((answerSet & usedLettersSet).GetSet().any()) {
        continue;
      }
      // Get all of the answers associated with this letter set.
      // TODO: This access somehow isn't thread-safe. This issue will appear
      // when using multithreading but disabling the vowel optimization.
      auto range = setToWordsAns.equal_range(answerSet);
      // Check if the range is valid (i.e., the key exists).
      if (range.first != range.second) {
        for (auto it = range.first; it != range.second; ++it) {
          validAnswers.push_back(it->second);
        }
      } else {
        std::cout << "WARNING:" << answerSet
                  << " is in the answerSets but not in the setToWordsAns."
                  << std::endl;
      }
    }

    // Get all combinations of valid words.
    std::vector<std::vector<Word>> combinations;
    combinations.reserve(chosenSets.size());
    for (size_t i = 0; i < chosenSets.size(); ++i) {
      combinations.push_back({});
      const auto& chosenSet = chosenSets[i];
      // TODO: This access somehow isn't thread-safe. This issue will appear
      // when using multithreading but disabling the vowel optimization.
      auto range = setToWordsGuess.equal_range(chosenSet);
      if (range.first != range.second) {
        for (auto it = range.first; it != range.second; ++it) {
          combinations.back().push_back(it->second);
        }
      } else {
        std::cout << "WARNING:" << chosenSet
                  << " is in the chosenSets but not in the setToWordsGuess."
                  << std::endl;
      }
    }

    // Now provide all of the valid combinations as answers.
    std::vector<Word> solution;
    FindWorstWordsRec(validAnswers, combinations, 0, solution);
  }

  // Recursive helper function to find combinations
  void FindWorstWordleRecursive(std::vector<LetterSet>& chosenSets,
                                const LetterSet& usedLetters,
                                const std::vector<LetterSet>& guessSets,
                                const bool& useFutures = false) {
    // Base case: if we have 6 sets, score them.
    if (chosenSets.size() >= 6) {
      // This recursive call converts our sets into concrete word solutions.
      FindWorstWords(usedLetters, chosenSets);
      // NOTE: If we don't return, we can find solutions with more than 6
      // guesses.
      return;
    }

    for (size_t i = 0; i < guessSets.size(); ++i) {
      const auto& set = guessSets[i];
#ifdef DISABLE_PRUNING
      // Check for overlap.
      // Needed to avoid enumerating every single solution (even imperfect
      // ones).
      bool hasOverlap = (set & usedLetters).GetSet().any();
      if (hasOverlap) {
        continue;
      }
#endif
      const std::vector<LetterSet> pruned = PruneSets(set, i + 1, guessSets);
      chosenSets.push_back(set);
      LetterSet newUsedLetters = (usedLetters | set);
      if (useFutures) {
        // Run the next level of the DFS in parallel.
        futures.push_back(pool.enqueue(
            std::bind(&WorstWordle::FindWorstWordleRecursiveConst, this,
                      chosenSets, newUsedLetters, pruned, false)));
      } else {
        FindWorstWordleRecursive(chosenSets, newUsedLetters, pruned);
      }
      chosenSets.pop_back();
    }
  }

  // Needed to make multithreaded calls to FindWorstWordleRecursive correct.
  void FindWorstWordleRecursiveConst(const std::vector<LetterSet>& chosenSets,
                                     const LetterSet& usedLetters,
                                     const std::vector<LetterSet>& guessSets,
                                     const bool& useFutures) {
    std::vector<LetterSet> chosenSetsCopy = chosenSets;
    FindWorstWordleRecursive(chosenSetsCopy, usedLetters, guessSets,
                             useFutures);
  }

  std::chrono::steady_clock::time_point startTime;
  const std::vector<Word> guessWords;
  const std::vector<Word> ansWords;
  // Not const because we must sort this list after reading it.
  std::vector<LetterSet> guessSets;
  const std::vector<LetterSet> answerSets;
  const std::unordered_multimap<LetterSet, Word, LetterSet::Hash>
      setToWordsGuess;
  const std::unordered_multimap<LetterSet, Word, LetterSet::Hash> setToWordsAns;

  static ThreadPool pool;
  std::atomic<size_t> solutions = 0;
  std::mutex printMutex;
  std::vector<std::future<void>> futures;
};

// Definition of the static member variables.
ThreadPool WorstWordle::pool(std::thread::hardware_concurrency());
std::unordered_map<char, double> LetterSet::letterFrequency;