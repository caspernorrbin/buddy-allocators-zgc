
# Buddy Allocators at Oracle

## Overview

This repository contains various buddy allocator strategy implementations, designed as part of a master's thesis project at Oracle. This repository contains code to adapt and evaluate different buddy allocator strategies for use within the Z Garbage Collector (ZGC) in the Java Virtual Machine (JVM). The primary focus is on enhancing memory allocation efficiency and minimizing fragmentation.

## Features

- Implementation of various buddy allocators:
  - Binary Buddy Allocator
  - Binary Tree Buddy Allocator
  - Inverse Buddy Allocator (iBuddy)
- Adaptations for use within ZGC.
- Performance evaluation tools.

## Getting Started

### Prerequisites

To build and run the project, ensure you have the following installed:

- A compatible C++ compiler (e.g., GCC, Clang)
- GNU Make

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/caspernorrbin/buddy-allocator-oracle.git
   cd buddy-allocator-oracle
   ```
2. Build the project using the provided Makefiles:
   ```bash
   make all
   ```

### Running the Tests
1.  To build the tests:
    ```bash
    make test
    ``` 
    
2.  To build various performance benchmarks:
    ```bash
    make benchmarks
    ``` 

To get a more detailed understanding of the various make targets, look in their respective Makefiles.

## Usage

The repository includes several examples demonstrating how to use the buddy allocators, found in `/tests`.

### Example: Basic Allocation

Here is a basic example of using the Binary Buddy Allocator:

```cpp
#include  "bbuddy.hpp"
BinaryBuddyAllocator<SmallSingleConfig> *buddy =
      BinaryBuddyAllocator<SmallSingleConfig>::create(nullptr, nullptr, 10, false);

int* allocation = buddy->allocate(sizeof(int));
buddy->deallocate(allocation);
```

## Performance Evaluation

Performance and memory efficiency are crucial metrics for the buddy allocators. The repository includes tools to measure:

-   Allocation and deallocation times.
-   Memory usage and fragmentation levels.

### Evaluation Methodology

1.  **Single Allocation/Deallocation Time**: Measures the time required for a single allocation or deallocation for various block sizes.
2.  **Contiguous Memory Block Allocation**: Allocates a large memory block using different block sizes and measures the total time taken.

The tools for performance evaluation can be found in the `benchmarks` directory.

## Acknowledgements

This project was developed as part of a master's thesis titled "Adapting and Evaluating Buddy Allocators for use Within ZGC" by Casper Norrbin at Uppsala University.

For more details, refer to the [completed thesis](https://github.com/caspernorrbin/master-thesis-oracle).