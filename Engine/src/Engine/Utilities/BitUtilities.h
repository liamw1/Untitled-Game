#pragma once

namespace Engine
{
  constexpr uint8_t BitUi8(int n) { return 1Ui8 << n; }
  constexpr uint16_t BitUi16(int n) { return 1Ui16 << n; }
  constexpr uint32_t BitUi32(int n) { return 1Ui32 << n; }
  constexpr uint64_t BitUi64(int n) { return 1Ui64 << n; }

  constexpr uint64_t Bit(int n) { return BitUi64(n); }
  constexpr uint64_t Pow2(int n) { return BitUi64(n); }
}