#pragma once

namespace eng
{
  /*
    A unique 128-bit identifier.
  */
  class UID
  {
    u64 m_LowerUID;
    u64 m_UpperUID;

  public:
    UID();

    std::string toString();

    size_t hash() const;
  };
}

namespace std
{
  template<>
  struct hash<eng::UID>
  {
    size_t operator()(const eng::UID& uuid) const
    {
      return uuid.hash();
    }
  };
}