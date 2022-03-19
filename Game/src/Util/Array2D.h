#pragma once

template<typename T, int N, int M = N>
using Array2D = std::array<std::array<T, M>, N>;