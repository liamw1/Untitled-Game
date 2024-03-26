#pragma once

namespace eng
{
  enum class StrongOrdering
  {
    Less = -1,
    Equal,
    Greater
  };

  enum class WeakOrdering
  {
    Less = -1,
    Equivalent,
    Greater
  };

  enum class PartialOrdering
  {
    Less = -1,
    Equivalent,
    Greater,
    Unordered
  };

  constexpr StrongOrdering resultOf(std::strong_ordering cmp)
  {
    if (cmp == std::strong_ordering::less)
      return StrongOrdering::Less;
    else if (cmp == std::strong_ordering::equal)
      return StrongOrdering::Equal;
    else
      return StrongOrdering::Greater;
  }

  constexpr WeakOrdering resultOf(std::weak_ordering cmp)
  {
    if (cmp == std::weak_ordering::less)
      return WeakOrdering::Less;
    else if (cmp == std::weak_ordering::equivalent)
      return WeakOrdering::Equivalent;
    else
      return WeakOrdering::Greater;
  }

  constexpr PartialOrdering resultOf(std::partial_ordering cmp)
  {
    if (cmp == std::partial_ordering::less)
      return PartialOrdering::Less;
    else if (cmp == std::partial_ordering::equivalent)
      return PartialOrdering::Equivalent;
    else if (cmp == std::partial_ordering::greater)
      return PartialOrdering::Greater;
    else
      return PartialOrdering::Unordered;
  }
}