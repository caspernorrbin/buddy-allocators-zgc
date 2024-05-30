#!/bin/bash

total_time=0
ignored=0

# Run the program 12 times
for ((i=1; i<=12; i++))
do
    echo "Running iteration $i"
    # Run the program and extract the time from the output
    result=$(./benchmark_threads.out | grep "Concurrent took:" | awk '{print $3}')
    
    if [ $i -gt 2 ]; then
        # Add the time to the total
        total_time=$(echo "$total_time + $result" | bc)
    else
        ((ignored++))
    fi
done

# Calculate the average of the last 10 runs
average=$(echo "scale=4; $total_time / (12 - $ignored)" | bc)

echo "Average time of the last 10 runs: $average seconds"