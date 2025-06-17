.PHONY: all clean

all:
	rm -rf build
	cmake -S . -B build
	cmake --build build -- -j$(shell nproc)
