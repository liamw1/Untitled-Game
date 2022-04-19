#pragma once

namespace Engine
{
  /*
    A unique 128-bit identifier.
  */
  class UID
  {
  public:
    UID();

  private:
    uint64_t m_LowerUID;
    uint64_t m_UpperUID;

    friend struct std::hash<UID>;
  };
}

namespace std
{
  template<>
  struct hash<Engine::UID>
  {
    size_t operator()(const Engine::UID& uuid) const
    {
      return hash<uint64_t>()(uuid.m_LowerUID + uuid.m_UpperUID);
    }
  };
}