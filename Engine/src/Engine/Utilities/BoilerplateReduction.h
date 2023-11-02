#pragma once

/*
  Used for creating const and mutable overloads of a member function without code duplication.
  Only for use in the definition of the mutable version of the function.
  Equivalent to 'return const_cast<NonConstReturnType>(static_cast<const ClassType*>(this)->functionName(args...)'

  Example:
    class i32
    {
      i32 m_Value;

    public:
      i32& get() { ENG_MUTABLE_VERSION(get); }
      const i32& get() const { return m_Value; }
    }

  Ideally, this wouldn't be a macro, but a function version of this is more verbose than manual casting.
  NOTE: If an argument of the function is a temporary, you must std::move the argument into the macro.
*/
#define ENG_MUTABLE_VERSION(functionName, ...)                                                        \
using ObjectType = std::remove_reference_t<decltype(*this)>;                                          \
const ObjectType* constObjectPointer = static_cast<const ObjectType*>(this);                          \
                                                                                                      \
using ReturnType = decltype(constObjectPointer->functionName(__VA_ARGS__));                           \
if constexpr (std::is_reference_v<ReturnType>)                                                        \
  return const_cast<std::remove_cvref_t<ReturnType>&>(constObjectPointer->functionName(__VA_ARGS__)); \
else                                                                                                  \
  return constObjectPointer->functionName(__VA_ARGS__);                                               \
static_assert(true)



/*
  Automatically defines all versions of begin/end iterators based on a stored container.
  Container type must have all iterators defined.

  Example:
    template<typenameT>
    class VectorWrapper
    {
      std::vector<T> m_Data;
    
    public:
      ENG_DEFINE_ITERATORS(m_Data);
    
      VectorWrapper() = default;
    
      bool remove(iterator removalPosition);
    }
*/
#define ENG_DEFINE_ITERATORS(container)           __DEFINE_ITERATORS(container,)
#define ENG_DEFINE_CONSTEXPR_ITERATORS(container) __DEFINE_ITERATORS(container, constexpr)

// Detail
#define __DEFINE_ITERATORS(container, keyword)                                  \
using iterator               = decltype(container.end());                       \
using const_iterator         = decltype(container.cend());                      \
using reverse_iterator       = decltype(container.rend());                      \
using const_reverse_iterator = decltype(container.crend());                     \
                                                                                \
keyword iterator               begin()         { return container.begin();   }  \
keyword iterator               end()           { return container.end();     }  \
keyword reverse_iterator       rbegin()        { return container.rbegin();  }  \
keyword reverse_iterator       rend()          { return container.rend();    }  \
                                                                                \
keyword const_iterator         begin()   const { return container.begin();   }  \
keyword const_iterator         end()     const { return container.end();     }  \
keyword const_reverse_iterator rbegin()  const { return container.rbegin();  }  \
keyword const_reverse_iterator rend()    const { return container.rend();    }  \
                                                                                \
keyword const_iterator         cbegin()  const { return container.cbegin();  }  \
keyword const_iterator         cend()    const { return container.cend();    }  \
keyword const_reverse_iterator crbegin() const { return container.crbegin(); }  \
keyword const_reverse_iterator crend()   const { return container.crend();   }  \
static_assert(true)