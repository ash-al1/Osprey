CXX = g++
CXXFLAGS = -Wall -Wextra
LIBS = -luhd -lpthread
THREAD_LIBS = -lpthread
USRP_LIBS = UsrpController.cpp TestUsrpController.cpp
CIRC_LIBS = TestCircularBuffer.cpp
TARGET = TestUsrp
CIRC   = Buffer

all: cmake-build

cmake-build:
	rm -rf build
	cmake -S . -B build
	cmake --build build -- -j$(shell nproc)

usrp: $(TARGET)

buffer: $(CIRC)

$(TARGET): $(USRP_LIBS)
	$(CXX) $(CXXFLAGS) $(USRP_LIBS) -o $(TARGET) $(LIBS)

$(CIRC): $(CIRC_LIBS)
	$(CXX) $(CXXFLAGS) $(CIRC_LIBS) -o $(CIRC) $(THREAD_LIBS)

clean:
	rm -rf build
	rm -rf $(TARGET)
	rm -rf $(CIRC)

.PHONY: all cmake-build usrp buffer clean
