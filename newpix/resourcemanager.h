#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <RSPiX/BLUE/System.h>

#include "sakarchive.h"

extern SAKArchive g_GameSAK;


namespace Resources
{
  static void normalize_path(std::string& path) noexcept;
}

template<typename T> // type T must inherit filedata_t
struct Resource : shared_arr<T>
{
  T& operator ->(void) const noexcept
  {
    ASSERT(std::shared_ptr<T>::operator bool()); // must already be allocated
    return *std::shared_ptr<T>::get()->dataptr;
  }

  std::shared_ptr<filedata_t>& operator =(std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    std::shared_ptr<T>::get()->load();
    return other;
  }
};

template<typename T> // type T must inherit filedata_t
struct MultiResource : shared_arr<T>
{
  enum loop_t : uint16_t
  {
    none        = 0x0000,
    backtofront = 0x0001,
    fronttoback = 0x0002,
  };

  enum type_t : uint16_t
  {
    uninitialized    = 0xFFFF,
    stillframe       = 0x0000,
    animation        = 0x0001,
    complexanimation = 0x0002,
  };

  uint32_t    magic;
  uint16_t    version;
  type_t      type;
  std::string name;
  loop_t      loopFlags;
  milliseconds_t totalTime;
  milliseconds_t interval;
  uint32_t       frameCount;
  std::vector<T> datapointers;

  MultiResource(void)
    : magic(0),
      version(0),
      type(uninitialized),
      loopFlags(none),
      totalTime(0),
      interval(0),
      frameCount(0)
  {
  }

  T& operator ->(void) const noexcept
  {
    ASSERT(std::shared_ptr<T>::operator bool()); // must already be allocated
    return datapointers.front();
  }

  T& operator [](milliseconds_t time) noexcept
  {
    ASSERT(std::shared_ptr<T>::operator bool()); // must already be allocated
    if (time >= totalTime) // for playing animation
    {
      if (loopFlags & loop_t::backtofront) // loop at the end to the beginning
        time %= totalTime;
      else
        time = totalTime - 1; // last frame
    }
    else if (time < 0) // for playing animation in reverse
    {
      if (loopFlags & loop_t::fronttoback) // loop at the beginning to the end
        time = (totalTime - 1) - ((-1 - time) % totalTime);
      else
        time = 0; // first frame
    }
    ASSERT(interval);
    ASSERT(time / interval < milliseconds_t(frameCount)); // keep it in range
    return datapointers[time / interval];
  }

  std::shared_ptr<filedata_t>& operator =(std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<T>::operator =(other);
    union
    {
      const char*     dataptrChar;
      const uint8_t*  dataptr8;
      const uint16_t* dataptr16;
      const uint32_t* dataptr32;
    };

    dataptr8  = std::shared_ptr<T>::get()->dataptr;

    magic     = *dataptr32++;
    version   = *dataptr16++;
    ASSERT(version == 1);
    type      = static_cast<type_t>(*dataptr16++);

    uint32_t slen = *dataptr32++; // read length of the name
    while(slen--)
      name.push_back(*dataptrChar++); // read name into a string

    if(!name.empty()) // if the file was named
      version = *dataptr16++; // read file version

    loopFlags = static_cast<loop_t>(*dataptr16++);

    if(type == stillframe)
    {
      totalTime  = 1;
      interval   = 1; // no divide by zero!
      frameCount = 1;
    }
    else
    {
      totalTime  = *dataptr32++;
      interval   = *dataptr32++;
      frameCount = *dataptr32++;
    }

    ASSERT(interval > 0);

    datapointers.resize(frameCount);

    switch(type)
    {
      case stillframe:
      {
        T& data = datapointers.front();
        data.dataptr = const_cast<uint8_t*>(dataptr8);
        data.size = std::shared_ptr<T>::get()->size - (data.dataptr - std::shared_ptr<T>::get()->dataptr); // size is the rest of the file
        data.load();
        dataptr8 += data.size;
        break;
      }

      case animation:
        for(T& data : datapointers)
        {
          data.dataptr = const_cast<uint8_t*>(dataptr8);
          data.load();
          dataptr8 += data.dataptr.count;
        }
        break;

      case complexanimation:
        for(T& data : datapointers)
        {
          if(*dataptr32++ == UINT32_MAX)
          {
            data.dataptr = const_cast<uint8_t*>(dataptr8);
            data.load();
            dataptr8 += data.dataptr.count;
          }
          else
            data = datapointers[*dataptr32++];
        }
        break;
    }
    return other;
  }
};

template<typename T>
static
bool rspGetResource(SAKArchive& archive,        // In:  Archive to be used
                    const std::string& filename,           // In:  Resource name
                    std::shared_ptr<T>& res) noexcept      // Out: Resource data pointer
{
  if(!archive.fileExists(filename))
  {
    TRACE("Couldn't open resource: %s\n", filename.c_str());
    return false;
  }

  res = archive.getFile<T>(filename);
  reinterpret_cast<T*>(res.get())->load();
  //std::static_pointer_cast<T, filedata_t>(res)->load();
  return res.operator bool();
}

template<typename T>
static
void rspReleaseResource(std::shared_ptr<T>& res) noexcept  // In:  Resource data pointer
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
