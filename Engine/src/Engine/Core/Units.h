#pragma once
#include "Casting.h"

using length_t = f32;
using seconds = f32;

// Literals
constexpr length_t operator"" _m(u64 x) { return eng::arithmeticUpcast<length_t>(x); }
constexpr length_t operator"" _m(fMax x) { return eng::arithmeticCastUnchecked<length_t>(x); }
constexpr seconds operator"" _s(u64 x) { return eng::arithmeticUpcast<seconds>(x); }
constexpr seconds operator"" _s(fMax x) { return eng::arithmeticCastUnchecked<seconds>(x); }