#pragma once
#include "Engine/Core/FixedWidthTypes.h"

namespace eng
{
  constexpr u8  u8Bit (i32 n) { return 1Ui8  << n; }
  constexpr u16 u16Bit(i32 n) { return 1Ui16 << n; }
  constexpr u32 u32Bit(i32 n) { return 1Ui32 << n; }
  constexpr u64 u64Bit(i32 n) { return 1Ui64 << n; }

  constexpr u64 bit (i32 n) { return u64Bit(n); }
  constexpr u64 pow2(i32 n) { return u64Bit(n); }
}