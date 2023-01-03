#ifndef CORE_DELEGATE_H
#define CORE_DELEGATE_H

template <typename T>
class Delegate;

/**
 * specialization for member functions
 *
 * \tparam T            class-type of the object who's member function to call
 * \tparam R            return type of the function that gets captured
 * \tparam params       variadic template list for possible arguments
 *                      of the captured function
 */
template <typename T, typename R, typename... Params>
class Delegate<R (T::*)(Params...)>
{
 public:
  typedef R (T::*FuncType)(Params...);

  Delegate(T& caller, FuncType func) : mCaller(caller), mFunc(func) {}

  R operator()(Params... args) const { return (mCaller.*mFunc)(args...); }

  bool operator==(const Delegate& other) const { return (&mCaller == &other.mCaller) && (mFunc == other.mFunc); }
  bool operator!=(const Delegate& other) const { return !((*this) == other); }

 private:
  T& mCaller;
  FuncType mFunc;
};

/**
 * specialization for const member functions
 */
template <typename T, typename R, typename... Params>
class Delegate<R (T::*)(Params...) const>
{
 public:
  typedef R (T::*FuncType)(Params...) const;

  Delegate(const T& caller, FuncType func) : mCaller(caller), mFunc(func) {}

  R operator()(Params... args) const { return (mCaller.*mFunc)(args...); }

  bool operator==(const Delegate& other) const { return (&mCaller == &other.mCaller) && (mFunc == other.mFunc); }
  bool operator!=(const Delegate& other) const { return !(*this == other); }

 private:
  const T& mCaller;
  FuncType mFunc;
};

/**
 * specialization for free functions
 *
 * \tparam R            return type of the function that gets captured
 * \tparam params       variadic template list for possible arguments
 *                      of the captured function
 */
template <typename R, typename... Params>
class Delegate<R (*)(Params...)>
{
 public:
  typedef R (*FuncType)(Params...);

  Delegate(FuncType func) : mFunc(func) {}

  R operator()(Params... args) const { return (*mFunc)(args...); }

  bool operator==(const Delegate& other) const { return mFunc == other.mFunc; }
  bool operator!=(const Delegate& other) const { return !((*this) == other); }

 private:
  FuncType mFunc;
};

template <typename T, typename F>
Delegate<F> MakeDelegate(T& obj, F func)
{
  return Delegate<F>(obj, func);
}

template <typename F>
Delegate<F> MakeDelegate(F func)
{
  return Delegate<F>(func);
}
#endif  // CORE_DELEGATE_H