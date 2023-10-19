#pragma once
#include "Engine/Core/Concepts.h"

namespace eng
{
  template<typename T>
  using removeConstFromReference = std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>;

  template<typename T>
  using removeConst = std::conditional_t<std::is_reference_v<T>, removeConstFromReference<T>, std::remove_const_t<T>>;

  template<std::copyable T>
  constexpr T clone(const T& obj) { return obj; }

  template<MemberFunction F, Pointer P>
  auto bindMemberFunction(F memberFunction, P objectPointer)
  {
    return [memberFunction, objectPointer](auto&&... args)
    {
      return std::invoke(memberFunction, objectPointer, std::forward<decltype(args)>(args)...);
    };
  }
}

// Detail
#define __OBJECT_TYPE std::remove_reference_t<decltype(*this)>
#define __CONST_OBJECT_POINTER static_cast<const __OBJECT_TYPE*>(this)
#define __RETURN_TYPE(functionName, ...) decltype(__CONST_OBJECT_POINTER->functionName(__VA_ARGS__))

/*
  Used for creating const and mutable overloads of a member function without code duplication.
  Only for use in the definition of the mutable version of the function.
  Equivalent to const_cast<NonConstReturnType>(static_cast<const ClassType*>(this)->functionName(args...)

  Example:
    class i32
    {
    public:
      int& get() { return ENG_MUTABLE_VERSION(get); }
      const int& get() const { return m_Value; }

    private:
      int m_Value;
    }

  Ideally, this wouldn't be a macro, but a function version of this is more verbose than manual casting.
  NOTE: If an argument of the function is a temporary, you must std::move the argument into the macro.
*/
#define ENG_MUTABLE_VERSION(functionName, ...) const_cast<::eng::removeConst<__RETURN_TYPE(functionName, __VA_ARGS__)>>(__CONST_OBJECT_POINTER->functionName(__VA_ARGS__))