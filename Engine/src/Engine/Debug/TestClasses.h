#pragma once
#include "Engine/Core/Log/Log.h"

namespace eng::debug
{
  class CopyMoveTest
  {
    struct Counters
    {
      std::atomic<i32> moves = 0;
      std::atomic<i32> copies = 0;

      ~Counters() { ENG_CORE_INFO("{0} move constructors and {1} copy constructors were called during the object's lifetime", moves, copies); }
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