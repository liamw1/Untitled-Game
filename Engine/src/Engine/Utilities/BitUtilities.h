#pragma once

namespace eng
{
  constexpr uint8_t  u8Bit (int n) { return 1Ui8  << n; }
  constexpr uint16_t u16Bit(int n) { return 1Ui16 << n; }
  constexpr uint32_t u32Bit(int n) { return 1Ui32 << n; }
  constexpr uint64_t u64Bit(int n) { return 1Ui64 << n; }

  constexpr uint64_t bit (int n) { return u64Bit(n); }
  constexpr uint64_t pow2(int n) { return u64Bit(n); }
}