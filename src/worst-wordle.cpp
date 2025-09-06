// worst-wordle.cpp

#include "worst-wordle.hpp"

#include <iostream>
#include <string>

void print_usage(const std::string& program_name) {
  std::cout << "Usage: " << program_name << " [options]\n\n"
            << "Options:\n"
            << "  -h, --help      Display this help message\n"
            << "  <guess_list>    Optional: Path to the guess wordlist file\n"
            << "  <ans_list>      Optional: Path to the answer wordlist file\n"
            << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc > 1 &&
      (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
    print_usage(argv[0]);
    return 0;
  }
  std::filesystem::path guessListPath =
      (argc > 1) ? std::filesystem::path(argv[1])
                 : std::filesystem::path("wordlists/nyt/guess.txt");
  std::filesystem::path ansListPath =
      (argc > 2) ? std::filesystem::path(argv[2])
                 : std::filesystem::path("wordlists/nyt/answer.txt");
  WorstWordle worstWordle(guessListPath, ansListPath);
#ifndef DISABLE_MULTITHREAD_OPTIMIZATION
  const bool useFutures = true;
#else
  const bool useFutures = false;
#endif
  worstWordle.FindWorstWordle(useFutures);

  return 0;
}