#pragma once
#include "Engine/Core/Concepts.h"
#include <memory>

namespace Engine
{
  class MoveableFunction
  {
  public:
    template<Invocable<void> F>
    MoveableFunction(F&& f)
      : m_TypeErasedFunction(new FunctionModel<F>(std::move(f))) {}
  
  private:
    class FunctionConcept
    {
    public:
      virtual ~FunctionConcept() = default;
      virtual void call() = 0;
    };
  
    template<Invocable<void> F>
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