#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <cstdint>
#include <memory>
#include <cstring>

template<typename T>
struct shared_arr : std::shared_ptr<T>
{
  uint32_t count;

  bool allocate(uint32_t cnt) // allocate cnt copies of T
  {
    std::shared_ptr<T>::reset(); // release old data
    if(cnt > 0) // if actually allocating data
      std::shared_ptr<T>::operator =(std::shared_ptr<T>(new T[cnt], std::default_delete<T[]>())); // allocate new data with auto deleter
    count = cnt; // save the number of copies created
    return true;
  }

  const shared_arr<T>& operator =(const shared_arr<T>& other)
  {
    std::shared_ptr<T>::operator =(other);
    count = other.count;
    return other;
  }

  T* operator =(T* ptr) // use external pointer data
  {
    std::shared_ptr<T>::operator =(std::shared_ptr<T>(ptr, [](T const*) { })); // use pointer but do not deallocate
    return ptr;
  }

  operator T*(void) const
    { return std::shared_ptr<T>::get(); } // get pointer to all data

  T& operator [](uint32_t num)
    { ASSERT(num < count); return std::shared_ptr<T>::get()[num]; } // get data

  T* operator +(uint32_t num) const
    { ASSERT(num < count); return std::shared_ptr<T>::get() + num; } // get pointer to data

  bool operator ==(const shared_arr<T>& ptr) // compare pointers first by size and then by data
  {
    return count == ptr.count &&
        (!count || std::memcmp(std::shared_ptr<T>::get(), ptr.get(), sizeof(T) * count) == SUCCESS);  // memory does match))
  }
};

#endif // SHAREDARRAY_H
