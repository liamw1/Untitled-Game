#pragma once
#include <type_traits>

template<typename T>
concept IntegerType = std::is_integral_v<T>;