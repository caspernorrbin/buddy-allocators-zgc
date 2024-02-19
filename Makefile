main: ibuddy

CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -g
CPP_FAST_FLAGS = -Wall -Wextra -std=c++14 -pedantic -O2
SHARED_LIB = libbuddy.so
CPP_UNIT = -lcppunit

%_d.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -fPIC -c $<

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $<

ilib: ibuddy.o ibmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) ibuddy.o ibmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

blib: binary_buddy.o bmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) binary_buddy.o bmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

buddy: test.o binary_buddy.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out test.o binary_buddy.o

buddy_debug: test_d.o binary_buddy_d.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out test.o binary_buddy.o

ibuddy: itest.o ibuddy.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out itest.o ibuddy.o

ibuddy_debug: itest_d.o ibuddy_d.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out itest.o ibuddy.o

test: buddy_test.o ibuddy.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o test.out buddy_test.o ibuddy.o $(CPP_UNIT)

profile:
	$(CPP_COMPILER) $(CPP_FAST_FLAGS) -pg ibuddy.cpp buddy_test.cpp -c
	$(CPP_COMPILER) ibuddy.o buddy_test.o -lm -pg $(CPP_UNIT) -o $@
	./$@
	gprof ./$@ > $@.txt
	rm gmon.out $@

clean:
	rm -rf *.o test.out buddy.out $(SHARED_LIB)
