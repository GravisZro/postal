#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "sakarchive.h"

namespace Resources
{
  static void normalize_path(std::string& path) noexcept;
}

template<typename T> // type T must inherit filedataio_t
struct Resource : std::shared_ptr<filedata_t>
{
  T& operator ->(void) const noexcept
  {
    ASSERT(*this); // must already be allocated
    return *reinterpret_cast<T*>(const_cast<uint8_t*>(get()->rawdata.data()));
  }

  virtual Resource<T>& operator=(const std::shared_ptr<filedata_t>& other) noexcept
  {
    ASSERT(!*this); // must not be previously allocated
    std::shared_ptr<filedata_t>::operator =(other);
    if(!other->loaded)
    {
      static_cast<filedataio_t*>(get())->Load();
      other->loaded = true;
    }
    return *this;
  }
};


template<typename T> // type T must inherit filedataio_t
struct MultiResource : std::shared_ptr<filedata_t>
{
  enum type_t : uint16_t
  {
    stillframe = 0,
    animation,
    complexanimation,
  };

  uint32_t magic;
  uint32_t version;
  type_t   type;
  uint16_t loopFlags;
  uint32_t totalTime;
  uint32_t interval;
  uint32_t frameCount;
  std::vector<T*> datapointers;

  MultiResource<T>& operator=(const std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    union
    {
      T*        dataptrT;
      uint8_t*  dataptr8;
      uint16_t* dataptr16;
      uint32_t* dataptr32;
    };

    dataptr8  = const_cast<uint8_t*>(other->rawdata.data());
    magic     = *dataptr32; ++dataptr32;
    version   = *dataptr32; ++dataptr32;
    type      = *dataptr16; ++dataptr16;
    loopFlags = *dataptr16; ++dataptr16;

    if(type == stillframe)
    {
      totalTime  = 0;
      interval   = 0;
      frameCount = 1;
    }
    else
    {
      totalTime  = *dataptr32; ++dataptr32;
      interval   = *dataptr32; ++dataptr32;
      frameCount = *dataptr32; ++dataptr32;
    }

    datapointers.resize(frameCount);

    switch(type)
    {
      case complexanimation:
        for(uint32_t x : frameCount)
        {
          if(*dataptr32 == UINT32_MAX)
          {
            ++dataptr32;
            datapointers[x] = dataptrT;
            ++dataptrT;
            if(!other->loaded)
              datapointers[x]->Load();
          }
          else
          {
            ++dataptr32;
            datapointers[x] = datapointers[*dataptr32];
          }
        }
        break;

      case stillframe:
      case animation:
        for(uint32_t x : frameCount)
        {
          datapointers[x] = dataptrT;
          ++dataptrT;
          if(!other->loaded)
            datapointers[x]->Load();
        }
        break;
    }

    other->loaded = true;
    return *this;
  }

  T* operator [](uint32_t num) const noexcept
  {
    ASSERT(num < frameCount);
    return dataptr[num];
  }
};

bool rspGetResource(SAKArchive& archive,        // In:  Archive to be used
                    const std::string& filename,           // In:  Resource name
                    std::shared_ptr<filedata_t>& res) noexcept      // Out: Resource data pointer
{
  if(archive.fileExists(filename))
    return false;

  res = archive.getFile(filename);
  return res;
}

template <class T>
void rspReleaseResource(std::shared_ptr<filedata_t>& res) noexcept  // In:  Pointer to resource
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
