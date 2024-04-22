main: bbuddy_debug

CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -ggdb
CPP_FAST_FLAGS = -Wall -Wextra -std=c++14 -pedantic -O2
SHARED_LIB = libbuddy.so
CPP_UNIT = -lcppunit

%_d.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -fPIC -c $<

%_f.o: %.cpp
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -fPIC -c $<

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $<

ilib: buddy_allocator.o ibuddy.o ibmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) buddy_allocator.o ibuddy.o ibmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

blib: buddy_allocator.o bbuddy.o bmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) buddy_allocator.o bbuddy.o bmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

btlib: buddy_allocator.o btbuddy.o btmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) buddy_allocator.o btbuddy.o btmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

bbuddy: btest.o bbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out btest.o bbuddy.o buddy_allocator.o

bbuddy_debug: btest_d.o bbuddy_d.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out btest.o bbuddy.o buddy_allocator.o

btbuddy: bttest.o btbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out bttest.o btbuddy.o buddy_allocator.o

btbuddy_debug: bttest_d.o btbuddy_d.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out bttest.o btbuddy.o buddy_allocator.o

ibuddy: itest.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out itest.o ibuddy.o buddy_allocator.o

ibuddy_debug: itest_d.o ibuddy_d.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out itest.o ibuddy.o buddy_allocator.o

btest: bbuddy_test.o bbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o btest.out bbuddy_test.o bbuddy.o buddy_allocator.o $(CPP_UNIT)

bttest: btbuddy_test.o btbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o bttest.out btbuddy_test.o btbuddy.o buddy_allocator.o $(CPP_UNIT)

itest: ibuddy_test.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o itest.out ibuddy_test.o ibuddy.o buddy_allocator.o $(CPP_UNIT)

benchmark_threads: benchmark_threads_f.o ibuddy_f.o bbuddy_f.o btbuddy_f.o buddy_allocator_f.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o benchmark_threads.out benchmark_threads.o ibuddy.o bbuddy.o btbuddy.o buddy_allocator.o

benchmark_threads_s: benchmark_threads.o ibuddy.o bbuddy.o btbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o benchmark_threads.out benchmark_threads.o ibuddy.o bbuddy.o btbuddy.o buddy_allocator.o

profile:
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -pg heap.cpp btbuddy.cpp bbuddy.cpp ibuddy.cpp buddy_allocator.cpp -c
	$(CPP_COMPILER) heap.cpp btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o -lm -pg $(CPP_UNIT) -o $@
	./$@ dist2.txt
	gprof ./$@ > $@.txt
	rm gmon.out $@

heap: heap.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(FAST_FLAGS) -o heap.out heap.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o

add_frees: add_frees.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o add_frees.out add_frees.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o

bench_single_alloc: bench_single_alloc_f.o btbuddy_f.o bbuddy_f.o ibuddy_f.o buddy_allocator_f.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o bench_single_alloc.out bench_single_alloc.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o

bench_allocs: bench_allocs_f.o btbuddy_f.o bbuddy_f.o ibuddy_f.o buddy_allocator_f.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o bench_allocs.out bench_allocs.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o

bench_page: bench_page_f.o btbuddy_f.o bbuddy_f.o ibuddy_f.o buddy_allocator_f.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o bench_page.out bench_page.o btbuddy.o bbuddy.o ibuddy.o buddy_allocator.o

clean:
	rm -rf *.o *.out $(SHARED_LIB)
