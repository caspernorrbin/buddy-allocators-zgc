CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -ggdb

SRC_DIR = ../../src
SRC_FILES = $(SRC_DIR)/bbuddy.o $(SRC_DIR)/btbuddy.o $(SRC_DIR)/ibuddy.o $(SRC_DIR)/buddy_allocator.o

all: add_frees bench_allocs bench_page bench_single_alloc benchmark_threads heap

add_frees: add_frees.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o add_frees.out add_frees.o $(SRC_FILES)

bench_allocs: bench_allocs.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o bench_allocs.out bench_allocs.o $(SRC_FILES)

bench_page: bench_page.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o bench_page.out bench_page.o $(SRC_FILES)

bench_single_alloc: bench_single_alloc.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o bench_single_alloc.out bench_single_alloc.o $(SRC_FILES)

benchmark_threads: benchmark_threads.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o benchmark_threads.out benchmark_threads.o $(SRC_FILES)

heap: heap.o $(SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o heap.out heap.o $(SRC_FILES)

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $< -o $@

clean:
	rm -rf *.o *.out
