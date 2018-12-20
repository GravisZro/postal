#ifndef MANAGEDPOINTER_H
#define MANAGEDPOINTER_H

#include <set>
extern std::set<void*> g_all_pointers;

template<typename T>
class managed_ptr
{
public:
  managed_ptr(void) noexcept : m_ptr(nullptr) { }
  managed_ptr(T* ptr) noexcept : m_ptr(ptr) { g_all_pointers.insert(reinterpret_cast<void*>(ptr)); }

  template<typename U>
  managed_ptr(const managed_ptr<U>& other)
    { m_ptr = other ? reinterpret_cast<T*>(other.pointer()) : nullptr; }

  void reset(void) noexcept { m_ptr = nullptr; }

  void destroy(void) noexcept
  {
    g_all_pointers.erase(m_ptr);
    delete m_ptr;
    m_ptr = nullptr;
  }

  T& operator * (void) const noexcept { return *m_ptr; }
  T* operator ->(void) const noexcept { return  m_ptr; }

  bool operator <(const managed_ptr& other) const noexcept { return m_ptr < other.m_ptr; }
  bool operator <(T* other) const noexcept { return m_ptr < other; }


  template<typename U>
  bool operator ==(U* other) const noexcept { return m_ptr == reinterpret_cast<T*>(other); }
  template<typename U>
  bool operator !=(U* other) const noexcept { return m_ptr != reinterpret_cast<T*>(other); }

  operator bool(void) const noexcept
  {
    if(m_ptr != nullptr && // if have a pointer
       g_all_pointers.find(reinterpret_cast<void*>(m_ptr)) == g_all_pointers.end()) // AND pointer is NOT registered
      m_ptr = nullptr; // reset pointer
    return m_ptr != nullptr;
  }

  T* pointer(void) const noexcept { return m_ptr; }
private:
  mutable T* m_ptr;
};

#endif // MANAGEDPOINTER_H
