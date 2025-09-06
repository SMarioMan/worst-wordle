# Worst Wordle

Enumerates all of the worst possible Wordle games, in under 20 seconds.

This work was inspired by [
Ellie Rasmussen](https://www.youtube.com/@EllieRasmussen)'s YouTube video, [You should play Wordle wrong](https://youtu.be/zoh5eLOjwHA).

[![You should play Wordle wrong - YouTube video](https://img.youtube.com/vi/zoh5eLOjwHA/0.jpg)](https://youtu.be/zoh5eLOjwHA)


[Wordle](https://www.nytimes.com/games/wordle/index.html) is a word guessing game.
The goal is to guess a 5-letter word of the day within 6 guesses.
Each guess has the opportunity to reveal partial information about the solution, similar to the [Mastermind](https://en.wikipedia.org/wiki/Mastermind_(board_game)) board game.
Specifically, if a letter in the guess is in the same position as it is in the answer word, it is highlighted green.
If the right letter was chosen but in the wrong position, it is yellow.
All other letters are grey.

I have always been more interested in the properties of Wordle that could be explored computationally than playing the game itself.
Early in its life, clever programmers found the wordlists of valid word guesses and a list of pre-generated solutions.
This made it possible to constrain and fully explore the solution space, finding properties like the optimal guesslist for every solution.
With the publication of the YouTube video, [You should play Wordle wrong](https://youtu.be/zoh5eLOjwHA), a novel way of exploring Wordle games emerged.
In the video, a computational solution was explored, but there were errors and missed optimizations.
I took it upon myself to see if I could do something new and interesting in this space by writing code to handle this myself.
I suggest watching the video first for context, then returning here to better understand the approaches used.

### Rules and Constraints:
- We want to find solutions with the lowest "badness" score. The "badness" of a Wordle game is scored as follows:
    - A grey letter is worth 0 points.
    - A yellow letter is worth 10 points.
    - A green letter is worth 20 points.
    - **NOTE**: This scoring system around green and yellow scoring is somewhat arbitrary, but it gives is an objective target to optimize around and matches the one used in the original video.
- A "perfectly wrong" Wordle game is a list of 6 guesses with all grey letters.
- Letters can be reused within a guess word but cannot be reused by subsequent guesses.
- Guesses must be in the official Wordle [guess list](wordlists/nyt/guess.txt) and answers must come from the official Wordle [answer list](wordlists/nyt/answer.txt).

Now we will go over each **Version** of the algorithm, with each one building on its predecessor in some way.

## **Version 1** from [You should play Wordle wrong](https://youtu.be/zoh5eLOjwHA)

### Word comparison

- Compare each letter in the guess against the corresponding letter in the answer you are testing against.
If any match, they are marked as green.
- Compare each letter in the guess against every other letter in the answer.
If any match (and are not already green), they are marked as yellow.
- All remaining letters are marked grey.

### Additional scoring rules

When exploring the search space, attempt a [greedy algorithm](https://en.wikipedia.org/wiki/Greedy_algorithm).
We prefer word choices that reuse letters when the score is otherwise equal.
For instance, when deciding whether `STATS` or `FIRST` are better options, we choose `STATS`, because it only has 3 unique letters while `FIRST` has 5.
To accomplish this, add a point to the score for each unique letter in the word.
This is helpful because using fewer letters constrains our subsequent guesses less severely.
In fact, if we use up 5 unique letters each time, we will run out of letters before we can try all of our guesses.

### Dictionary comparison

For each guess, score all of the words in the guess list.
If a guess with a lower score is found, replace the prior guess with the lowest score and track that new score record.
The guess with the lowest score is retained.

Do this for every guess.
The final guess with the lowest score is applied as the optimal solution for the given answer word.

## **Version 2** from [You should play Wordle wrong](https://youtu.be/zoh5eLOjwHA)

In the prior approach, the guess with the lowest score is always the same.
Version 1 doesn't do anything to prevent letter reuse from the previous guesses.
Instead, it will simply guess the same word every time to preserve a perfect score.
This is a legal option in a real game of Wordle, but it isn't adequately constrained to result in any interesting solutions.
To account for this, we will now first check and skip any words that contain letters from prior guesses.

To maintain the rules of Wordle Hard Mode, (Any revealed hints must be used in subsequent guesses), any green letters found in prior guesses **must** be present in the same location in subsequent guesses.
Likewise, all yellow letters found in prior guesses must be present somewhere in subsequent guesses.
Ellie added an additional consatraint that yellow letters must be placed in a different position than before.
Words violating any of these rules are likewise skipped.

If a word passes these validation steps, then they can be scored, and the algorithm otherwise continues as normal.

## **Version 3** from [You should play Wordle wrong](https://youtu.be/zoh5eLOjwHA)

Validation is more expensive than scoring, so score first, and only validate if the score is lower than the current best score found.

Add parallelism by splitting the answer list among 5 threads.

This is the final technique proposed, and it finds no solution with fewer than one yellow letter.

## Errors in the video

### Repeating yellows

The video makes the same scoring mistake caught by [3Blue1Brown
](https://www.youtube.com/@3blue1brown) in [Oh, wait, actually the best Wordle opener is not “crane”…](https://youtu.be/fRed0Xmc2Wg)

- If we have the guess word `SPEED` and the answer is `ERASE`, then both `E`s in `SPEED` should be marked yellow.
- Conversely, if we have the guess word `SPEED` and the answer is `ABIDE`, then only the first `E` in `SPEED` should be marked yellow.

In Ellie's solution, both `E`s would be marked in `SPEED` if there was even a single `E` in the solution.
This results in an incorrect score that wrongly penalizes several games.

### Greedy search

This mistake ruins the algorithm and results in suboptimal solutions.
Unfortunately, a greedy algorithm cannot find the optimal Wordle score, as pointed out by [domingo99](https://www.youtube.com/@domingo99) in the YouTube comments:

>It is possible that an earlier "worst" guess could lock you out of future guesses and force you to converge more quickly.
>
>For example, imagine a wordle universe where there are only four valid words and you only get two guesses.
>- `AGHIJ`
>- `BKLMN`
>- `ABBAB`
>- `COCCO`
>
>If the correct answer is `COCCO`, using the greedy algorithm you used in the video, the first "worst" guess is `ABBAB` (all gray with the minimum number of unique letters).
>But this consumes both the `A` and the `B` letter, which prevents you from guessing `AGHIJ` and `BKLMN`, and so you are then forced to guess `COCCO` on the next guess and win.
>
>A better strategy would have been to actually guess `AGHIJ` first and then `BKLMN` second, failing to score any points at all.
>
>Basically, the best "worst" guess made earlier locked us out of not just the letters we used, but effectively locked us out of additional letters because it removed all the remaining words that used those letters (in this example, letters `GHIJ` and `KLMN` are only used in words that also contain `A` or `B` and so by guessing `A` and `B` we eliminate those letters as well).

## **Version 4** from [danzibob9452](https://www.youtube.com/@danzibob9452) in the YouTube comments

>I used a DFS traversal of guesses prioritised by letter re-use within the word, then once you reach depth 6 you check against the list of valid wordle answers.
>I'd like to see if I can find a way to search efficiently for solutions with less cursed words but my computer is already screaming at me heheh

This version makes the following optimizations:

### Only test for "perfectly wrong" solutions

This simplifies and accelerates the checking logic significantly, as we only have to compare the used letters set against the list of letters being used in the current word being evaluated.
We can throw out all of the hard mode Wordle logic.
This means we will not find the optimal guess for every answer, but we will find a list of words that have perfect answers.

### Test against the answer last

I'm not convinced that the order that you evaluate guesses vs. answers really matters in this technique, but it will be useful in subsequent iterations of this problem.

### Depth-first search

Use a [depth-first search](https://en.wikipedia.org/wiki/Depth-first_search) to find the optimal solution.
Since DFS uses backtracking, it will not get stuck in a local optimum like the prior versions.
However, this causes a huge (but necessary) explosion in the [big O](https://en.wikipedia.org/wiki/Big_O_notation) run time of the algorithm.

Given a number of answer words, `A`, and a number of guess words, `G`:
- The greedy algorithm ran in O(6\*A\*G) time.
- The DFS algorithm will run in O(A\*G^6) time.

### Tree pruning

Even though a naïve DFS is exhaustive, it may be possible to reduce the search space sufficiently where a search for all solutions can run in a reasonable amount of time.
The main way to accomplish this is via pruning.

When we find an invalid guess due to letter reuse, we can skip all of its branches (subsequent guesses) and backtrack to another word candidate for that guess number.

### Sorting the guess list

By using a custom sort on the guess list, we can choose which branches to pursue first in the DFS.
By putting the words that reuse the most letters earliest in the traversal, we are more likely to find a solution early in the search.

By using this approach, danzibob9452 found at least 6 answers with perfect scores.

## **Version 5** from SMarioMan

### Parallelism

Version 4 provided no clear path to parallelizing the search.
In Version 3, it was trivial to parallelize on the answers list.
In Version 5, we parallelize on the first level of the DFS.
We use one thread for each and leave it to the OS thread scheduler to assign them to CPU cores.
On my machine, this makes the entire algorithm about 12x faster than the sequential approach.

### Pruning by eliminating permutations

In Version 4, every guess word is checked at every level of the DFS.
In a "perfectly bad" game of wordle, the order of the guesses doesn't matter, so we should trim out those duplicates.
For instance, `babka`, `deeve`, `grrrl`, `jinni`, `phpht`, `sowff` vs. `sowff`, `phpht`, `jinni`, `grrrl`, `deeve`, `babka` report the guesses in a different order but can be viewed as equivalent solutions.

To eliminate these duplicates, we can, at every level of the DFS, only check in lower levels for words that come after the parent guess from the sorted guess words list.
This changes the run time required to generate all of the guess lists from a 6th degree polynomial to a 6th degree [triangular](https://en.wikipedia.org/wiki/Triangular_number) run time, making the whole design about 32x faster in principle (but 55x faster when validated experimentally).

### Aggressive pruning by sorting the guess list

In Version 4, a custom sort was used to increase the likelihood of finding a perfect solution in the least amount of time.
It did this by prioritizing words with more reused letters first.

However, when considering pruning and an ultimate goal of identifying all answers with perfect solutions, it actually makes sense to do the opposite: use as many letters as possible as early as possible in the early guesses.
This will result in pruning more branches at higher levels, significantly reducing the search space.
This also synergizes with the permutation optimization, since pruning first guess words more quickly also now eliminates them from all subsequent branches.

### Sorting the guess list on letter frequency

We can extend the sorting idea even farther than letter reuse.
Instead of factoring letter reuse explicitly, we instead use a scoring technique to sort guess words.
We start by computing the relative frequency of each letter in the guess list, ignoring duplicate letters within words.

It looks something like this (depending on which wordlist you use):

| Letter | Frequency (relative %) |
| - | - |
| S | 10.8599 |
| E | 9.49376 |
| A | 9.02816 |
| O | 6.61235 |
| R | 6.27336 |
| I | 6.00788 |
| L | 5.03584 |
| T | 4.83163 |
| N | 4.5682 |
| U | 4.04133 |
| D | 3.93718 |
| Y | 3.29596 |
| M | 3.20611 |
| P | 3.1428 |
| C | 3.00598 |
| H | 2.71396 |
| B | 2.55672 |
| G | 2.53834 |
| K | 2.5363 |
| W | 1.70312 |
| F | 1.59897 |
| V | 1.07211 |
| Z | 0.726991 |
| J | 0.535032 |
| X | 0.510527 |
| Q | 0.167453 |

Once we have this list, we can score each word by summing the relative percentage of each letter in the word (ignoring duplicate letters) and sort the words with the highest score first to evaluate them earlier in the DFS.
This will prune out all of the most commonly occurring letters as early as possible, drastically reducing the branching factor.

For example: cost(`SPEED`) = 10.8599 + 3.1428 + 9.49376 + 0 + 3.93718 = 27.43364

Note that these significantly more aggressive pruning optimizations mean that checking against the answer list last is significantly faster.
Since the answer list cannot be pruned as aggressively as the guesses list (there are fewer options to remove, to begin with), we hold off on checking against it until the leaf node of the guess list, when significant pruning has already happened.

Note that this is a heuristic.
The idea is that "using an `S` in this word prunes out about 10.9% of the remaining words," which may be true at the first level but becomes less true as we apply this technique to subsequent guesses, as prior guesses have already pruned out lots of words.
For instance, most words with `Q` also have a `U`, but the algorithm has no way of knowing that filtering out the `U` words should, in effect, also filter out the `Q` words.
Thus the algorithm may place a greater priority on choosing a word with a `Q` in it than it should with perfect information.
We could probably optimize this further if we pre-computed the letter frequency of words from every subset of remaining letters, but then we whould also need to apply this to an O(n\*log(n)) sort for every prune, which occurs at every node in the DFS, which would be too expensive.

In experimental testing, the letter frequency sort was found to provide a 1.15x speedup compared to a lexicographically sorted letter list.

## **Version 6** from SMarioMan (with ideas from [colinbaker5306](https://www.youtube.com/@colinbaker5306))

### Vowel optimization (idea by [colinbaker5306](https://www.youtube.com/@colinbaker5306) in the YouTube comments)

colinbaker5306 made an interesting observation to prune the word list even more dramatically, first noting that there are 6 vowels in the English language, `A`, `E`, `I`, `O`, `U`, and `Y`, then noting that all of the words in the answer list contain vowels:

>There are only 6 legal guesses that have no vowels (`crwth`, `cwtch`, `grrls`, `grrrl`, `pfftt`, `phpht`) and as we are looking for 7 word groups (the answer and the 6 guesses) any solution must include at least one of them, so we can set all the starting words to just one of those

This improves our O(A\*G^6) run time into an O(A\*6\*G^5) time.
In principle, this will make the algorithm 2,158x faster on a guess list of 12,947 words.
In experimental testing, this made the design 122x faster.

If we used our older parallelization technique, we would only have a branching factor of 6 in this revised design, which would result in poor parallelism, using only 6 cores.
To restore full multithreaded CPU usage, we can parallelize the technique on the second level of the DFS instead.

### Using letter combinations (idea by [colinbaker5306](https://www.youtube.com/@colinbaker5306) in the YouTube comments)

In all prior approaches, words were compared against other words.
However, many words share the same list of used letters.
We can significantly reduce the search space by representing letter combinations instead of full words.
For instance, `ALERT`, `ALTER`, and `LATER` all use the same 5 letters (`A`, `E`, `L`, `R`, `T`), so we can consolidate them into a single letter combination to evaluate against other letter combinations, significantly reducing our search space.

Later, when we want to enumerate all perfect solutions, we can substitute in its full list of words, to produce actual solutions.

Using letter sets reduces a 12,947 word guess list into a 7,622 letter set guess list.
Likewise, using letter sets prunes a 2,309 word answer list into a 2,033 letter set answer list.

Note that this effectively changes the letter frequency distribution in the frequency sorting optimization, since we now want to prune out as many letter combinations as possible instead of as many words as possible.

This makes the algorithm approximately 18x faster.

### Using bitsets

While it was never explicitly detailed by the other approaches, my letter sets were originally represented using the `C++` [`std::unordered_set<char>`](https://en.cppreference.com/w/cpp/container/unordered_set.html) data structure.
It is significantly faster to store these sets as a [`std::bitset<26>`](https://en.cppreference.com/w/cpp/utility/bitset.html) instead.

To check for letter overlap, the unordered set requires checking every letter of one set against every letter of the other set.
For 5-letter words, this results in 5\*5=25 letter checks in the worst case, as well as the creation of 2 new unordered sets, one for each word in a 2-way comparison.

With a bitset, we can check for bit overlaps using a single [bitwise AND](https://en.cppreference.com/w/cpp/language/operator_arithmetic.html) operation and checking if any bits are still set in the result.
Since the hot path of our code spends most of its time doing letter overlap checks, this is a significant performance optimization, though I have not validated the speedup from this change.

### Pass partially built solutions by reference

In prior versions of my code, I treated all partially built solutions as const references.
To pass them into each recursive call of the DFS, I needed to make a copy with the modifications applied.
In profiling, I found that the construction and destruction of these copies dominated run time.
If I instead pass the partial results in by reference, adding the guess as I traverse down and removing it as I backtrack up, the need to make copies at every level disappears.
This results in a 3x speedup in my testing.

## Total performance gain

When implementing all of these optimizations, Version 6 is estimated to run approximately 5,000,292x faster than Version 4, even without accounting for bitset optimizations.
In reality, improvements from some optimizations will diminish the effective improvement from other optimizations, resulting in a much smaller improvement overall.
In practice, Version 5 took 35 hours to complete on my machine, whereas Version 6 now runs in under 20 seconds.

# Usage

## Building

### Linux

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -O3 -march=native -flto src/worst-wordle.cpp -o worst-wordle.out
```

### Windows

Open the Developer Command Prompt for VS, then run:

```bat
cl /MP /O2 /GL /arch:AVX2 /fp:fast /EHsc /std:c++17 src/worst-wordle.cpp /link /LTCG
```

#### Debugging

Run the following to preserve symbols for debugging:

```bat
cl /MP /O2 /GL /arch:AVX2 /fp:fast /EHsc /std:c++17 /Zi src/worst-wordle.cpp /link /LTCG /DEBUG
```

### Flags

Several `#define` sections of the code can be used to selectively disable a variety of optimizations at build time.

- `DISABLE_MULTITHREAD_OPTIMIZATION`: Disables [parallelism](#parallelism).
- `DISABLE_PERMUTATION_DEDUP`: Disables [permutation pruning](#pruning-by-eliminating-permutations), which will result in many duplicate answers being reported.
- `DISABLE_PRUNING`: Disables all [DFS pruning](#tree-pruning), except for permutation pruning. Can be used alongside `DISABLE_PERMUTATION_DEDUP` to comprehensively disable pruning.
- `DISABLE_RARITY_SORT`: Disables [sorting the guess list on letter frequency](#sorting-the-guess-list-on-letter-frequency), instead falling back on a simple kind of "lexicographical" order based on which letters appear in each set.
- `DISABLE_VOWEL_OPTIMIZATION`: Disables [vowel optimizations](#vowel-optimization-idea-by-colinbaker5306-in-the-youtube-comments).

- `DEBUG`: Enables additional, optional correctness checks.
- `NO_PRINT`: Disables solution printing. This is useful for benchmarking each approach without the overhead of terminal or file I/O.

To use them, add the appropriate build flag.
For instance, to disable parallelism, add the flag `-DDISABLE_MULTITHREAD_OPTIMIZATION`.
An example would be: `g++ -std=c++17 -Wall -Wextra -pedantic -O3 -march=native -flto -DDISABLE_MULTITHREAD_OPTIMIZATION src/utility.cpp -o evaluate.out`

# Evaluation and results

- Number of unique answer words with "perfectly bad" solutions: **449**
- Number of unique "perfectly bad" solutions: **1,994,009**
- Best guess set to use when targeting a "perfectly bad" wordle score:
    - Since the nature of the game inherently means that prior guesses provide no new information, our best strategy (assuming we want a perfectly bad Wordle game or bust) is to choose a set of guesses that overlaps with the fewest answers.
    - The best sets overlap with as many as **19** valid Wordle answers.
    - Picking any one of these will result in a successful, perfectly bad Wordle game 19 times out of every 2309 games, or **0.8%** of the time.

One set of optimal guesses:
- Guesses: phpht, gynny, qajaq, immix, susus, cocco

Which will result in perfectly bad Wordle games for the following solutions:
- Answers: belle, bevel, bezel, bleed, breed, defer, delve, dwell, elder, fever, fewer, freed, freer, level, lever, rebel, refer, revel, verve

## Full list of answer words with "perfectly bad" Wordle scores

`ABACK`, `ABYSS`, `ADAPT`, `AFFIX`, `ALARM`, `AMASS`, `ASSAY`, `AWARD`, `BANAL`, `BASAL`, `BAWDY`, `BEECH`, `BEEFY`, `BELLE`, `BESET`, `BEVEL`, `BEZEL`, `BIDDY`, `BLACK`, `BLAND`, `BLANK`, `BLEED`, `BLEEP`, `BLEND`, `BLESS`, `BLIMP`, `BLIND`, `BLINK`, `BLISS`, `BLOCK`, `BLOND`, `BLOOD`, `BLOOM`, `BLOWN`, `BLUFF`, `BLURB`, `BOBBY`, `BONGO`, `BOOBY`, `BOOST`, `BOOTH`, `BOOZY`, `BOSOM`, `BOSSY`, `BOTCH`, `BOUND`, `BRAND`, `BRASS`, `BRAWL`, `BRAWN`, `BREED`, `BRICK`, `BRING`, `BRINK`, `BRISK`, `BROOD`, `BROOK`, `BROOM`, `BROWN`, `BUDDY`, `BUNCH`, `BUNNY`, `BUTCH`, `BUXOM`, `CABAL`, `CABBY`, `CACAO`, `CADDY`, `CANAL`, `CANDY`, `CANNY`, `CATCH`, `CHAFF`, `CHAMP`, `CHECK`, `CHEEK`, `CHICK`, `CHOCK`, `CHUCK`, `CHUMP`, `CHUNK`, `CINCH`, `CIVIC`, `CIVIL`, `CLACK`, `CLASS`, `CLERK`, `CLICK`, `CLIFF`, `CLIMB`, `CLOCK`, `CLOWN`, `CLUCK`, `CLUNG`, `COCOA`, `COLON`, `COLOR`, `COMFY`, `CONCH`, `CONDO`, `CRACK`, `CRANK`, `CRASS`, `CRAWL`, `CREED`, `CREEK`, `CREME`, `CRESS`, `CRICK`, `CROCK`, `CROOK`, `CROSS`, `CROWD`, `CROWN`, `CRUMB`, `CUMIN`, `CYNIC`, `DADDY`, `DANDY`, `DEFER`, `DELVE`, `DENSE`, `DEUCE`, `DIZZY`, `DONOR`, `DOWDY`, `DOWNY`, `DRAMA`, `DRANK`, `DRAWL`, `DRAWN`, `DRESS`, `DRILL`, `DRINK`, `DROLL`, `DROOL`, `DROOP`, `DROSS`, `DROWN`, `DRUNK`, `DRYLY`, `DUMMY`, `DUMPY`, `DUSKY`, `DUTCH`, `DWARF`, `DWELL`, `EJECT`, `ELDER`, `EMBED`, `EMBER`, `EMCEE`, `ENEMY`, `EVENT`, `EXCEL`, `EXPEL`, `FANCY`, `FANNY`, `FEMME`, `FENCE`, `FEVER`, `FEWER`, `FIFTH`, `FINCH`, `FIZZY`, `FJORD`, `FLACK`, `FLANK`, `FLASK`, `FLECK`, `FLICK`, `FLOCK`, `FLOOD`, `FLOOR`, `FLOSS`, `FLUFF`, `FLUNG`, `FLUNK`, `FORGO`, `FRANK`, `FREED`, `FREER`, `FRILL`, `FRISK`, `FROCK`, `FROND`, `FROWN`, `FUNKY`, `FUNNY`, `FUSSY`, `FUZZY`, `GAMMA`, `GEESE`, `GENRE`, `GLAND`, `GLOOM`, `GRAND`, `GRASS`, `GREED`, `GREEN`, `GRILL`, `GRIND`, `GROOM`, `GROSS`, `GROWL`, `GROWN`, `GRUFF`, `GYPSY`, `HATCH`, `HENCE`, `HITCH`, `HUMPH`, `HUNCH`, `HUTCH`, `ICING`, `IDIOM`, `JAZZY`, `JEWEL`, `JIFFY`, `JUMBO`, `JUMPY`, `KAPPA`, `KARMA`, `KAYAK`, `KINKY`, `KNACK`, `KNEED`, `KNEEL`, `KNOCK`, `KNOLL`, `KNOWN`, `KRILL`, `LARVA`, `LEDGE`, `LEPER`, `LEVEL`, `LEVER`, `LIPID`, `LIVID`, `LLAMA`, `LUPUS`, `MACAW`, `MADAM`, `MAGMA`, `MAMMA`, `MAMMY`, `MANGA`, `MAXIM`, `MELEE`, `MERGE`, `MIMIC`, `MINIM`, `MISSY`, `MOODY`, `MORON`, `MOSSY`, `MOTTO`, `MUCKY`, `MUCUS`, `MUDDY`, `MUMMY`, `MUNCH`, `MUSKY`, `NANNY`, `NASAL`, `NAVAL`, `NEEDY`, `NERVE`, `NEVER`, `NEWER`, `NINNY`, `NYMPH`, `OVOID`, `PADDY`, `PAGAN`, `PAPAL`, `PARKA`, `PENCE`, `PENNE`, `PENNY`, `PHOTO`, `PINCH`, `PINKY`, `PITCH`, `PLAZA`, `PLUMB`, `PLUMP`, `POOCH`, `POPPY`, `PRANK`, `PREEN`, `PRESS`, `PRISM`, `PRONG`, `PROOF`, `PUFFY`, `PUNCH`, `PUPPY`, `PYGMY`, `QUACK`, `QUEEN`, `QUEUE`, `QUICK`, `RADAR`, `REBEL`, `REFER`, `RENEW`, `REPEL`, `REVEL`, `RIGID`, `SALAD`, `SALSA`, `SAPPY`, `SASSY`, `SAVVY`, `SCARF`, `SCENE`, `SCENT`, `SCOFF`, `SCOLD`, `SCORN`, `SCOWL`, `SCRAM`, `SCREE`, `SCREW`, `SCRUB`, `SCRUM`, `SEMEN`, `SENSE`, `SERVE`, `SEVEN`, `SEVER`, `SEWER`, `SISSY`, `SKIFF`, `SKILL`, `SKIMP`, `SKULK`, `SKULL`, `SKUNK`, `SLEEK`, `SLEEP`, `SLICK`, `SLOOP`, `SLUMP`, `SLUNK`, `SLURP`, `SLYLY`, `SMACK`, `SMALL`, `SMELL`, `SMIRK`, `SMOCK`, `SMOKY`, `SNACK`, `SNEER`, `SNIFF`, `SNOOP`, `SNOWY`, `SNUCK`, `SNUFF`, `SPANK`, `SPARK`, `SPASM`, `SPEED`, `SPELL`, `SPEND`, `SPERM`, `SPILL`, `SPINY`, `SPOOF`, `SPOOK`, `SPOOL`, `SPOON`, `SPREE`, `SPRIG`, `SPUNK`, `SPURN`, `STAFF`, `STIFF`, `STINT`, `STOCK`, `STOOD`, `STUCK`, `STUFF`, `STUNK`, `STUNT`, `SUNNY`, `SWARM`, `SWEEP`, `SWEET`, `SWELL`, `SWIFT`, `SWILL`, `SWING`, `SWIRL`, `SWOON`, `SWOOP`, `SWORD`, `SWORN`, `SWUNG`, `SYNOD`, `TEETH`, `TENET`, `TENSE`, `TEPEE`, `THEFT`, `THEME`, `THUMB`, `THUMP`, `TIMID`, `TOOTH`, `TWEED`, `TWEET`, `TWIST`, `TWIXT`, `UNCUT`, `UNDID`, `UNDUE`, `UNWED`, `UNZIP`, `USURP`, `VENUE`, `VERGE`, `VERSE`, `VERVE`, `VIGIL`, `VISIT`, `VIVID`, `WACKY`, `WEAVE`, `WEDGE`, `WEEDY`, `WHICH`, `WHIFF`, `WHOOP`, `WINCH`, `WINDY`, `WITCH`, `WOODY`, `WOOZY`, `WORLD`, `WOUND`, `WRACK`, `WRECK`, `WRING`, `WRONG`, `WRUNG`, `WRYLY`

## Example solutions for every perfect answer word

The output structure is of the form `[ANSWER]`: `[GUESS]`, `[GUESS]`, `[GUESS]`, `[GUESS]`, `[GUESS]`, `[GUESS]`, followed by the number of unique guess combinations that provide a perfect solution for this word.

- `ABACK`: `GRRRL`, `DJINN`, `FUFFS`, `WOOTZ`, `EXEEM`, `HYPHY` (2610 solutions)
- `ABYSS`: `GRRRL`, `PHPHT`, `EEVEN`, `KUDZU`, `COCCO`, `IMMIX` (6 solutions)
- `ADAPT`: `GRRRL`, `WYNNS`, `CHUCK`, `BOFFO`, `IMMIX`, `JEEZE` (1 solution)
- `AFFIX`: `GRRRL`, `PHPHT`, `WYNNS`, `JEMBE`, `KUDZU`, `COCCO` (1 solution)
- `ALARM`: `CWTCH`, `BIFID`, `JUJUS`, `KEEVE`, `GYNNY`, `ZOPPO` (242 solutions)
- `AMASS`: `CRWTH`, `DEEVE`, `FLUFF`, `GYNNY`, `KIBBI`, `ZOPPO` (2276 solutions)
- `ASSAY`: `GRRRL`, `CWTCH`, `BOMBO`, `KUDZU`, `JINNI`, `PEEPE` (51 solutions)
- `AWARD`: `PHPHT`, `BECKE`, `FUFFS`, `OVOLO`, `GYNNY`, `IMMIX` (3772 solutions)
- `BANAL`: `CWTCH`, `DEEVE`, `ZORRO`, `SUSUS`, `IMMIX`, `GYPPY` (66 solutions)
- `BASAL`: `PHPHT`, `FERER`, `KUDZU`, `GYNNY`, `COCCO`, `IMMIX` (8 solutions)
- `BAWDY`: `GRRRL`, `PHPHT`, `COMMO`, `SUSUS`, `JINNI`, `FEEZE` (206 solutions)
- `BEECH`: `GRRRL`, `MUNTU`, `SKYFS`, `VIVID`, `ZOPPO`, `QAJAQ` (14 solutions)
- `BEEFY`: `GRRLS`, `PHPHT`, `AJWAN`, `KUDZU`, `COCCO`, `IMMIX` (939 solutions)
- `BELLE`: `CRWTH`, `DONKO`, `FUFFS`, `IMMIX`, `GYPPY`, `QAJAQ` (2715 solutions)
- `BESET`: `GRRRL`, `AJWAN`, `KUDZU`, `HYPHY`, `COCCO`, `IMMIX` (16 solutions)
- `BEVEL`: `CRWTH`, `DONKO`, `FUFFS`, `IMMIX`, `GYPPY`, `QAJAQ` (1329 solutions)
- `BEZEL`: `CRWTH`, `DONKO`, `FUFFS`, `IMMIX`, `GYPPY`, `QAJAQ` (861 solutions)
- `BIDDY`: `GRRRL`, `CWTCH`, `AMMAN`, `FUFFS`, `KEEVE`, `ZOPPO` (1479 solutions)
- `BLACK`: `PHPHT`, `DEEVE`, `ZORRO`, `SUSUS`, `GYNNY`, `IMMIX` (26 solutions)
- `BLAND`: `CWTCH`, `FUFFS`, `ZORRO`, `KEEVE`, `IMMIX`, `GYPPY` (3 solutions)
- `BLANK`: `CWTCH`, `DEEVE`, `ZORRO`, `SUSUS`, `IMMIX`, `GYPPY` (18 solutions)
- `BLEED`: `CRWTH`, `FUFFS`, `GYNNY`, `ZOPPO`, `IMMIX`, `QAJAQ` (375 solutions)
- `BLEEP`: `CWTCH`, `DROOK`, `FUFFS`, `GYNNY`, `IMMIX`, `QAJAQ` (66 solutions)
- `BLEND`: `CWTCH`, `FUFFS`, `ZORRO`, `IMMIX`, `GYPPY`, `QAJAQ` (8 solutions)
- `BLESS`: `CWTCH`, `FRORN`, `KUDZU`, `IMMIX`, `GYPPY`, `QAJAQ` (9 solutions)
- `BLIMP`: `CWTCH`, `DEEVE`, `ZORRO`, `SUSUS`, `GYNNY`, `JAFFA` (37 solutions)
- `BLIND`: `CWTCH`, `FEMME`, `ZORRO`, `SUSUS`, `GYPPY`, `QAJAQ` (42 solutions)
- `BLINK`: `CWTCH`, `DEEVE`, `PYGMY`, `ZORRO`, `SUSUS`, `JAFFA` (87 solutions)
- `BLISS`: `CWTCH`, `DURUM`, `KEEVE`, `GYNNY`, `JAFFA`, `ZOPPO` (74 solutions)
- `BLOCK`: `PHPHT`, `ADRAD`, `FUFFS`, `GYNNY`, `IMMIX`, `JEEZE` (350 solutions)
- `BLOND`: `CRWTH`, `FUFFS`, `KEEVE`, `IMMIX`, `GYPPY`, `QAJAQ` (118 solutions)
- `BLOOD`: `CRWTH`, `ANANA`, `SUSUS`, `FEEZE`, `IMMIX`, `GYPPY` (2993 solutions)
- `BLOOM`: `CRWTH`, `ANANA`, `SUSUS`, `FEEZE`, `VIVID`, `GYPPY` (2344 solutions)
- `BLOWN`: `PHPHT`, `SKYRS`, `ADDAX`, `JUGUM`, `FEEZE`, `CIVIC` (1 solution)
- `BLUFF`: `CRWTH`, `AKKAS`, `DEEVE`, `GYNNY`, `ZOPPO`, `IMMIX` (12872 solutions)
- `BLURB`: `CWTCH`, `AKKAS`, `DEEVE`, `GYNNY`, `ZOPPO`, `IMMIX` (4202 solutions)
- `BOBBY`: `GRRLS`, `CWTCH`, `ANANA`, `KUDZU`, `PEEPE`, `IMMIX` (11327 solutions)
- `BONGO`: `CRWTH`, `FEMME`, `JUJUS`, `PZAZZ`, `VIVID`, `XYLYL` (6824 solutions)
- `BOOBY`: `GRRLS`, `CWTCH`, `ANANA`, `KUDZU`, `PEEPE`, `IMMIX` (11327 solutions)
- `BOOST`: `GRRRL`, `AJWAN`, `FEMME`, `KUDZU`, `CIVIC`, `HYPHY` (49 solutions)
- `BOOTH`: `GRRRL`, `SYNCS`, `KUDZU`, `PEEPE`, `IMMIX`, `QAJAQ` (29 solutions)
- `BOOZY`: `GRRRL`, `CWTCH`, `AMMAN`, `FUFFS`, `PEEPE`, `VIVID` (5377 solutions)
- `BOSOM`: `CRWTH`, `ADDAX`, `KEEVE`, `FLUFF`, `JINNI`, `GYPPY` (1434 solutions)
- `BOSSY`: `GRRRL`, `CWTCH`, `ANANA`, `KUDZU`, `PEEPE`, `IMMIX` (73 solutions)
- `BOTCH`: `GRRRL`, `WYNNS`, `KUDZU`, `PEEPE`, `IMMIX`, `QAJAQ` (4 solutions)
- `BOUND`: `GRRRL`, `CWTCH`, `SKYFS`, `PEEPE`, `IMMIX`, `QAJAQ` (8 solutions)
- `BRAND`: `CWTCH`, `FIFIS`, `KEEVE`, `JUGUM`, `ZOPPO`, `XYLYL` (31 solutions)
- `BRASS`: `CWTCH`, `DEEVE`, `FLUFF`, `GYNNY`, `ZOPPO`, `IMMIX` (81 solutions)
- `BRAWL`: `PHPHT`, `COMMO`, `JUJUS`, `GYNNY`, `FEEZE`, `VIVID` (167 solutions)
- `BRAWN`: `PHPHT`, `COCKS`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (172 solutions)
- `BREED`: `CWTCH`, `AFLAJ`, `KUKUS`, `GYNNY`, `ZOPPO`, `IMMIX` (1262 solutions)
- `BRICK`: `PHPHT`, `AJWAN`, `DOGGO`, `SUSUS`, `FEEZE`, `XYLYL` (449 solutions)
- `BRING`: `CWTCH`, `DEEVE`, `SUSUS`, `JAFFA`, `ZOPPO`, `XYLYL` (662 solutions)
- `BRINK`: `CWTCH`, `AGAMA`, `DEEVE`, `SUSUS`, `ZOPPO`, `XYLYL` (764 solutions)
- `BRISK`: `CWTCH`, `DEEVE`, `ANANA`, `JUGUM`, `ZOPPO`, `XYLYL` (101 solutions)
- `BROOD`: `CWTCH`, `AFLAJ`, `EEVEN`, `SUSUS`, `IMMIX`, `GYPPY` (7329 solutions)
- `BROOK`: `CWTCH`, `ADDAX`, `MEZZE`, `FLUFF`, `JINNI`, `GYPPY` (13032 solutions)
- `BROOM`: `CWTCH`, `ADDAX`, `KEEVE`, `FLUFF`, `JINNI`, `GYPPY` (5868 solutions)
- `BROWN`: `PHPHT`, `ACCAS`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (625 solutions)
- `BUDDY`: `GRRLS`, `CWTCH`, `ANANA`, `KEEVE`, `ZOPPO`, `IMMIX` (5212 solutions)
- `BUNCH`: `GRRRL`, `SKYFS`, `DEEVE`, `TAATA`, `ZOPPO`, `IMMIX` (64 solutions)
- `BUNNY`: `GRRLS`, `CWTCH`, `DEEVE`, `JAFFA`, `ZOPPO`, `IMMIX` (2868 solutions)
- `BUTCH`: `GRRRL`, `ENDED`, `SKYFS`, `ZOPPO`, `IMMIX`, `QAJAQ` (64 solutions)
- `BUXOM`: `GRRRL`, `CWTCH`, `SKYFS`, `ANANA`, `PEEPE`, `VIVID` (47 solutions)
- `CABAL`: `PHPHT`, `DEEVE`, `ZORRO`, `SUSUS`, `GYNNY`, `IMMIX` (107 solutions)
- `CABBY`: `GRRRL`, `PHPHT`, `DONKO`, `FUFFS`, `IMMIX`, `JEEZE` (55 solutions)
- `CACAO`: `GRRRL`, `PHPHT`, `SKYFS`, `BUNDU`, `IMMIX`, `JEEZE` (1 solution)
- `CADDY`: `GRRRL`, `PHPHT`, `BOMBO`, `SUSUS`, `JINNI`, `FEEZE` (137 solutions)
- `CANAL`: `PHPHT`, `SKYFS`, `DOGGO`, `URUBU`, `IMMIX`, `JEEZE` (2 solutions)
- `CANDY`: `GRRRL`, `PHPHT`, `JUJUS`, `KEEVE`, `BOFFO`, `IMMIX` (7 solutions)
- `CANNY`: `GRRRL`, `PHPHT`, `BOMBO`, `SUSUS`, `FEEZE`, `VIVID` (115 solutions)
- `CATCH`: `GRRRL`, `WYNNS`, `KUDZU`, `BOFFO`, `PEEPE`, `IMMIX` (2 solutions)
- `CHAFF`: `GRRRL`, `STYMY`, `KUDZU`, `OXBOW`, `JINNI`, `PEEPE` (20 solutions)
- `CHAMP`: `GRRRL`, `WYNNS`, `TUKTU`, `BOFFO`, `VIVID`, `JEEZE` (1 solution)
- `CHECK`: `GRRRL`, `SYNDS`, `BUTUT`, `JAFFA`, `ZOPPO`, `IMMIX` (9 solutions)
- `CHEEK`: `GRRRL`, `SYNDS`, `BUTUT`, `JAFFA`, `ZOPPO`, `IMMIX` (9 solutions)
- `CHICK`: `GRRRL`, `SYNDS`, `EMMEW`, `BUTUT`, `JAFFA`, `ZOPPO` (33 solutions)
- `CHOCK`: `GRRRL`, `SYNDS`, `BUTUT`, `FEEZE`, `IMMIX`, `QAJAQ` (45 solutions)
- `CHUCK`: `GRRRL`, `ADAPT`, `WYNNS`, `BOFFO`, `IMMIX`, `JEEZE` (840 solutions)
- `CHUMP`: `GRRRL`, `ANATA`, `SKYFS`, `OXBOW`, `VIVID`, `JEEZE` (48 solutions)
- `CHUNK`: `GRRRL`, `STYMY`, `BOFFO`, `PEEPE`, `VIVID`, `QAJAQ` (48 solutions)
- `CINCH`: `GRRRL`, `MEWED`, `SKYFS`, `BUTUT`, `ZOPPO`, `QAJAQ` (39 solutions)
- `CIVIC`: `GRRLS`, `ADDAX`, `MEZZE`, `TUKTU`, `BOFFO`, `HYPHY` (301082 solutions)
- `CIVIL`: `PHPHT`, `ABAKA`, `EMMEW`, `JUJUS`, `ZORRO`, `GYNNY` (4617 solutions)
- `CLACK`: `PHPHT`, `BEDEW`, `FUFFS`, `ZORRO`, `GYNNY`, `IMMIX` (371 solutions)
- `CLASS`: `PHPHT`, `REEVE`, `KUDZU`, `GYNNY`, `BOFFO`, `IMMIX` (2 solutions)
- `CLERK`: `PHPHT`, `BOMBO`, `SUSUS`, `GYNNY`, `VIVID`, `JAFFA` (22 solutions)
- `CLICK`: `PHPHT`, `ADRAD`, `BOMBO`, `SUSUS`, `GYNNY`, `FEEZE` (1506 solutions)
- `CLIFF`: `PHPHT`, `ABAKA`, `DEEVE`, `ZORRO`, `SUSUS`, `GYNNY` (1841 solutions)
- `CLIMB`: `PHPHT`, `DEEVE`, `ZORRO`, `SUSUS`, `GYNNY`, `JAFFA` (78 solutions)
- `CLOCK`: `PHPHT`, `ABRAM`, `FUFFS`, `GYNNY`, `VIVID`, `JEEZE` (1724 solutions)
- `CLOWN`: `PHPHT`, `SKYFS`, `AGAMA`, `URUBU`, `VIVID`, `JEEZE` (11 solutions)
- `CLUCK`: `PHPHT`, `ABBAS`, `DEEVE`, `ZORRO`, `GYNNY`, `IMMIX` (4468 solutions)
- `CLUNG`: `PHPHT`, `BEDEW`, `SKYFS`, `ZORRO`, `IMMIX`, `QAJAQ` (32 solutions)
- `COCOA`: `GRRRL`, `PHPHT`, `SKYFS`, `BUNDU`, `IMMIX`, `JEEZE` (1 solution)
- `COLON`: `PHPHT`, `SKYFS`, `AGAMA`, `URUBU`, `VIVID`, `JEEZE` (20 solutions)
- `COLOR`: `PHPHT`, `ABAKA`, `DEEVE`, `SUSUS`, `GYNNY`, `IMMIX` (684 solutions)
- `COMFY`: `GRRRL`, `PHPHT`, `ABAKA`, `DEEVE`, `SUSUS`, `JINNI` (195 solutions)
- `CONCH`: `GRRRL`, `SKYFS`, `BUTUT`, `MAMMA`, `VIVID`, `JEEZE` (33 solutions)
- `CONDO`: `GRRLS`, `BUTUT`, `FEEZE`, `HYPHY`, `IMMIX`, `QAJAQ` (4573 solutions)
- `CRACK`: `PHPHT`, `BEDEW`, `FUFFS`, `OVOLO`, `GYNNY`, `IMMIX` (1072 solutions)
- `CRANK`: `PHPHT`, `BOBOS`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (46 solutions)
- `CRASS`: `PHPHT`, `BOMBO`, `FLUFF`, `GYNNY`, `VIVID`, `JEEZE` (21 solutions)
- `CRAWL`: `PHPHT`, `BOMBO`, `SUSUS`, `GYNNY`, `FEEZE`, `VIVID` (52 solutions)
- `CREED`: `PHPHT`, `ABAKA`, `JUJUS`, `OVOLO`, `GYNNY`, `IMMIX` (377 solutions)
- `CREEK`: `PHPHT`, `AFLAJ`, `BOMBO`, `SUSUS`, `GYNNY`, `VIVID` (962 solutions)
- `CREME`: `PHPHT`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (660 solutions)
- `CRESS`: `PHPHT`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `XYLYL` (38 solutions)
- `CRICK`: `PHPHT`, `ADAWS`, `BOMBO`, `FLUFF`, `GYNNY`, `JEEZE` (5187 solutions)
- `CROCK`: `PHPHT`, `ABAND`, `SWISS`, `JUGUM`, `FEEZE`, `XYLYL` (4742 solutions)
- `CROOK`: `PHPHT`, `ABAND`, `SWISS`, `JUGUM`, `FEEZE`, `XYLYL` (4742 solutions)
- `CROSS`: `PHPHT`, `ABAKA`, `DEEVE`, `FLUFF`, `GYNNY`, `IMMIX` (196 solutions)
- `CROWD`: `PHPHT`, `ABAKA`, `FEMME`, `JUJUS`, `VILLI`, `GYNNY` (1609 solutions)
- `CROWN`: `PHPHT`, `ABAKA`, `FEMME`, `JUJUS`, `VIVID`, `XYLYL` (591 solutions)
- `CRUMB`: `PHPHT`, `AKKAS`, `DOGGO`, `JINNI`, `FEEZE`, `XYLYL` (748 solutions)
- `CUMIN`: `GRRRL`, `PHPHT`, `SKYFS`, `DEEVE`, `OXBOW`, `QAJAQ` (1 solution)
- `CYNIC`: `GRRLS`, `PHPHT`, `EMMEW`, `KUDZU`, `BOFFO`, `QAJAQ` (517 solutions)
- `DADDY`: `GRRRL`, `CWTCH`, `BENNE`, `FUFFS`, `ZOPPO`, `IMMIX` (1832 solutions)
- `DANDY`: `GRRRL`, `CWTCH`, `BUBUS`, `KEEVE`, `ZOPPO`, `IMMIX` (237 solutions)
- `DEFER`: `CWTCH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (3065 solutions)
- `DELVE`: `CRWTH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (2430 solutions)
- `DENSE`: `CWTCH`, `ABAKA`, `ZORRO`, `FLUFF`, `IMMIX`, `GYPPY` (164 solutions)
- `DEUCE`: `GRRRL`, `PHPHT`, `SKYFS`, `MINIM`, `OXBOW`, `QAJAQ` (3 solutions)
- `DIZZY`: `GRRRL`, `CWTCH`, `AMMAN`, `JUJUS`, `BOFFO`, `PEEPE` (2573 solutions)
- `DONOR`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `VILLI`, `GYPPY` (2055 solutions)
- `DOWDY`: `GRRLS`, `PHPHT`, `BUCKU`, `ANANA`, `FEEZE`, `IMMIX` (3299 solutions)
- `DOWNY`: `GRRLS`, `PHPHT`, `BUCKU`, `FEEZE`, `IMMIX`, `QAJAQ` (375 solutions)
- `DRAMA`: `CWTCH`, `BIBBS`, `KEEVE`, `FLUFF`, `GYNNY`, `ZOPPO` (907 solutions)
- `DRANK`: `CWTCH`, `BOBOL`, `JUJUS`, `FEEZE`, `IMMIX`, `GYPPY` (96 solutions)
- `DRAWL`: `PHPHT`, `BOMBO`, `SUSUS`, `GYNNY`, `CIVIC`, `JEEZE` (118 solutions)
- `DRAWN`: `PHPHT`, `BIBBS`, `JUGUM`, `FEEZE`, `COCCO`, `XYLYL` (176 solutions)
- `DRESS`: `CWTCH`, `ABAKA`, `FLUFF`, `GYNNY`, `ZOPPO`, `IMMIX` (53 solutions)
- `DRILL`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `GYNNY`, `ZOPPO` (1165 solutions)
- `DRINK`: `CWTCH`, `AGAMA`, `JUJUS`, `BOFFO`, `PEEPE`, `XYLYL` (638 solutions)
- `DROLL`: `CWTCH`, `ABAKA`, `FEMME`, `SUSUS`, `JINNI`, `GYPPY` (1260 solutions)
- `DROOL`: `CWTCH`, `ABAKA`, `FEMME`, `SUSUS`, `JINNI`, `GYPPY` (1260 solutions)
- `DROOP`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `VILLI`, `GYNNY` (1609 solutions)
- `DROSS`: `CWTCH`, `ABAKA`, `EXEEM`, `FLUFF`, `JINNI`, `GYPPY` (315 solutions)
- `DROWN`: `PHPHT`, `ABAKA`, `FEMME`, `JUJUS`, `CIVIC`, `XYLYL` (501 solutions)
- `DRUNK`: `CWTCH`, `ABBAS`, `OVOLO`, `FEEZE`, `IMMIX`, `GYPPY` (1222 solutions)
- `DRYLY`: `CWTCH`, `ABAKA`, `FEMME`, `SUSUS`, `JINNI`, `ZOPPO` (11090 solutions)
- `DUMMY`: `GRRLS`, `CWTCH`, `EEVEN`, `KIBBI`, `JAFFA`, `ZOPPO` (2547 solutions)
- `DUMPY`: `GRRRL`, `CWTCH`, `BOBOS`, `INFIX`, `KEEVE`, `QAJAQ` (12 solutions)
- `DUSKY`: `GRRRL`, `CWTCH`, `ANANA`, `BOFFO`, `IMMIX`, `JEEZE` (204 solutions)
- `DUTCH`: `GRRRL`, `SKYFS`, `BENNE`, `ZOPPO`, `IMMIX`, `QAJAQ` (19 solutions)
- `DWARF`: `PHPHT`, `BECKE`, `JUJUS`, `OVOLO`, `GYNNY`, `IMMIX` (1025 solutions)
- `DWELL`: `PHPHT`, `ABACA`, `JUJUS`, `ZORRO`, `GYNNY`, `IMMIX` (1299 solutions)
- `EJECT`: `GRRLS`, `ANANA`, `KUDZU`, `BOFFO`, `HYPHY`, `IMMIX` (314 solutions)
- `ELDER`: `CWTCH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (267 solutions)
- `EMBED`: `CRWTH`, `FILII`, `SUSUS`, `GYNNY`, `ZOPPO`, `QAJAQ` (3979 solutions)
- `EMBER`: `CWTCH`, `AFLAJ`, `KUKUS`, `GYNNY`, `VIVID`, `ZOPPO` (1640 solutions)
- `EMCEE`: `GRRLS`, `ADDAX`, `TUKTU`, `JINNI`, `BOFFO`, `HYPHY` (15045 solutions)
- `ENEMY`: `GRRLS`, `PHPHT`, `KUDZU`, `BOFFO`, `CIVIC`, `QAJAQ` (229 solutions)
- `EVENT`: `GRRLS`, `KUDZU`, `BOFFO`, `HYPHY`, `IMMIX`, `QAJAQ` (444 solutions)
- `EXCEL`: `PHPHT`, `ABAKA`, `JUJUS`, `ZORRO`, `GYNNY`, `VIVID` (479 solutions)
- `EXPEL`: `CRWTH`, `BOMBO`, `SUSUS`, `GYNNY`, `VIVID`, `JAFFA` (446 solutions)
- `FANCY`: `GRRRL`, `PHPHT`, `BOMBO`, `SUSUS`, `VIVID`, `JEEZE` (16 solutions)
- `FANNY`: `GRRRL`, `CWTCH`, `BOBOS`, `KUDZU`, `PEEPE`, `IMMIX` (602 solutions)
- `FEMME`: `CRWTH`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (48977 solutions)
- `FENCE`: `GRRLS`, `BOMBO`, `TUKTU`, `VIVID`, `HYPHY`, `QAJAQ` (2043 solutions)
- `FEVER`: `CWTCH`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (7119 solutions)
- `FEWER`: `PHPHT`, `ABACA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (7962 solutions)
- `FIFTH`: `GRRRL`, `SYNCS`, `BOMBO`, `KUDZU`, `PEEPE`, `QAJAQ` (36 solutions)
- `FINCH`: `GRRRL`, `STYMY`, `KUDZU`, `OXBOW`, `PEEPE`, `QAJAQ` (5 solutions)
- `FIZZY`: `GRRLS`, `PHPHT`, `BUNDU`, `COMMO`, `KEEVE`, `QAJAQ` (6019 solutions)
- `FJORD`: `CWTCH`, `ABAKA`, `SUSUS`, `GYNNY`, `PEEPE`, `IMMIX` (2400 solutions)
- `FLACK`: `PHPHT`, `BEDEW`, `JUJUS`, `ZORRO`, `GYNNY`, `IMMIX` (75 solutions)
- `FLANK`: `CRWTH`, `BOMBO`, `SUSUS`, `VIVID`, `GYPPY`, `JEEZE` (31 solutions)
- `FLASK`: `CWTCH`, `DEEVE`, `URUBU`, `GYNNY`, `ZOPPO`, `IMMIX` (20 solutions)
- `FLECK`: `PHPHT`, `BOMBO`, `SUSUS`, `GYNNY`, `VIVID`, `QAJAQ` (70 solutions)
- `FLICK`: `PHPHT`, `ADRAD`, `BOMBO`, `SUSUS`, `GYNNY`, `JEEZE` (342 solutions)
- `FLOCK`: `PHPHT`, `ABRAM`, `SUSUS`, `GYNNY`, `VIVID`, `JEEZE` (472 solutions)
- `FLOOD`: `CRWTH`, `ABAKA`, `JUJUS`, `GYNNY`, `PEEPE`, `IMMIX` (5185 solutions)
- `FLOOR`: `CWTCH`, `ABAKA`, `DEEVE`, `PYGMY`, `SUSUS`, `JINNI` (3275 solutions)
- `FLOSS`: `CRWTH`, `BENNE`, `KUDZU`, `IMMIX`, `GYPPY`, `QAJAQ` (419 solutions)
- `FLUFF`: `CRWTH`, `ABAKA`, `DEEVE`, `GYNNY`, `ZOPPO`, `IMMIX` (78115 solutions)
- `FLUNG`: `CWTCH`, `SKYRS`, `BOMBO`, `PEEPE`, `VIVID`, `QAJAQ` (39 solutions)
- `FLUNK`: `CRWTH`, `BOBOS`, `DEEVE`, `IMMIX`, `GYPPY`, `QAJAQ` (325 solutions)
- `FORGO`: `CWTCH`, `ABAKA`, `DEEVE`, `SUSUS`, `JINNI`, `XYLYL` (9413 solutions)
- `FRANK`: `CWTCH`, `BEGEM`, `JUJUS`, `VIVID`, `ZOPPO`, `XYLYL` (223 solutions)
- `FREED`: `CWTCH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (3065 solutions)
- `FREER`: `CWTCH`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (17892 solutions)
- `FRILL`: `CWTCH`, `ABAKA`, `DEEVE`, `SUSUS`, `GYNNY`, `ZOPPO` (2105 solutions)
- `FRISK`: `CWTCH`, `BUNDU`, `OVOLO`, `EXEEM`, `GYPPY`, `QAJAQ` (76 solutions)
- `FROCK`: `PHPHT`, `ABBAS`, `NEEZE`, `JUGUM`, `VIVID`, `XYLYL` (1134 solutions)
- `FROND`: `CWTCH`, `ABAKA`, `JUJUS`, `MEZZE`, `VILLI`, `GYPPY` (547 solutions)
- `FROWN`: `PHPHT`, `ABACA`, `JUJUS`, `MEZZE`, `VIVID`, `XYLYL` (994 solutions)
- `FUNKY`: `GRRLS`, `CWTCH`, `BOMBO`, `PEEPE`, `VIVID`, `QAJAQ` (1069 solutions)
- `FUNNY`: `GRRLS`, `CWTCH`, `ABAKA`, `DEEVE`, `ZOPPO`, `IMMIX` (5058 solutions)
- `FUSSY`: `GRRRL`, `CWTCH`, `ABAKA`, `DEEVE`, `JINNI`, `ZOPPO` (1849 solutions)
- `FUZZY`: `GRRLS`, `CWTCH`, `ANANA`, `BOMBO`, `PEEPE`, `VIVID` (22391 solutions)
- `GAMMA`: `CRWTH`, `BENNE`, `FUFFS`, `VIVID`, `ZOPPO`, `XYLYL` (17808 solutions)
- `GEESE`: `CRWTH`, `KUDZU`, `JINNI`, `BOFFO`, `MAMMA`, `XYLYL` (685 solutions)
- `GENRE`: `CWTCH`, `ABAKA`, `JUJUS`, `VIVID`, `ZOPPO`, `XYLYL` (828 solutions)
- `GLAND`: `PHPHT`, `SKYFS`, `URUBU`, `COCCO`, `IMMIX`, `JEEZE` (1 solution)
- `GLOOM`: `CWTCH`, `SKYFS`, `BUNDU`, `VIZIR`, `PEEPE`, `QAJAQ` (89 solutions)
- `GRAND`: `CWTCH`, `FEMME`, `JUJUS`, `KIBBI`, `ZOPPO`, `XYLYL` (88 solutions)
- `GRASS`: `CWTCH`, `BOMBO`, `KUDZU`, `JINNI`, `PEEPE`, `XYLYL` (20 solutions)
- `GREED`: `CWTCH`, `ABAKA`, `SUSUS`, `JINNI`, `ZOPPO`, `XYLYL` (1172 solutions)
- `GREEN`: `CWTCH`, `ABAKA`, `JUJUS`, `VIVID`, `ZOPPO`, `XYLYL` (828 solutions)
- `GRILL`: `CWTCH`, `SKYFS`, `BUNDU`, `EXEEM`, `ZOPPO`, `QAJAQ` (22 solutions)
- `GRIND`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `ZOPPO`, `XYLYL` (627 solutions)
- `GROOM`: `CWTCH`, `ABAKA`, `DEEVE`, `SUSUS`, `JINNI`, `XYLYL` (4997 solutions)
- `GROSS`: `CWTCH`, `EEVEN`, `FLYBY`, `KUDZU`, `IMMIX`, `QAJAQ` (170 solutions)
- `GROWL`: `PHPHT`, `SKYFS`, `BUNDU`, `CEZVE`, `IMMIX`, `QAJAQ` (6 solutions)
- `GROWN`: `PHPHT`, `ABACA`, `FEMME`, `JUJUS`, `VIVID`, `XYLYL` (1267 solutions)
- `GRUFF`: `CWTCH`, `ABAKA`, `DEEVE`, `JINNI`, `ZOPPO`, `XYLYL` (28776 solutions)
- `GYPSY`: `CRWTH`, `ANANA`, `BOMBO`, `FLUFF`, `VIVID`, `JEEZE` (1514 solutions)
- `HATCH`: `GRRRL`, `WYNNS`, `KUDZU`, `BOFFO`, `PEEPE`, `IMMIX` (2 solutions)
- `HENCE`: `GRRRL`, `SKYFS`, `BUTUT`, `MAMMA`, `VIVID`, `ZOPPO` (4 solutions)
- `HITCH`: `GRRRL`, `SKYFS`, `BUNDU`, `EMMEW`, `ZOPPO`, `QAJAQ` (13 solutions)
- `HUMPH`: `GRRRL`, `ABAND`, `SKYFS`, `TWIXT`, `COCCO`, `JEEZE` (993 solutions)
- `HUNCH`: `GRRRL`, `BEDEW`, `SKYFS`, `IMMIT`, `ZOPPO`, `QAJAQ` (337 solutions)
- `HUTCH`: `GRRRL`, `BEDEW`, `SKYFS`, `ANANA`, `ZOPPO`, `IMMIX` (315 solutions)
- `ICING`: `PHPHT`, `ABAKA`, `DEEVE`, `ZORRO`, `SUSUS`, `XYLYL` (3914 solutions)
- `IDIOM`: `GRRRL`, `PHPHT`, `WYNNS`, `BUCKU`, `FEEZE`, `QAJAQ` (1 solution)
- `JAZZY`: `GRRLS`, `PHPHT`, `BUNDU`, `KEEVE`, `COCCO`, `IMMIX` (2609 solutions)
- `JEWEL`: `PHPHT`, `ABACA`, `ZORRO`, `SUSUS`, `GYNNY`, `IMMIX` (2698 solutions)
- `JIFFY`: `GRRLS`, `CWTCH`, `ANANA`, `BOMBO`, `KUDZU`, `PEEPE` (6023 solutions)
- `JUMBO`: `GRRRL`, `CWTCH`, `SKYFS`, `ANANA`, `PEEPE`, `VIVID` (11 solutions)
- `JUMPY`: `GRRRL`, `CWTCH`, `AKKAS`, `NEEZE`, `BOFFO`, `VIVID` (50 solutions)
- `KAPPA`: `CRWTH`, `BIFID`, `JUJUS`, `OVOLO`, `EXEEM`, `GYNNY` (12275 solutions)
- `KARMA`: `CWTCH`, `BELEE`, `FUFFS`, `GYNNY`, `VIVID`, `ZOPPO` (1787 solutions)
- `KAYAK`: `GRRLS`, `PHPHT`, `BUNDU`, `COCCO`, `IMMIX`, `JEEZE` (2796 solutions)
- `KINKY`: `GRRRL`, `CWTCH`, `BOMBO`, `SUSUS`, `ADDAX`, `FEEZE` (1271 solutions)
- `KNACK`: `GRRLS`, `PHPHT`, `BOMBO`, `VIVID`, `FUFFY`, `JEEZE` (2156 solutions)
- `KNEED`: `CRWTH`, `BOBOL`, `SUSUS`, `IMMIX`, `GYPPY`, `QAJAQ` (3840 solutions)
- `KNEEL`: `CRWTH`, `BOMBO`, `SUSUS`, `VIVID`, `GYPPY`, `QAJAQ` (183 solutions)
- `KNOCK`: `GRRLS`, `BUTUT`, `FEEZE`, `HYPHY`, `IMMIX`, `QAJAQ` (8597 solutions)
- `KNOLL`: `CRWTH`, `BIFID`, `SUSUS`, `EXEEM`, `GYPPY`, `QAJAQ` (1291 solutions)
- `KNOWN`: `GRRLS`, `ADDAX`, `BUTUT`, `CIVIC`, `HYPHY`, `JEEZE` (22725 solutions)
- `KRILL`: `CWTCH`, `ADMAN`, `SUSUS`, `BOFFO`, `GYPPY`, `JEEZE` (1633 solutions)
- `LARVA`: `CWTCH`, `BIFID`, `JUJUS`, `EXEEM`, `GYNNY`, `ZOPPO` (922 solutions)
- `LEDGE`: `CWTCH`, `SKYFS`, `JNANA`, `URUBU`, `ZOPPO`, `IMMIX` (34 solutions)
- `LEPER`: `CWTCH`, `BOBOS`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (42 solutions)
- `LEVEL`: `CRWTH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (10331 solutions)
- `LEVER`: `CWTCH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (835 solutions)
- `LIPID`: `CRWTH`, `BOMBO`, `SUSUS`, `GYNNY`, `FEEZE`, `QAJAQ` (418 solutions)
- `LIVID`: `CRWTH`, `ABAKA`, `FEMME`, `JUJUS`, `GYNNY`, `ZOPPO` (9374 solutions)
- `LLAMA`: `CRWTH`, `BIFID`, `JUJUS`, `KEEVE`, `GYNNY`, `ZOPPO` (3669 solutions)
- `LUPUS`: `CRWTH`, `BOMBO`, `GYNNY`, `FEEZE`, `VIVID`, `QAJAQ` (200 solutions)
- `MACAW`: `GRRLS`, `DEEVE`, `TUKTU`, `JINNI`, `BOFFO`, `HYPHY` (6335 solutions)
- `MADAM`: `CRWTH`, `BIBBS`, `KEEVE`, `FLUFF`, `GYNNY`, `ZOPPO` (21825 solutions)
- `MAGMA`: `CRWTH`, `BENNE`, `FUFFS`, `VIVID`, `ZOPPO`, `XYLYL` (17808 solutions)
- `MAMMA`: `CRWTH`, `BEEFS`, `OVOLO`, `KUDZU`, `JINNI`, `GYPPY` (174894 solutions)
- `MAMMY`: `GRRLS`, `CWTCH`, `KUDZU`, `JINNI`, `BOFFO`, `PEEPE` (1787 solutions)
- `MANGA`: `CRWTH`, `BIFID`, `JUJUS`, `KEEVE`, `ZOPPO`, `XYLYL` (2269 solutions)
- `MAXIM`: `GRRRL`, `PHPHT`, `SKYFS`, `BUNDU`, `COCCO`, `JEEZE` (1 solution)
- `MELEE`: `CRWTH`, `ABAKA`, `JUJUS`, `GYNNY`, `VIVID`, `ZOPPO` (2896 solutions)
- `MERGE`: `CWTCH`, `ABAKA`, `JUJUS`, `VIVID`, `ZOPPO`, `XYLYL` (1643 solutions)
- `MIMIC`: `GRRLS`, `ADDAX`, `TUKTU`, `BOFFO`, `HYPHY`, `JEEZE` (76422 solutions)
- `MINIM`: `CRWTH`, `ABAKA`, `DEEVE`, `SUSUS`, `ZOPPO`, `XYLYL` (65191 solutions)
- `MISSY`: `GRRRL`, `CWTCH`, `ANANA`, `KUDZU`, `BOFFO`, `PEEPE` (29 solutions)
- `MOODY`: `GRRRL`, `CWTCH`, `ABAKA`, `SUSUS`, `JINNI`, `FEEZE` (627 solutions)
- `MORON`: `CWTCH`, `ABAKA`, `DEEVE`, `FILII`, `SUSUS`, `GYPPY` (1793 solutions)
- `MOSSY`: `GRRRL`, `CWTCH`, `INFIX`, `KUDZU`, `PEEPE`, `QAJAQ` (22 solutions)
- `MOTTO`: `GRRLS`, `AJWAN`, `BUCKU`, `FEEZE`, `VIVID`, `HYPHY` (4878 solutions)
- `MUCKY`: `GRRLS`, `PHPHT`, `ANANA`, `BOFFO`, `VIVID`, `JEEZE` (311 solutions)
- `MUCUS`: `GRRRL`, `ABAFT`, `KNOWN`, `VIVID`, `HYPHY`, `JEEZE` (3867 solutions)
- `MUDDY`: `GRRLS`, `CWTCH`, `EEVEN`, `KIBBI`, `JAFFA`, `ZOPPO` (2547 solutions)
- `MUMMY`: `GRRLS`, `CWTCH`, `ABAKA`, `DEEVE`, `JINNI`, `ZOPPO` (19436 solutions)
- `MUNCH`: `GRRRL`, `BEWET`, `SKYFS`, `VIVID`, `ZOPPO`, `QAJAQ` (38 solutions)
- `MUSKY`: `GRRRL`, `CWTCH`, `ADDAX`, `JINNI`, `BOFFO`, `PEEPE` (126 solutions)
- `NANNY`: `GRRLS`, `CWTCH`, `KUDZU`, `BOFFO`, `PEEPE`, `IMMIX` (1754 solutions)
- `NASAL`: `CWTCH`, `REEVE`, `KUDZU`, `BOFFO`, `IMMIX`, `GYPPY` (2 solutions)
- `NAVAL`: `CRWTH`, `DEKED`, `JUJUS`, `BOFFO`, `IMMIX`, `GYPPY` (314 solutions)
- `NEEDY`: `GRRRL`, `CWTCH`, `ABAKA`, `JUJUS`, `ZOPPO`, `IMMIX` (210 solutions)
- `NERVE`: `CWTCH`, `AGAMA`, `JUJUS`, `KIBBI`, `ZOPPO`, `XYLYL` (1341 solutions)
- `NEVER`: `CWTCH`, `AGAMA`, `JUJUS`, `KIBBI`, `ZOPPO`, `XYLYL` (1341 solutions)
- `NEWER`: `PHPHT`, `ABAKA`, `COMMO`, `JUJUS`, `VIVID`, `XYLYL` (1730 solutions)
- `NINNY`: `GRRLS`, `CWTCH`, `BOMBO`, `KUDZU`, `PEEPE`, `JAFFA` (5321 solutions)
- `NYMPH`: `GRRLS`, `ADDAX`, `TUKTU`, `BOFFO`, `CIVIC`, `JEEZE` (2600 solutions)
- `OVOID`: `GRRRL`, `PHPHT`, `WYNNS`, `BUCKU`, `EXEEM`, `JAFFA` (10 solutions)
- `PADDY`: `GRRRL`, `CWTCH`, `BOMBO`, `SUSUS`, `JINNI`, `FEEZE` (63 solutions)
- `PAGAN`: `CRWTH`, `BOMBO`, `SUSUS`, `FEEZE`, `VIVID`, `XYLYL` (372 solutions)
- `PAPAL`: `CRWTH`, `BOMBO`, `SUSUS`, `GYNNY`, `FEEZE`, `VIVID` (846 solutions)
- `PARKA`: `CWTCH`, `BIFID`, `JUJUS`, `OVOLO`, `EXEEM`, `GYNNY` (579 solutions)
- `PENCE`: `GRRRL`, `FIFIS`, `MYTHY`, `KUDZU`, `OXBOW`, `QAJAQ` (43 solutions)
- `PENNE`: `CRWTH`, `AGAMA`, `JUJUS`, `BOFFO`, `VIVID`, `XYLYL` (4329 solutions)
- `PENNY`: `GRRLS`, `CWTCH`, `KUDZU`, `BOFFO`, `IMMIX`, `QAJAQ` (43 solutions)
- `PHOTO`: `GRRRL`, `SKYFS`, `BUNDU`, `CEZVE`, `IMMIX`, `QAJAQ` (27 solutions)
- `PINCH`: `GRRRL`, `XYSTS`, `EMMEW`, `KUDZU`, `BOFFO`, `QAJAQ` (1 solution)
- `PINKY`: `GRRRL`, `CWTCH`, `BOMBO`, `SUSUS`, `ADDAX`, `FEEZE` (39 solutions)
- `PITCH`: `GRRRL`, `WYNNS`, `KUDZU`, `EXEEM`, `BOFFO`, `QAJAQ` (2 solutions)
- `PLAZA`: `CRWTH`, `DEEVE`, `SUSUS`, `GYNNY`, `BOFFO`, `IMMIX` (275 solutions)
- `PLUMB`: `CRWTH`, `KOOKS`, `GYNNY`, `FEEZE`, `VIVID`, `QAJAQ` (60 solutions)
- `PLUMP`: `CRWTH`, `AKKAS`, `GYNNY`, `BOFFO`, `VIVID`, `JEEZE` (629 solutions)
- `POOCH`: `GRRRL`, `AXMAN`, `SKYFS`, `BUTUT`, `VIVID`, `JEEZE` (153 solutions)
- `POPPY`: `GRRLS`, `CWTCH`, `BENNE`, `KUDZU`, `IMMIX`, `QAJAQ` (3862 solutions)
- `PRANK`: `CWTCH`, `BOBOS`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (36 solutions)
- `PREEN`: `CWTCH`, `AGAMA`, `JUJUS`, `BOFFO`, `VIVID`, `XYLYL` (134 solutions)
- `PRESS`: `CWTCH`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `XYLYL` (24 solutions)
- `PRISM`: `CWTCH`, `EEVEN`, `KUDZU`, `BOFFO`, `XYLYL`, `QAJAQ` (7 solutions)
- `PRONG`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `VIVID`, `XYLYL` (211 solutions)
- `PROOF`: `CWTCH`, `ABAKA`, `DEEVE`, `SUSUS`, `GYNNY`, `IMMIX` (3179 solutions)
- `PUFFY`: `GRRLS`, `CWTCH`, `ANANA`, `BOMBO`, `VIVID`, `JEEZE` (1434 solutions)
- `PUNCH`: `GRRRL`, `SKYFS`, `BOMBO`, `TAATA`, `VIVID`, `JEEZE` (33 solutions)
- `PUPPY`: `GRRLS`, `CWTCH`, `ADDAX`, `KEEVE`, `JINNI`, `BOFFO` (5479 solutions)
- `PYGMY`: `CRWTH`, `ABAKA`, `OVOLO`, `SUSUS`, `JINNI`, `FEEZE` (13066 solutions)
- `QUACK`: `GRRRL`, `PHPHT`, `SYNDS`, `BOFFO`, `IMMIX`, `JEEZE` (10 solutions)
- `QUEEN`: `GRRRL`, `CWTCH`, `SKYFS`, `BOMBO`, `PZAZZ`, `VIVID` (15 solutions)
- `QUEUE`: `GRRRL`, `CWTCH`, `ABAND`, `SKYFS`, `ZOPPO`, `IMMIX` (354 solutions)
- `QUICK`: `GRRRL`, `PHPHT`, `SYNDS`, `BOFFO`, `MAMMA`, `JEEZE` (15 solutions)
- `RADAR`: `CWTCH`, `BEEFS`, `KININ`, `JUGUM`, `ZOPPO`, `XYLYL` (9026 solutions)
- `REBEL`: `CWTCH`, `DONKO`, `FUFFS`, `IMMIX`, `GYPPY`, `QAJAQ` (232 solutions)
- `REFER`: `CWTCH`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (17892 solutions)
- `RENEW`: `PHPHT`, `ABAKA`, `COMMO`, `JUJUS`, `VIVID`, `XYLYL` (1730 solutions)
- `REPEL`: `CWTCH`, `BOBOS`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (42 solutions)
- `REVEL`: `CWTCH`, `ABAKA`, `JUJUS`, `GYNNY`, `ZOPPO`, `IMMIX` (835 solutions)
- `RIGID`: `CWTCH`, `ABAKA`, `FEMME`, `JUJUS`, `ZOPPO`, `XYLYL` (5606 solutions)
- `SALAD`: `CWTCH`, `KNURR`, `BOFFO`, `IMMIX`, `GYPPY`, `JEEZE` (11 solutions)
- `SALSA`: `CRWTH`, `EEVEN`, `KUDZU`, `BOFFO`, `IMMIX`, `GYPPY` (205 solutions)
- `SAPPY`: `GRRRL`, `CWTCH`, `EEVEN`, `KUDZU`, `BOFFO`, `IMMIX` (3 solutions)
- `SASSY`: `GRRRL`, `CWTCH`, `BOMBO`, `KUDZU`, `JINNI`, `PEEPE` (51 solutions)
- `SAVVY`: `GRRRL`, `CWTCH`, `BOMBO`, `KUDZU`, `JINNI`, `PEEPE` (26 solutions)
- `SCARF`: `PHPHT`, `EMMEW`, `KUDZU`, `GOBBO`, `JINNI`, `XYLYL` (1 solution)
- `SCENE`: `GRRRL`, `BOMBO`, `TUKTU`, `VIVID`, `HYPHY`, `QAJAQ` (67 solutions)
- `SCENT`: `GRRRL`, `KUDZU`, `BOFFO`, `HYPHY`, `IMMIX`, `QAJAQ` (1 solution)
- `SCOFF`: `GRRRL`, `ABAND`, `TUKTU`, `HYPHY`, `IMMIX`, `JEEZE` (2568 solutions)
- `SCOLD`: `PHPHT`, `URUBU`, `GYNNY`, `FEEZE`, `IMMIX`, `QAJAQ` (5 solutions)
- `SCORN`: `PHPHT`, `ABAKA`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (3 solutions)
- `SCOWL`: `PHPHT`, `BEVER`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (40 solutions)
- `SCRAM`: `PHPHT`, `OXBOW`, `FLUFF`, `GYNNY`, `VIVID`, `JEEZE` (1 solution)
- `SCREE`: `PHPHT`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `XYLYL` (38 solutions)
- `SCREW`: `PHPHT`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `XYLYL` (24 solutions)
- `SCRUB`: `PHPHT`, `DEKED`, `OVOLO`, `GYNNY`, `IMMIX`, `QAJAQ` (71 solutions)
- `SCRUM`: `PHPHT`, `ABAKA`, `DOGGO`, `JINNI`, `FEEZE`, `XYLYL` (147 solutions)
- `SEMEN`: `CRWTH`, `VILLI`, `KUDZU`, `BOFFO`, `GYPPY`, `QAJAQ` (209 solutions)
- `SENSE`: `CRWTH`, `ABAKA`, `JUGUM`, `VIVID`, `ZOPPO`, `XYLYL` (1306 solutions)
- `SERVE`: `CWTCH`, `ABAKA`, `FLUFF`, `GYNNY`, `ZOPPO`, `IMMIX` (293 solutions)
- `SEVEN`: `CRWTH`, `BOBOL`, `KUDZU`, `IMMIX`, `GYPPY`, `QAJAQ` (553 solutions)
- `SEVER`: `CWTCH`, `ABAKA`, `FLUFF`, `GYNNY`, `ZOPPO`, `IMMIX` (293 solutions)
- `SEWER`: `PHPHT`, `ABACA`, `OVOLO`, `KUDZU`, `GYNNY`, `IMMIX` (353 solutions)
- `SISSY`: `GRRRL`, `CWTCH`, `AMMAN`, `KUDZU`, `BOFFO`, `PEEPE` (223 solutions)
- `SKIFF`: `CRWTH`, `BUNDU`, `OVOLO`, `EXEEM`, `GYPPY`, `QAJAQ` (4571 solutions)
- `SKILL`: `CWTCH`, `BUNDU`, `FEMME`, `ZORRO`, `GYPPY`, `QAJAQ` (121 solutions)
- `SKIMP`: `CWTCH`, `BLUFF`, `DEEVE`, `ZORRO`, `GYNNY`, `QAJAQ` (33 solutions)
- `SKULK`: `CRWTH`, `ADDAX`, `MEZZE`, `JINNI`, `BOFFO`, `GYPPY` (2326 solutions)
- `SKULL`: `CRWTH`, `ADDAX`, `MEZZE`, `JINNI`, `BOFFO`, `GYPPY` (2326 solutions)
- `SKUNK`: `CRWTH`, `ADDAX`, `MEZZE`, `VILLI`, `BOFFO`, `GYPPY` (8315 solutions)
- `SLEEK`: `CWTCH`, `BUNDU`, `ZORRO`, `IMMIX`, `GYPPY`, `QAJAQ` (22 solutions)
- `SLEEP`: `CRWTH`, `KUDZU`, `GYNNY`, `BOFFO`, `IMMIX`, `QAJAQ` (2 solutions)
- `SLICK`: `PHPHT`, `DURUM`, `OXBOW`, `GYNNY`, `FEEZE`, `QAJAQ` (1 solution)
- `SLOOP`: `CWTCH`, `BEVER`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (40 solutions)
- `SLUMP`: `CWTCH`, `BIFID`, `ZORRO`, `KEEVE`, `GYNNY`, `QAJAQ` (12 solutions)
- `SLUNK`: `CRWTH`, `BOFFO`, `MAMMA`, `VIVID`, `GYPPY`, `JEEZE` (64 solutions)
- `SLURP`: `CWTCH`, `BOMBO`, `GYNNY`, `FEEZE`, `VIVID`, `QAJAQ` (9 solutions)
- `SLYLY`: `CRWTH`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `PEEPE` (15721 solutions)
- `SMACK`: `GRRRL`, `PHPHT`, `DEEVE`, `OXBOW`, `FUZZY`, `JINNI` (15 solutions)
- `SMALL`: `CWTCH`, `KNURR`, `BOFFO`, `VIVID`, `GYPPY`, `JEEZE` (17 solutions)
- `SMELL`: `CWTCH`, `KNURR`, `BOFFO`, `VIVID`, `GYPPY`, `QAJAQ` (20 solutions)
- `SMIRK`: `CWTCH`, `BLUFF`, `DEEVE`, `GYNNY`, `ZOPPO`, `QAJAQ` (41 solutions)
- `SMOCK`: `GRRRL`, `ADDAX`, `BUTUT`, `JINNI`, `FEEZE`, `HYPHY` (127 solutions)
- `SMOKY`: `GRRRL`, `PHPHT`, `BUNDU`, `FEEZE`, `CIVIC`, `QAJAQ` (1 solution)
- `SNACK`: `GRRRL`, `PHPHT`, `BOMBO`, `VIVID`, `FUFFY`, `JEEZE` (13 solutions)
- `SNEER`: `CWTCH`, `ABAKA`, `JUGUM`, `VIVID`, `ZOPPO`, `XYLYL` (45 solutions)
- `SNIFF`: `CRWTH`, `ABAKA`, `DEEVE`, `JUGUM`, `ZOPPO`, `XYLYL` (2032 solutions)
- `SNOOP`: `CRWTH`, `ABAKA`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (312 solutions)
- `SNOWY`: `GRRRL`, `PHPHT`, `BUCKU`, `DEEVE`, `IMMIX`, `QAJAQ` (19 solutions)
- `SNUCK`: `GRRRL`, `BIFID`, `WOOTZ`, `EXEEM`, `HYPHY`, `QAJAQ` (644 solutions)
- `SNUFF`: `CRWTH`, `ABAKA`, `OVOLO`, `IMMIX`, `GYPPY`, `JEEZE` (8529 solutions)
- `SPANK`: `CWTCH`, `GRUFF`, `BOMBO`, `VIVID`, `JEEZE`, `XYLYL` (5 solutions)
- `SPARK`: `CWTCH`, `BOMBO`, `FLUFF`, `GYNNY`, `VIVID`, `JEEZE` (3 solutions)
- `SPASM`: `CWTCH`, `BROOK`, `FLUFF`, `GYNNY`, `VIVID`, `JEEZE` (37 solutions)
- `SPEED`: `CWTCH`, `ABAKA`, `ZORRO`, `FLUFF`, `GYNNY`, `IMMIX` (42 solutions)
- `SPELL`: `CRWTH`, `KUDZU`, `GYNNY`, `BOFFO`, `IMMIX`, `QAJAQ` (2 solutions)
- `SPEND`: `GRRRL`, `CWTCH`, `YUKKY`, `BOFFO`, `IMMIX`, `QAJAQ` (1 solution)
- `SPERM`: `CWTCH`, `VILLI`, `KUDZU`, `GYNNY`, `BOFFO`, `QAJAQ` (1 solution)
- `SPILL`: `CRWTH`, `KUDZU`, `EXEEM`, `GYNNY`, `BOFFO`, `QAJAQ` (26 solutions)
- `SPINY`: `GRRRL`, `CWTCH`, `KUDZU`, `EXEEM`, `BOFFO`, `QAJAQ` (2 solutions)
- `SPOOF`: `CRWTH`, `BELEE`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (1061 solutions)
- `SPOOK`: `CRWTH`, `ANANA`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (1047 solutions)
- `SPOOL`: `CWTCH`, `BEVER`, `KUDZU`, `GYNNY`, `IMMIX`, `QAJAQ` (40 solutions)
- `SPOON`: `CRWTH`, `ABAKA`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (312 solutions)
- `SPREE`: `CWTCH`, `AGAMA`, `KUDZU`, `JINNI`, `BOFFO`, `XYLYL` (24 solutions)
- `SPRIG`: `CWTCH`, `EEVEN`, `BOMBO`, `KUDZU`, `JAFFA`, `XYLYL` (5 solutions)
- `SPUNK`: `CRWTH`, `AGAMA`, `BOFFO`, `VIVID`, `JEEZE`, `XYLYL` (287 solutions)
- `SPURN`: `CWTCH`, `AGAMA`, `BOFFO`, `VIVID`, `JEEZE`, `XYLYL` (25 solutions)
- `STAFF`: `GRRRL`, `BENNE`, `KUDZU`, `HYPHY`, `COCCO`, `IMMIX` (13 solutions)
- `STIFF`: `GRRRL`, `AJWAN`, `KUDZU`, `EXEEM`, `HYPHY`, `COCCO` (70 solutions)
- `STINT`: `GRRRL`, `EMCEE`, `KUDZU`, `BOFFO`, `HYPHY`, `QAJAQ` (14 solutions)
- `STOCK`: `GRRRL`, `BUNDU`, `FEEZE`, `HYPHY`, `IMMIX`, `QAJAQ` (1 solution)
- `STOOD`: `GRRRL`, `AJWAN`, `BUCKU`, `FEEZE`, `HYPHY`, `IMMIX` (19 solutions)
- `STUCK`: `GRRRL`, `ADDAX`, `MEZZE`, `JINNI`, `BOFFO`, `HYPHY` (136 solutions)
- `STUFF`: `GRRRL`, `ABAKA`, `COMMO`, `DEEVE`, `JINNI`, `HYPHY` (1629 solutions)
- `STUNK`: `GRRRL`, `ADDAX`, `BOFFO`, `CIVIC`, `HYPHY`, `JEEZE` (130 solutions)
- `STUNT`: `GRRRL`, `ABAKA`, `COMMO`, `FEEZE`, `VIVID`, `HYPHY` (388 solutions)
- `SUNNY`: `GRRRL`, `CWTCH`, `ABAKA`, `DEEVE`, `ZOPPO`, `IMMIX` (478 solutions)
- `SWARM`: `PHPHT`, `BLUFF`, `GYNNY`, `VIVID`, `COCCO`, `JEEZE` (13 solutions)
- `SWEEP`: `GRRRL`, `CHYND`, `TUKTU`, `BOFFO`, `IMMIX`, `QAJAQ` (13 solutions)
- `SWEET`: `GRRRL`, `AMMAN`, `KUDZU`, `BOFFO`, `CIVIC`, `HYPHY` (56 solutions)
- `SWELL`: `PHPHT`, `AFARA`, `BOMBO`, `KUDZU`, `GYNNY`, `CIVIC` (58 solutions)
- `SWIFT`: `GRRRL`, `ANANA`, `KUDZU`, `EXEEM`, `HYPHY`, `COCCO` (38 solutions)
- `SWILL`: `PHPHT`, `AFARA`, `JEMBE`, `KUDZU`, `GYNNY`, `COCCO` (319 solutions)
- `SWING`: `PHPHT`, `AFARA`, `JEMBE`, `KUDZU`, `COCCO`, `XYLYL` (228 solutions)
- `SWIRL`: `PHPHT`, `EMCEE`, `KUDZU`, `GYNNY`, `BOFFO`, `QAJAQ` (8 solutions)
- `SWOON`: `GRRRL`, `ABACA`, `DEEVE`, `TUKTU`, `HYPHY`, `IMMIX` (2040 solutions)
- `SWOOP`: `GRRRL`, `BENNE`, `MYTHY`, `KUDZU`, `CIVIC`, `JAFFA` (30 solutions)
- `SWORD`: `PHPHT`, `ABACA`, `FLUFF`, `GYNNY`, `IMMIX`, `JEEZE` (113 solutions)
- `SWORN`: `PHPHT`, `ABACA`, `JUGUM`, `FEEZE`, `VIVID`, `XYLYL` (31 solutions)
- `SWUNG`: `PHPHT`, `ABACA`, `FEMME`, `ZORRO`, `VIVID`, `XYLYL` (1467 solutions)
- `SYNOD`: `GRRRL`, `PHPHT`, `BUCKU`, `FEEZE`, `IMMIX`, `QAJAQ` (1 solution)
- `TEETH`: `GRRRL`, `SKYFS`, `BUNDU`, `CIVIC`, `ZOPPO`, `QAJAQ` (36 solutions)
- `TENET`: `GRRLS`, `BOMBO`, `KUDZU`, `CIVIC`, `HYPHY`, `QAJAQ` (994 solutions)
- `TENSE`: `GRRRL`, `BOMBO`, `KUDZU`, `CIVIC`, `HYPHY`, `QAJAQ` (13 solutions)
- `TEPEE`: `GRRRL`, `BUNDH`, `SKYFS`, `COCCO`, `IMMIX`, `QAJAQ` (163 solutions)
- `THEFT`: `GRRRL`, `SYNDS`, `BUCKU`, `ZOPPO`, `IMMIX`, `QAJAQ` (8 solutions)
- `THEME`: `GRRRL`, `SKYFS`, `BUNDU`, `CIVIC`, `ZOPPO`, `QAJAQ` (5 solutions)
- `THUMB`: `GRRRL`, `ENDED`, `SKYFS`, `CIVIC`, `ZOPPO`, `QAJAQ` (88 solutions)
- `THUMP`: `GRRRL`, `BWANA`, `SKYFS`, `VIVID`, `COCCO`, `JEEZE` (49 solutions)
- `TIMID`: `GRRRL`, `ABAKA`, `JUJUS`, `FEEZE`, `HYPHY`, `COCCO` (823 solutions)
- `TOOTH`: `GRRRL`, `SKYFS`, `BUNDU`, `CEZVE`, `IMMIX`, `QAJAQ` (118 solutions)
- `TWEED`: `GRRRL`, `ABAKA`, `COMMO`, `SUSUS`, `JINNI`, `HYPHY` (1098 solutions)
- `TWEET`: `GRRLS`, `AMMAN`, `KUDZU`, `BOFFO`, `CIVIC`, `HYPHY` (5632 solutions)
- `TWIST`: `GRRRL`, `ANANA`, `KUDZU`, `EXEEM`, `BOFFO`, `HYPHY` (92 solutions)
- `TWIXT`: `GRRLS`, `BAJAN`, `FEMME`, `KUDZU`, `HYPHY`, `COCCO` (10821 solutions)
- `UNCUT`: `GRRLS`, `BOFFO`, `MAMMA`, `VIVID`, `HYPHY`, `JEEZE` (1449 solutions)
- `UNDID`: `GRRRL`, `CWTCH`, `SKYFS`, `BOMBO`, `PEEPE`, `QAJAQ` (19 solutions)
- `UNDUE`: `GRRRL`, `CWTCH`, `SKYFS`, `ZOPPO`, `IMMIX`, `QAJAQ` (8 solutions)
- `UNWED`: `GRRRL`, `PHPHT`, `SKYFS`, `BOMBO`, `CIVIC`, `QAJAQ` (4 solutions)
- `UNZIP`: `GRRRL`, `CWTCH`, `SKYFS`, `DEEVE`, `BOMBO`, `QAJAQ` (2 solutions)
- `USURP`: `CWTCH`, `ABAKA`, `DOGGO`, `JINNI`, `FEEZE`, `XYLYL` (546 solutions)
- `VENUE`: `GRRRL`, `CWTCH`, `SKYFS`, `DABBA`, `ZOPPO`, `IMMIX` (9 solutions)
- `VERGE`: `CWTCH`, `ABAKA`, `SUSUS`, `JINNI`, `ZOPPO`, `XYLYL` (3707 solutions)
- `VERSE`: `CWTCH`, `ABAKA`, `FLUFF`, `GYNNY`, `ZOPPO`, `IMMIX` (293 solutions)
- `VERVE`: `CWTCH`, `ABAKA`, `DOGGO`, `SUSUS`, `JINNI`, `XYLYL` (25156 solutions)
- `VIGIL`: `CRWTH`, `SKYFS`, `BUNDU`, `EXEEM`, `ZOPPO`, `QAJAQ` (605 solutions)
- `VISIT`: `GRRRL`, `AJWAN`, `EMCEE`, `KUDZU`, `BOFFO`, `HYPHY` (130 solutions)
- `VIVID`: `CRWTH`, `ABAKA`, `EXEEM`, `FLUFF`, `GYNNY`, `ZOPPO` (408177 solutions)
- `WACKY`: `GRRRL`, `PHPHT`, `BOMBO`, `SUSUS`, `FEEZE`, `VIVID` (82 solutions)
- `WEAVE`: `GRRRL`, `PHPHT`, `SKYFS`, `BUNDU`, `COCCO`, `IMMIX` (3 solutions)
- `WEDGE`: `PHPHT`, `ABACA`, `KININ`, `JUJUS`, `ZORRO`, `XYLYL` (7900 solutions)
- `WEEDY`: `GRRRL`, `PHPHT`, `ABAKA`, `COMMO`, `SUSUS`, `JINNI` (1080 solutions)
- `WHICH`: `GRRRL`, `AXMAN`, `SKYFS`, `DEEVE`, `BUTUT`, `ZOPPO` (167 solutions)
- `WHIFF`: `GRRRL`, `ABAMP`, `XYSTS`, `EEVEN`, `KUDZU`, `COCCO` (439 solutions)
- `WHOOP`: `GRRRL`, `ADMAN`, `SKYFS`, `BUTUT`, `CIVIC`, `JEEZE` (401 solutions)
- `WINCH`: `GRRRL`, `SKYFS`, `DEEVE`, `BUTUT`, `MAMMA`, `ZOPPO` (19 solutions)
- `WINDY`: `GRRRL`, `PHPHT`, `ABAKA`, `COMMO`, `JUJUS`, `FEEZE` (385 solutions)
- `WITCH`: `GRRRL`, `SKYFS`, `BUNDU`, `EXEEM`, `ZOPPO`, `QAJAQ` (2 solutions)
- `WOODY`: `GRRLS`, `PHPHT`, `BUCKU`, `ANANA`, `FEEZE`, `IMMIX` (3299 solutions)
- `WOOZY`: `GRRLS`, `PHPHT`, `BUCKU`, `ANANA`, `EXEEM`, `VIVID` (7829 solutions)
- `WORLD`: `PHPHT`, `ABACA`, `JUJUS`, `GYNNY`, `FEEZE`, `IMMIX` (374 solutions)
- `WOUND`: `GRRRL`, `PHPHT`, `SKYFS`, `ABACA`, `IMMIX`, `JEEZE` (9 solutions)
- `WRACK`: `PHPHT`, `BIFID`, `JUJUS`, `OVOLO`, `EXEEM`, `GYNNY` (579 solutions)
- `WRECK`: `PHPHT`, `AFLAJ`, `BOMBO`, `SUSUS`, `GYNNY`, `VIVID` (597 solutions)
- `WRING`: `PHPHT`, `ABAKA`, `COMMO`, `DEEVE`, `SUSUS`, `XYLYL` (1466 solutions)
- `WRONG`: `PHPHT`, `ABACA`, `FEMME`, `JUJUS`, `VIVID`, `XYLYL` (1267 solutions)
- `WRUNG`: `PHPHT`, `ABAKA`, `COMMO`, `FEEZE`, `VIVID`, `XYLYL` (3649 solutions)
- `WRYLY`: `PHPHT`, `ABACA`, `DOGGO`, `SUSUS`, `EXEEM`, `JINNI` (38475 solutions)

## Complete list of perfect solutions acievable with more than 6 guesses:

- `CIVIC`: `GRRRL`, `PHPHT`, `WYNNS`, `KUDZU`, `EXEME`, `BOFFO`, `QAJAQ`
- `CIVIC`: `GRRRL`, `PHPHT`, `WYNNS`, `KUDZU`, `EXEEM`, `BOFFO`, `QAJAQ`

# Remaining work

- Investigate the segmentation fault that occurs when disabling the vowel optimization but keeping multi-threading enabled.
The issue seemingly stems from unsafe access to multimap iterators.
- Optimize file I/O performance, which seemingly dominates the overall run time.
