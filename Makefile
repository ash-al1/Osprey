CXX = g++
CXXFLAGS = -Wall -Wextra
LIBS = -luhd -lpthread
USRP_LIBS = UsrpController.cpp UsrpController.h TestUsrpController.cpp
TARGET = TestUsrp

all: cmake-build

cmake-build:
	rm -rf build
	cmake -S . -B build
	cmake --build build -- -j$(shell nproc)

usrp: $(TARGET)

$(TARGET): $(USRP_LIBS)
	$(CXX) $(CXXFLAGS) $(LIBS) -o $(TARGET) $(USRP_LIBS)

clean:
	rm -rf build
	rm -rf $(TARGET)

.PHONY: all cmake-build usrp clean
