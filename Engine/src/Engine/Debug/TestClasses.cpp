#include "ENpch.h"
#include "TestClasses.h"

namespace Engine::Debug
{
  CopyMoveTest::CopyMoveTest()
    : m_Counters(std::make_shared<Counters>())
  {
    EN_CORE_TRACE("Default constructor.");
  }

  CopyMoveTest::~CopyMoveTest()
  {
    EN_CORE_TRACE("Destructor.");
  }

  CopyMoveTest::CopyMoveTest(CopyMoveTest&& other) noexcept
    : m_Counters(std::move(other.m_Counters))
  {
    m_Counters->moves++;
    EN_CORE_INFO("Move constructor.");
  }

  CopyMoveTest& CopyMoveTest::operator=(CopyMoveTest&& other) noexcept
  {
    m_Counters = std::move(other.m_Counters);
    m_Counters->moves++;
    EN_CORE_INFO("Move assignment.");
    return *this;
  }

  CopyMoveTest::CopyMoveTest(const CopyMoveTest& other)
    : m_Counters(other.m_Counters)
  {
    m_Counters->copies++;
    EN_CORE_WARN("Copy constructor!");
  }

  CopyMoveTest& CopyMoveTest::operator=(const CopyMoveTest& other)
  {
    m_Counters = other.m_Counters;
    m_Counters->copies++;
    EN_CORE_WARN("Copy assignment!");
    return *this;
  }
}