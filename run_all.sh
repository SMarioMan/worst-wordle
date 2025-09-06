#!/bin/bash

# Ensure the output directory exists
mkdir -p build
mkdir -p output

# Function to run an evaluation, report stats, and calculate speedup
run_ablation_study() {
    local label="$1"
    local flags="$2"
    local report=""

    report+="Building and running for: ${label}\n"

    # Build and run
    g++ -std=c++17 -Wall -Wextra -pedantic -O3 -march=native -flto ${flags} src/worst-wordle.cpp -o "build/worst-wordle-${label}.out"

    # Get start time
    start_time=$(date +%s.%N)

    "./build/worst-wordle-${label}.out" > "output/output-${label}.txt"

    # Get end time
    end_time=$(date +%s.%N)

    rm "./build/worst-wordle-${label}.out"

    # Calculate and store run time
    runtime=$(echo "${end_time} - ${start_time}" | bc)
    report+="Run time: ${runtime} seconds\n"

    # Store stats
    grep ':' "output/output-${label}.txt" | grep ',' | sort > "output/output-sorted-${label}.txt"
    num_unique_answers=$(cut -d: -f1 "output/output-sorted-${label}.txt" | sort | uniq | wc -l)
    num_solutions=$(wc -l "output/output-sorted-${label}.txt" | awk '{print $1}')
    num_duplicates=$(grep ':' "output/output-sorted-${label}.txt" | grep ',' | sort | uniq -d | wc -l)

    report+="Number of unique answers: ${num_unique_answers}\n"
    report+="Number of solutions: ${num_solutions}\n"
    report+="Number of duplicate solutions: ${num_duplicates}\n"

    # Calculate and report speedup against the fully optimized run
    if [ "${label}" != "fully_optimized" ]; then
        fully_optimized_runtime=$(cat output/fully_optimized_runtime.txt)
        speedup=$(echo "scale=2; ${runtime} / ${fully_optimized_runtime}" | bc)
        report+="Speedup against fully optimized: ${speedup}x\n"
    else
        echo "${runtime}" > output/fully_optimized_runtime.txt
    fi

    report+="---\n"

    # Print accumulated report
    echo -e "$report"
}

# Run the fully optimized version first to establish a baseline
run_ablation_study "fully_optimized" ""

# Run each ablation study
# run_ablation_study "multithread_disabled" "-DDISABLE_MULTITHREAD_OPTIMIZATION"
# run_ablation_study "rarity_sort_disabled" "-DDISABLE_RARITY_SORT"
# run_ablation_study "vowel_optimization_disabled" "-DDISABLE_VOWEL_OPTIMIZATION -DDISABLE_MULTITHREAD_OPTIMIZATION"
# run_ablation_study "pruning_disabled" "-DDISABLE_PRUNING"
# run_ablation_study "permutation_dedup_disabled" "-DDISABLE_PERMUTATION_DEDUP"
# run_ablation_study "everything_disabled" "-DDISABLE_VOWEL_OPTIMIZATION -DDISABLE_MULTITHREAD_OPTIMIZATION -DDISABLE_PERMUTATION_DEDUP -DDISABLE_RARITY_SORT -DDISABLE_PRUNING"

# Wait for any parallel jobs to complete
wait

# Clean up temporary files
rm output/fully_optimized_runtime.txt
