main: ibuddy

CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -g
SHARED_LIB = libbuddy.so

%_d.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -fPIC -c $<

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $<

lib: binary_buddy.o bmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $(SHARED_LIB) binary_buddy.o bmalloc.o
	cp libbuddy.so ~/ioopm/inluppar/inlupp1/libbuddy.so

buddy: test.o binary_buddy.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o buddy.out test.o binary_buddy.o

buddy_debug: test_d.o binary_buddy_d.o
	$(CPP_COMPILER) $(CPP_FLAGS) -DDEBUG -o buddy.out test.o binary_buddy.o

clean:
	rm -rf *.o buddy.out $(SHARED_LIB)
