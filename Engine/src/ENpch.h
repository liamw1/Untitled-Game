#pragma once

// ========================= Logging ========================== //
#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

#include "Engine/Core/Log/Log.h"
#include "Engine/Debug/Assert.h"

// ============================ Math ========================== //
#include "Engine/Math/Vec.h"

// ========================= Platform ========================= //
#ifdef ENG_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

// ==================== Standard Libraries ==================== //
#include <algorithm>
#include <chrono>
#include <concepts>
#include <compare>
#include <cstdint>
#include <initializer_list>
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
#include <memory>
#include <optional>
#include <variant>