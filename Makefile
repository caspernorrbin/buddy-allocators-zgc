.PHONY: all clean

all: src tests benchmarks

src:
	$(MAKE) -C src

tests: src_build
	$(MAKE) -C tests

benchmarks: src_build
	$(MAKE) -C benchmarks/src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(MAKE) -C benchmarks/src clean