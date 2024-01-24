#include "ENpch.h"
#include "UID.h"

namespace eng
{
  static std::random_device s_RandomDevice;
  static std::mt19937_64 s_RandomEngine(s_RandomDevice());
  static std::uniform_int_distribution<u64> s_UniformDistribution;

  UID::UID()
    : m_LowerUID(s_UniformDistribution(s_RandomEngine)),
      m_UpperUID(s_UniformDistribution(s_RandomEngine)) {}

  std::string UID::toString() const
  {
    static constexpr i32 numHexCharacters = 2 * sizeof(u64);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(numHexCharacters) << m_UpperUID;
    ss << std::setw(numHexCharacters) << m_LowerUID;
    return ss.str();
  }

  uSize UID::hash() const { return m_LowerUID + m_UpperUID; }
}