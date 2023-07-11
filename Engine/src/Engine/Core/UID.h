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

    std::string toString();

    size_t hash() const;

  private:
    uint64_t m_LowerUID;
    uint64_t m_UpperUID;
  };
}

namespace std
{
  template<>
  struct hash<Engine::UID>
  {
    size_t operator()(const Engine::UID& uuid) const
    {
      return uuid.hash();
    }
  };
}