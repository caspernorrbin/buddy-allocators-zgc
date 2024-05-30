CPP_COMPILER = g++
CPP_FLAGS = -Wall -Wextra -std=c++14 -pedantic -O2
CPP_UNIT = -lcppunit

SRC_DIR = ../src
BBUDDY_SRC_FILES = $(SRC_DIR)/bbuddy.o $(SRC_DIR)/buddy_allocator.o 
BTBUDDY_SRC_FILES = $(SRC_DIR)/btbuddy.o $(SRC_DIR)/buddy_allocator.o 
IBUDDY_SRC_FILES = $(SRC_DIR)/ibuddy.o $(SRC_DIR)/buddy_allocator.o 

all: bbuddy btbuddy ibuddy btest btest bttest itest

bbuddy: btest.o $(BBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o bbuddy.out btest.o $(BBUDDY_SRC_FILES)

btbuddy: bttest.o $(BTBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o btbuddy.out bttest.o $(BTBUDDY_SRC_FILES)

ibuddy: itest.o $(IBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o ibuddy.out itest.o $(IBUDDY_SRC_FILES)

btest: bbuddy_test.o $(BBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o btest.out bbuddy_test.o $(BBUDDY_SRC_FILES) $(CPP_UNIT)

bttest: btbuddy_test.o $(BTBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o bttest.out btbuddy_test.o $(BTBUDDY_SRC_FILES) $(CPP_UNIT)

itest: ibuddy_test.o $(IBUDDY_SRC_FILES)
	$(CPP_COMPILER) $(CPP_FLAGS) -o itest.out ibuddy_test.o $(IBUDDY_SRC_FILES) $(CPP_UNIT)

%.o: %.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -fPIC -c $< -o $@

clean:
	rm -rf *.o *.out
