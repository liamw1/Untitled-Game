#pragma once

// ================= Preprocessor Definitions ================= //
#define NOMINMAX
#define GLM_ENABLE_EXPERIMENTAL
#define _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS

// ========================= Platform ========================= //
#include "Engine/Core/PlatformDetection.h"

#ifdef ENG_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

// ==================== Standard Libraries ==================== //
#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <compare>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

// Data structures
#include <array>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Concurrency
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>

// I/O
#include <filesystem>
#include <fstream>
#include <iostream>

// Math
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <random>

// Wrappers
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <variant>