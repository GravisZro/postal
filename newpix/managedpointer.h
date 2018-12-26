#ifndef MANAGEDPOINTER_H
#define MANAGEDPOINTER_H

#include <cstdint>

extern uint64_t g_insert_count;
extern uint64_t g_erase_count;
extern uint64_t g_lookup_count;
extern uint64_t g_reset_count;
extern uint64_t g_validity_check_count;

#ifndef NEW_MANAGED_PTR
#include <set>

extern std::set<void*> g_all_pointers;


template<typename T>
class managed_ptr
{
public:
  managed_ptr(T* ptr = nullptr) noexcept
    : m_ptr(ptr)
  {
    if(m_ptr != nullptr)
      ++g_insert_count, g_all_pointers.insert(reinterpret_cast<void*>(ptr));
  }

  template<typename U>
  managed_ptr(const managed_ptr<U>& other)
    { m_ptr = other ? reinterpret_cast<T*>(other.pointer()) : nullptr; }

  void reset(void) noexcept { ++g_reset_count; m_ptr = nullptr; }

  static void destroy(T* ptr) noexcept
  {
    if(ptr != nullptr)
    {
      ++g_erase_count;
      if(g_all_pointers.erase(ptr) > 0)
        delete ptr;
    }
  }

  T& operator * (void) const noexcept { return *pointer(); }
  T* operator ->(void) const noexcept { return  pointer(); }

  bool operator <(const managed_ptr& other) const noexcept { return pointer() < other.pointer(); }
  bool operator <(T* other) const noexcept { return pointer() < other; }


  template<typename U>
  bool operator ==(U* other) const noexcept { return pointer() == reinterpret_cast<T*>(other); }
  template<typename U>
  bool operator !=(U* other) const noexcept { return pointer() != reinterpret_cast<T*>(other); }

  operator bool(void) const noexcept
  {
    ++g_validity_check_count;
    if(m_ptr != nullptr && // if have a pointer
       ++g_lookup_count, g_all_pointers.find(reinterpret_cast<void*>(pointer())) == g_all_pointers.end()) // AND pointer is NOT registered
      m_ptr = nullptr; // reset pointer
    return m_ptr != nullptr;
  }

  T* pointer(void) const noexcept { return m_ptr; }
private:
  mutable T* m_ptr;
};
#else
#include <set>
#include <algorithm>
#include <memory>

#include <put/object.h>

extern bool operator ==(const std::shared_ptr<Object> shobj, Object *const obj);

extern std::set<std::shared_ptr<Object>> g_all_pointers;


class managed_object_ptr
{
public:
  managed_object_ptr(Object* ptr) noexcept;
  operator Object*(void) const noexcept;
  operator bool(void) const noexcept;

  void reset(void) noexcept;

  static void destroy(Object* ptr) noexcept;

private:
  std::weak_ptr<Object> m_ptr;
};


template<typename T>
class managed_ptr : public managed_object_ptr
{
public:
  template<typename U = T>
  managed_ptr(const managed_ptr<U>& other) : managed_object_ptr(other.operator Object*()) { }

  template<typename U = T>
  managed_ptr(U* ptr = nullptr) noexcept : managed_object_ptr(static_cast<Object*>(ptr)) { }

  template<typename U>
  bool operator ==(const managed_ptr<U>& other) const noexcept { return operator Object*() == other.operator Object*(); }
  template<typename U>
  bool operator !=(const managed_ptr<U>& other) const noexcept { return !operator ==(other); }

  operator T*(void) const noexcept { return static_cast<T*>(operator Object*()); }
  T* operator ->(void) const noexcept { return operator T*(); }
  T& operator * (void) const noexcept { return *operator T*(); }

  operator uintptr_t(void) const noexcept { return uintptr_t(operator Object*()); }
  bool operator <(const managed_ptr<T>& other) const noexcept { return operator Object*() < other.operator Object*(); }
};
#endif

#endif // MANAGEDPOINTER_H
