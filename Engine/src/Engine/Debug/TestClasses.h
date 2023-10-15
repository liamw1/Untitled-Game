#pragma once

namespace eng::debug
{
  class CopyMoveTest
  {
  public:
    CopyMoveTest();
    ~CopyMoveTest();

    CopyMoveTest(CopyMoveTest&& other) noexcept;
    CopyMoveTest& operator=(CopyMoveTest&& other) noexcept;

    CopyMoveTest(const CopyMoveTest& other);
    CopyMoveTest& operator=(const CopyMoveTest& other);

  private:
    struct Counters
    {
      std::atomic<int> moves = 0;
      std::atomic<int> copies = 0;

      ~Counters()
      {
        EN_CORE_INFO("{0} move constructors and {1} copy constructors were called during the object's lifetime", moves, copies);
      }
    };

    std::shared_ptr<Counters> m_Counters;
  };
}