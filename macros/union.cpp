#include <iostream>
int main() {
    union {
        uint32_t i;
        uint8_t c[4];
    } testUnion;

    testUnion.i = 0x01020304;

    if (testUnion.c[0] == 0x01) {
        std::cout << "Big Endian" << std::endl;
    } else if (testUnion.c[0] == 0x04) {
        std::cout << "Little Endian" << std::endl;
    } else {
        std::cout << "Unknown Endian" << std::endl;
    }

    return 0;
}
