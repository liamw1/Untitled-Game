#pragma once
#include "Engine/Core/FixedWidthTypes.h"

namespace eng::debug
{
  class CopyMoveTest
  {
    struct Counters
    {
      std::atomic<i32> moves = 0;
      std::atomic<i32> copies = 0;

      ~Counters();
    };

    std::shared_ptr<Counters> m_Counters;

  public:
    CopyMoveTest();
    ~CopyMoveTest();

    CopyMoveTest(CopyMoveTest&& other) noexcept;
    CopyMoveTest& operator=(CopyMoveTest&& other) noexcept;

    CopyMoveTest(const CopyMoveTest& other);
    CopyMoveTest& operator=(const CopyMoveTest& other);
  };
}