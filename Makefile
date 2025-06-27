CXX = g++
CXXFLAGS = -Wall -Wextra
LIBS = -luhd -lpthread
USRP_LIBS = UsrpController.cpp TestUsrpController.cpp
TARGET = TestUsrp

all: cmake-build

cmake-build:
	rm -rf build
	cmake -S . -B build
	cmake --build build -- -j$(shell nproc)

usrp: $(TARGET)

$(TARGET): $(USRP_LIBS)
	$(CXX) $(CXXFLAGS) $(USRP_LIBS) -o $(TARGET) $(LIBS)

clean:
	rm -rf build
	rm -rf $(TARGET)

.PHONY: all cmake-build usrp clean
