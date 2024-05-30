CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -O2

all: blib ilib btlib

blib: buddy_allocator.o bbuddy.o bmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o blib.so buddy_allocator.o bbuddy.o bmalloc.o

ilib: buddy_allocator.o ibuddy.o ibmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o ilib.so buddy_allocator.o ibuddy.o ibmalloc.o

btlib: buddy_allocator.o btbuddy.o btmalloc.o
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o btlib.so buddy_allocator.o btbuddy.o btmalloc.o

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $< -o $@

clean:
	rm -rf *.o *.so
