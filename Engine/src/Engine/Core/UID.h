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

    uSize hash() const;
  };
}

namespace std
{
  template<>
  struct hash<eng::UID>
  {
    uSize operator()(const eng::UID& uuid) const
    {
      return uuid.hash();
    }
  };
}