#include "rubik/cube.hpp"

#include <iostream>
#include <string>

int main()
{
    const std::string stickers =
        "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";

    const auto parsed = rubik::Cube::fromStickers(stickers);
    if (!parsed) {
        std::cerr << "invalid cube: " << parsed.error.message << "\n";
        return 1;
    }

    std::cout << "valid: true\n";
    std::cout << "solved: " << (parsed.cube.isSolved() ? "true" : "false") << "\n";
    return 0;
}
