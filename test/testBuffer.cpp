#include "eirian/Buffer.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    eirian::Buffer buffer;

    buffer.append("abc", 3);
    buffer.append(std::string("def"));
    assert(buffer.readableBytes() == 6);

    buffer.retrieve(2);
    assert(buffer.readableBytes() == 4);

    std::string s = buffer.retrieveAllAsString();
    assert(s == "cdef");
    assert(buffer.readableBytes() == 0);

    buffer.append("xyz", 3);
    buffer.retrieve(10);
    assert(buffer.readableBytes() == 0);
    assert(buffer.retrieveAllAsString().empty());

    std::cout << "testBuffer passed\n";
    return 0;
}
