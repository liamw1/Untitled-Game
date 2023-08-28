#pragma once
#include "Engine/Core/Concepts.h"

namespace Engine
{
  /*
    A class similar to std::function that performs type erasure on the given
    callable object given to the constructor. Unlike std::function, this class
    can handle move-only types, like std::packaged_task.

    C++23: Should be replaced by std::move_only_function
  */
  class MoveOnlyFunction
  {
  public:
    MoveOnlyFunction() = default;

    template<InvocableWithReturnType<void> F>
      requires Movable<F>
    MoveOnlyFunction(F&& f)
      : m_TypeErasedFunction(new FunctionModel<F>(std::move(f))) {}

    MoveOnlyFunction(MoveOnlyFunction&& other) noexcept = default;
    MoveOnlyFunction& operator=(MoveOnlyFunction&& other) noexcept = default;

    void operator()() { m_TypeErasedFunction->call(); }
  
  private:
    class FunctionConcept
    {
    public:
      virtual ~FunctionConcept() = default;
      virtual void call() = 0;
    };
  
    template<InvocableWithReturnType<void> F>
      requires Movable<F>
    class FunctionModel : public FunctionConcept
    {
    public:
      FunctionModel(F&& function)
        : m_Function(std::move(function)) {}
  
      void call() { m_Function(); }
  
    private:
      F m_Function;
    };

    MoveOnlyFunction(const MoveOnlyFunction& other) = delete;
    MoveOnlyFunction& operator=(const MoveOnlyFunction& other) = delete;
  
    std::unique_ptr<FunctionConcept> m_TypeErasedFunction;
  };
}