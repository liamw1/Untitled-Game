#pragma once

namespace eng
{
  enum class AllocationPolicy
  {
    Deferred,
    ForOverwrite,
    DefaultInitialize
  };

  enum class SortPolicy
  {
    Ascending,
    Descending
  };
}