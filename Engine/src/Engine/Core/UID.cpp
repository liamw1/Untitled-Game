#include "ENpch.h"
#include "UID.h"

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

  std::string UID::toString()
  {
    static constexpr int numHexCharacters = 2 * sizeof(uint64_t);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(numHexCharacters) << m_UpperUID;
    ss << std::setw(numHexCharacters) << m_LowerUID;
    return ss.str();
  }

  size_t UID::hash() const
  {
    return m_LowerUID + m_UpperUID;
  }
}