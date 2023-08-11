#pragma once

// ==================== Common Utilities ==================== //
#include "Engine/Core/Core.h"
#include "Engine/Core/Log.h"

// =========================== Math ========================= //
#include "Engine/Math/Vec.h"

// ======================== Platform ======================== //
#ifdef EN_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

// ==================== Standard Libraries ==================== //
#include <cmath>
#include <chrono>
#include <memory>

// Data structures
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <functional>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <variant>
#include <stack>

// I/O
#include <iostream>
#include <fstream>
#include <filesystem>

// Concurrency
#include <thread>
#include <mutex>
#include <shared_mutex>