#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <put/cxxutils/posix_helpers.h>
#include <memory>


template<typename T>
class shared_arr : public std::shared_ptr<T>
{
private:
  uint32_t m_count;

  template<typename U>
  static void faux_deleter(U const*) noexcept { }

public:
  uint32_t size(void) const noexcept { return m_count; }
  void setSize(uint32_t cnt) noexcept { m_count = cnt; }

  bool allocate(uint32_t cnt) noexcept // allocate cnt copies of T
  {
    std::shared_ptr<T>::reset(); // release old data
    if(cnt > 0) // if actually allocating data
      std::shared_ptr<T>::operator =(std::shared_ptr<T>(new T[cnt], std::default_delete<T[]>())); // allocate new data with auto deleter
    setSize(cnt); // save the number of copies created
    return true;
  }

  const shared_arr<T>& operator =(const shared_arr<T>& other) noexcept
  {
    std::shared_ptr<T>::operator =(other);
    setSize(other.size());
    return other;
  }

  T* operator =(T* ptr) noexcept // use external pointer data
  {
    std::shared_ptr<T>::operator =(std::shared_ptr<T>(ptr, faux_deleter<T>)); // use pointer but do not deallocate
    return ptr;
  }

  operator T*(void) const noexcept
    { return std::shared_ptr<T>::get(); } // get pointer to all data

  T* operator +(uint32_t num) const noexcept
    { ASSERT(num < size()); return std::shared_ptr<T>::get() + num; } // get pointer to data

  bool operator ==(const shared_arr<T>& ptr) noexcept // compare pointers first by size and then by data
  {
    return size() == ptr.size() &&
        (!size() || posix::memcmp(std::shared_ptr<T>::get(), ptr.get(), sizeof(T) * size()) == posix::success_response);  // memory does match))
  }
};

#endif // SHAREDARRAY_H
