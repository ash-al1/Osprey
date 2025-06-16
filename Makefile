.PHONY: all clean

all:
	cmake -S . -B build
	cmake --build build -- -j$(shell nproc)

clean:
	rm -rf build

