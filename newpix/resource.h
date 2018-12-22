#ifndef RESOURCE_H
#define RESOURCE_H

#include <memory>

#include "sakarchive.h"

template<typename T> // type T must inherit filedata_t
struct Resource : shared_arr<T>
{
  T& operator ->(void) const noexcept
  {
    ASSERT(std::shared_ptr<T>::operator bool()); // must already be allocated
    return *std::shared_ptr<T>::get()->data;
  }

  managed_ptr<filedata_t>& operator =(managed_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    std::shared_ptr<T>::get()->load();
    return other;
  }
};


#endif // RESOURCE_H
