#pragma once

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

// I/O
#include <iostream>
#include <fstream>
#include <filesystem>

// ==================== Common Utilities ==================== //
#include "Engine/Core/Core.h"
#include "Engine/Core/Log.h"
#include "Engine/Debug/Instrumentor.h"

#ifdef EN_PLATFORM_WINDOWS
  #include <Windows.h>
#endif