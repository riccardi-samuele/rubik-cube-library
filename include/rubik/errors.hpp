#pragma once

#include <string>

namespace rubik {

enum class CubeErrorCode {
    None,
    InvalidStickerCount,
    InvalidColor,
    InvalidColorCount,
    InvalidCenterConfiguration,
    InvalidCornerPermutation,
    InvalidCornerOrientation,
    InvalidEdgePermutation,
    InvalidEdgeOrientation,
    InvalidParity,
    UnsolvableCube
};

struct CubeError {
    CubeErrorCode code = CubeErrorCode::None;
    std::string message;
};

} // namespace rubik
