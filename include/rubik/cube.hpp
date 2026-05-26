#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include "rubik/errors.hpp"
#include "rubik/move.hpp"

namespace rubik {

class InvalidCube : public std::runtime_error {
public:
    explicit InvalidCube(const std::string& message);
};

class Cube;
struct CubeParseResult;

class Cube {
public:
    static constexpr std::size_t sticker_count = 54;
    using Stickers = std::array<char, sticker_count>;

    Cube();
    explicit Cube(const Stickers& stickers);

    static Cube solved();
    static Cube fromUncheckedStickers(const Stickers& stickers);
    static Cube fromString(std::string_view stickers);
    static CubeParseResult fromStickers(std::string_view stickers);
    static CubeParseResult fromStickers(const Stickers& stickers);
    static CubeError validateStickers(std::string_view stickers);
    static CubeError validateStickers(const Stickers& stickers);

    const Stickers& stickers() const;
    std::string toString() const;

    bool isSolved() const;
    bool isValid() const;

    void apply(Move move);
    void apply(const std::vector<Move>& moves);
    Cube moved(Move move) const;

    friend bool operator==(const Cube& lhs, const Cube& rhs) = default;

private:
    Stickers stickers_;
};

struct CubeParseResult {
    Cube cube = Cube::solved();
    CubeError error;

    bool ok() const;
    explicit operator bool() const;
};

CubeParseResult fromStickers(std::string_view stickers);
CubeParseResult fromStickers(const Cube::Stickers& stickers);
CubeError validateStickers(std::string_view stickers);
CubeError validateStickers(const Cube::Stickers& stickers);

} // namespace rubik
