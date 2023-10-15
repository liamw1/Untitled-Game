#pragma once

namespace eng
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
  struct hash<eng::UID>
  {
    size_t operator()(const eng::UID& uuid) const
    {
      return uuid.hash();
    }
  };
}