#pragma once

// ========================= Logging ========================== //
#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

#include "Engine/Core/Core.h"
#include "Engine/Core/Log.h"

// ============================ Math ========================== //
#include "Engine/Math/Vec.h"

// ========================= Platform ========================= //
#ifdef EN_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

// ==================== Standard Libraries ==================== //
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

// Data structures
#include <array>
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// I/O
#include <filesystem>
#include <fstream>
#include <iostream>

// Concurrency
#include <atomic>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>