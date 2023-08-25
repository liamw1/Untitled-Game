#pragma once

namespace Engine
{
  /*
    A class similar to std::function that performs type erasure on the given
    callable object given to the constructor. Unlike std::function, this class
    can handle move-only types, like std::packaged_task.
  */
  class MoveableFunction
  {
  public:
    MoveableFunction() = default;

    template<InvocableWithReturnType<void> F>
      requires Movable<F>
    MoveableFunction(F&& f)
      : m_TypeErasedFunction(new FunctionModel<F>(std::move(f))) {}

    MoveableFunction(MoveableFunction&& other) noexcept = default;
    MoveableFunction& operator=(MoveableFunction&& other) noexcept = default;

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

    MoveableFunction(const MoveableFunction& other) = delete;
    MoveableFunction& operator=(const MoveableFunction& other) = delete;
  
    std::unique_ptr<FunctionConcept> m_TypeErasedFunction;
  };
}