#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <RSPiX/BLUE/System.h>

#include "sakarchive.h"

namespace Resources
{
  static void normalize_path(std::string& path) noexcept;
}

template<typename T> // type T must inherit filedata_t
struct Resource : std::shared_ptr<filedata_t>
{
  T& operator ->(void) const noexcept
  {
    ASSERT(operator bool()); // must already be allocated
    return *reinterpret_cast<T*>(get()->dataptr);
  }

  std::shared_ptr<filedata_t>& operator =(std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    static_cast<T*>(get())->load();
    return other;
  }
};

template<typename T> // type T must inherit filedata_t
struct MultiResource : std::shared_ptr<filedata_t>
{
  enum loop_t : uint16_t
  {
    backtofront = 0x0001,
    fronttoback = 0x0002,
  };

  enum type_t : uint16_t
  {
    stillframe = 0,
    animation,
    complexanimation,
  };

  uint32_t magic;
  uint32_t version;
  type_t   type;
  loop_t   loopFlags;
  milliseconds_t totalTime;
  milliseconds_t interval;
  uint32_t       frameCount;
  std::vector<T> datapointers;

  T& operator [](milliseconds_t time) const noexcept
  {
    ASSERT(operator bool()); // must already be allocated
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
    ASSERT(time / interval < frameCount); // keep it in range
    return datapointers[time / interval];
  }

  std::shared_ptr<filedata_t>& operator =(std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    union
    {
      const uint8_t*  dataptr8;
      const uint16_t* dataptr16;
      const uint32_t* dataptr32;
    };

    dataptr8  = get()->dataptr;
    magic     = *dataptr32++;
    version   = *dataptr32++;
    type      = *dataptr16++;
    loopFlags = *dataptr16++;

    if(type == stillframe)
    {
      totalTime  = 0;
      interval   = 1; // no divide by zero!
      frameCount = 1;
    }
    else
    {
      totalTime  = *dataptr32++;
      interval   = *dataptr32++;
      frameCount = *dataptr32++;
    }

    ASSERT(interval);

    datapointers.resize(frameCount);

    switch(type)
    {
      case complexanimation:
        for(T& data : datapointers)
        {
          if(*dataptr32++ == UINT32_MAX)
          {
            data.dataptr = const_cast<uint8_t*>(dataptr8);
            data.load();
            dataptr8 += data.size;
          }
          else
            data = datapointers[*dataptr32++];
        }
        break;

      case stillframe:
      case animation:
        for(T& data : datapointers)
        {
          data.dataptr = const_cast<uint8_t*>(dataptr8);
          data.load();
          dataptr8 += data.size;
        }
        break;
    }
    return other;
  }
};


bool rspGetResource(SAKArchive& archive,        // In:  Archive to be used
                    const std::string& filename,           // In:  Resource name
                    std::shared_ptr<filedata_t>& res) noexcept      // Out: Resource data pointer
{
  if(archive.fileExists(filename))
    return false;

  res = archive.getFile(filename);
  return res.operator bool();
}

void rspReleaseResource(std::shared_ptr<filedata_t>& res) noexcept  // In:  Resource data pointer
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
