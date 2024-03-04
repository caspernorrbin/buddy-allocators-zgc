main: bbuddy_debug

CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -g
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

bbuddy: btest.o bbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out btest.o bbuddy.o buddy_allocator.o

bbuddy_debug: btest_d.o bbuddy_d.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out btest.o bbuddy.o buddy_allocator.o

ibuddy: itest.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out itest.o ibuddy.o buddy_allocator.o

ibuddy_debug: itest_d.o ibuddy_d.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out itest.o ibuddy.o buddy_allocator.o

btest: bbuddy_test.o bbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o btest.out bbuddy_test.o bbuddy.o buddy_allocator.o $(CPP_UNIT)

itest: ibuddy_test.o ibuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o itest.out ibuddy_test.o ibuddy.o buddy_allocator.o $(CPP_UNIT)

benchmark_threads: benchmark_threads_f.o ibuddy_f.o bbuddy_f.o buddy_allocator_f.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o benchmark_threads.out benchmark_threads.o ibuddy.o bbuddy.o buddy_allocator.o

benchmark_threads_s: benchmark_threads.o ibuddy.o bbuddy.o buddy_allocator.o
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -o benchmark_threads.out benchmark_threads.o ibuddy.o bbuddy.o buddy_allocator.o

profile:
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -pg ibuddy.cpp buddy_test.cpp -c
	$(CPP_COMPILER) ibuddy.o buddy_test.o buddy_allocator.o -lm -pg $(CPP_UNIT) -o $@
	./$@
	gprof ./$@ > $@.txt
	rm gmon.out $@

clean:
	rm -rf *.o *.out $(SHARED_LIB)
