#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#if defined(TARGET)
# include <RSPiX/BLUE/System.h>
#else
# include <cassert>
# define ASSERT(x) assert(x)
#endif

#include "sakarchive.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace Resources
{
  static std::unordered_map<const char*, SAKArchive> archives;

  static void normalize_path(char* path) noexcept;
}

template<typename T>
struct resource : std::shared_ptr<filedata_t>
{
  T* operator ->(void) const noexcept
  {
    ASSERT(*this); // must already be allocated
    return reinterpret_cast<T*>(const_cast<uint8_t*>(get()->rawdata.data()));
  }

  resource<T>& operator=(const std::shared_ptr<filedata_t>& other) noexcept
  {
    ASSERT(!*this); // must not be previously allocated
    std::shared_ptr<filedata_t>::operator =(other);
    if(!other->loaded)
    {
      static_cast<T*>(get())->Load(); // type T _must_ inherherit filedataio_t
      other->loaded = true;
    }
    return *this;
  }
};

template <class T>
bool rspGetResource(const char* archivename,        // In:  Archive to be used
                    const char* filename,           // In:  Resource name
                    resource<T>& res) noexcept      // Out: Resource data pointer
{
  auto architer = Resources::archives.find(archivename);
  if(architer == Resources::archives.end() ||
     !architer->second.fileExists(filename))
    return false;

  res = architer->second.getFile(filename);
  return res;
}

template <class T>
void rspReleaseResource(resource<T>& res) noexcept  // In:  Pointer to resource
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
