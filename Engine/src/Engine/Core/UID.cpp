#include "ENpch.h"
#include "UID.h"
#include <random>

namespace Engine
{
  static std::random_device s_RandomDevice;
  static std::mt19937_64 s_RandomEngine(s_RandomDevice());
  static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

  UID::UID()
    : m_LowerUID(s_UniformDistribution(s_RandomEngine)),
      m_UpperUID(s_UniformDistribution(s_RandomEngine))
  {
  }
}