#include "ENpch.h"
#include "TestClasses.h"
#include "Engine/Core/Logger.h"

namespace eng::debug
{
  CopyMoveTest::Counters::~Counters()
  {
    ENG_CORE_INFO("{0} move constructors and {1} copy constructors were called during the object's lifetime", moves, copies);
  }

  CopyMoveTest::CopyMoveTest()
    : m_Counters(std::make_shared<Counters>())
  {
    ENG_CORE_TRACE("Default constructor.");
  }

  CopyMoveTest::~CopyMoveTest()
  {
    ENG_CORE_TRACE("Destructor.");
  }

  CopyMoveTest::CopyMoveTest(CopyMoveTest&& other) noexcept
    : m_Counters(std::move(other.m_Counters))
  {
    m_Counters->moves++;
    ENG_CORE_INFO("Move constructor.");
  }

  CopyMoveTest& CopyMoveTest::operator=(CopyMoveTest&& other) noexcept
  {
    m_Counters = std::move(other.m_Counters);
    m_Counters->moves++;
    ENG_CORE_INFO("Move assignment.");
    return *this;
  }

  CopyMoveTest::CopyMoveTest(const CopyMoveTest& other)
    : m_Counters(other.m_Counters)
  {
    m_Counters->copies++;
    ENG_CORE_WARN("Copy constructor!");
  }

  CopyMoveTest& CopyMoveTest::operator=(const CopyMoveTest& other)
  {
    m_Counters = other.m_Counters;
    m_Counters->copies++;
    ENG_CORE_WARN("Copy assignment!");
    return *this;
  }
}