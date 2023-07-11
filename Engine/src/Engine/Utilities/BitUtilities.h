#pragma once

constexpr uint8_t bitUi8(int n) { return 1Ui8 << n; }
constexpr uint16_t bitUi16(int n) { return 1Ui16 << n; }
constexpr uint32_t bitUi32(int n) { return 1Ui32 << n; }
constexpr uint64_t bitUi64(int n) { return 1Ui64 << n; }

constexpr uint64_t bit(int n) { return bitUi64(n); }
constexpr uint64_t pow2(int n) { return bitUi64(n); }