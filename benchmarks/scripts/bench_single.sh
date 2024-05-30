#!/bin/bash

# Check if the correct number of arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <N> <size>"
    exit 1
fi

# Extract arguments
N=$1    # Number of iterations
size=$2 # Size sent to the program

# Initialize sum of outputs
total_output=0

# Loop N times to run the program and accumulate outputs
for (( i=1; i<=$N; i++ ))
do
    # Run the program and capture the output
    output=$(./bench_single_alloc.out "$size")
    
    # Add the output to the total
    total_output=$((total_output + output))
done

# Calculate average output
average_output=$((total_output / N))

# Print the average output
echo "$average_output"