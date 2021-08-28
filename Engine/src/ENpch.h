#pragma once

// ==================== Standard Libraries ==================== //
#include <memory>
#include <cmath>
#include <chrono>

// Data structures
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <functional>

// I/O
#include <iostream>
#include <fstream>

// ==================== Common Utilities ==================== //
#include "Engine/Core.h"
#include "Engine/Log.h"

#ifdef EN_PLATFORM_WINDOWS
  #include <Windows.h>
#endif