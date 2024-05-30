.PHONY: all clean

all: src_build test_build benchmarks_build

src_build:
	$(MAKE) -C src

test_build: src_build
	$(MAKE) -C tests

benchmarks_build: src_build
	$(MAKE) -C benchmarks/src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(MAKE) -C benchmarks/src clean