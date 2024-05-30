#!/bin/bash

run_version() {
    version="$1"
    for ((i = 4; i <= 18; i++)); do
        alloc=$((2 ** i))
        echo "" > "$version"_"$alloc".txt
    done
    echo "" > "$version"_32.txt
    echo "" > "$version"_64.txt

    for ((i = 4; i <= 18; i++)); do
        alloc=$((2 ** i))
        echo $alloc
        for ((j = 1; j <= 10000; j++)); do
            # Run the program and append the result to a temporary file
            result=$(./"bench_$version.out" "$alloc")
            #echo "$result" >> "$version"_
            #sum=$(awk "BEGIN {print $sum + $result; exit}")
            echo $result >> "$version"_"$alloc".txt
        done
    done
}

# Run each program with different allocators and save output to respective files
run_version "bt"
run_version "binary" 
run_version "ibuddy"
run_version "lazy"