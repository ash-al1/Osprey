#include "CircularBuffer.h"
#include <iostream>
#include <cassert>

using namespace std;

void Basics() {
	cout << "Basics:" << endl;

	CircularBuffer<float> buffer(5);

	assert(buffer.IsEmpty());
	assert(!buffer.IsFull());
	assert(buffer.Size() == 0);
	assert(buffer.Capacity() == 5);
	buffer.Push(1.0f);
	buffer.Push(2.0f);
	buffer.Push(3.0f);
	assert(buffer.Size() == 3);
	assert(!buffer.IsEmpty());
	assert(!buffer.IsFull());
	float value;
    assert(buffer.Pop(value));
    assert(value == 1.0f);
    assert(buffer.Size() == 2);
    assert(buffer.Pop(value));
    assert(value == 2.0f);
    assert(buffer.Size() == 1);

    std::cout << "   PASSED" << std::endl;
}

void Overflow() {
    std::cout << "Overflow" << std::endl;

    CircularBuffer<int> buffer(3);

    buffer.Push(1);
    buffer.Push(2);
    buffer.Push(3);
    assert(buffer.IsFull());
    assert(buffer.Size() == 3);
    buffer.Push(4);
    assert(buffer.Size() == 3);
    int value;
    assert(buffer.Pop(value));
    assert(value == 2);
    assert(buffer.Pop(value));
    assert(value == 3);
    assert(buffer.Pop(value));
    assert(value == 4);
    assert(buffer.IsEmpty());
    std::cout << "   PASSED" << std::endl;
}

void Latest() {
    std::cout << "Latest" << std::endl;

    CircularBuffer<float> buffer(5);

    for (int i = 1; i <= 4; ++i) {
        buffer.Push(float(i));
    }
    float data[4];
    buffer.CopyLatest(data, 4);
    for (int i = 0; i < 4; ++i) {
        assert(data[i] == float(i + 1));
        std::cout << "data[" << i << "] = " << data[i] << std::endl;
    }
    float big_data[10];
    buffer.CopyLatest(big_data, 10);
    for (int i = 0; i < 4; ++i) {
        assert(big_data[i] == float(i + 1));
    }
    for (int i = 4; i < 10; ++i) {
        assert(big_data[i] == 0.0f);
    }
    std::cout << "   PASSED" << std::endl;
}

int main() {
    std::cout << "=== CircularBuffer Test ===" << std::endl;
    try {
        Basics();
        Overflow();
        Latest();
        std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return -1;
    }
}
