#pragma once
#include "Constraints.h"
#include "Engine/Core/Concepts.h"

namespace eng
{
  /*
    A class similar to std::function that performs type erasure on the given
    callable object given to the constructor. Unlike std::function, this class
    can handle move-only types, like std::packaged_task.

    C++23: Should be replaced by std::move_only_function
  */
  class MoveOnlyFunction : private NonCopyable
  {
  public:
    MoveOnlyFunction() = default;

    template<InvocableWithReturnType<void> F>
      requires std::movable<F>
    MoveOnlyFunction(F&& f)
      : m_TypeErasedFunction(new FunctionModel<F>(std::move(f))) {}

    void operator()() { m_TypeErasedFunction->call(); }
  
  private:
    class FunctionConcept
    {
    public:
      virtual ~FunctionConcept() = default;
      virtual void call() = 0;
    };
  
    template<InvocableWithReturnType<void> F>
      requires std::movable<F>
    class FunctionModel : public FunctionConcept
    {
    public:
      FunctionModel(F&& function)
        : m_Function(std::move(function)) {}
  
      void call() { m_Function(); }
  
    private:
      F m_Function;
    };
  
    std::unique_ptr<FunctionConcept> m_TypeErasedFunction;
  };
}