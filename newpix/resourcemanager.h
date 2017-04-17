#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <BLUE/System.h>

#include "sakarchive.h"

extern SAKArchive g_GameSAK;

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
struct MultiResource : filedata_t
{
  enum class loop_t : uint16_t
  {
    uninitialized = 0xFFF0,
    none          = 0x0000,
    backtofront   = 0x0001,
    fronttoback   = 0x0002,
  };

  enum class type_t : uint16_t
  {
    uninitialized     = 0xFFF0,
    stillframe        = 0x0000,
    animation         = 0x0001,
    complexanimation  = 0x0002,
  };

  friend constexpr bool operator &(loop_t a, loop_t b) { return uint16_t(a) & uint16_t(b); }
  friend constexpr bool operator &(type_t a, type_t b) { return uint16_t(a) & uint16_t(b); }

  uint32_t    magic;
  uint16_t    version;
  type_t      type;
  std::string name;
  loop_t      loopFlags;
  milliseconds_t totalTime;
  milliseconds_t interval;
  uint32_t       frameCount;
  std::vector<T> datapointers;

  MultiResource(uint32_t sz = 0)
    : filedata_t(sz),
      magic(0),
      version(0),
      type(type_t::uninitialized),
      loopFlags(loop_t::uninitialized),
      totalTime(0),
      interval(0),
      frameCount(0)
  {
  }

  T& operator ->(void) const noexcept
  {
    ASSERT(data.operator bool()); // must already be allocated
    return datapointers.front();
  }

  T& atTime(milliseconds_t time) noexcept
  {
    ASSERT(data.operator bool()); // must already be allocated
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

  bool SetLooping(uint16_t val)
  {
    loop_t old = loopFlags;
    loopFlags = loop_t(val);
    return old == loopFlags;
  }

  void load(void) noexcept
  {
    union
    {
      const char*     dataptrChar;
      const uint8_t*  dataptr8;
      const uint16_t* dataptr16;
      const uint32_t* dataptr32;
    };

    dataptr8  = data;

    magic     = *dataptr32++;
    version   = *dataptr16++;
    ASSERT(version == 1);
    type      = static_cast<type_t>(*dataptr16++);

    uint32_t slen = *dataptr32++; // read length of the name
    while(slen--)
      name.push_back(*dataptrChar++); // read name into a string
    slen = 0;

    if(!name.empty()) // if the file was named
      version = *dataptr16++; // read file version

    loopFlags = static_cast<loop_t>(*dataptr16++);

    if(type == type_t::stillframe)
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
      default:
        ASSERT(false);
        break;

      case type_t::stillframe:
      {
        T& d = datapointers.front();
        d.data = const_cast<uint8_t*>(dataptr8);
        d.data.count = data.count - (dataptr8 - data); // size is the rest of the file
        d.load();
        dataptr8 += d.data.count;
        break;
      }

      case type_t::animation:
        for(T& d : datapointers)
        {
          d.data = const_cast<uint8_t*>(dataptr8);
          d.load();
          dataptr8 += d.data.count;
          ++slen;
        }
        break;

      case type_t::complexanimation:
        for(T& d : datapointers)
        {
          if(*dataptr32 == UINT32_MAX)
          {
            dataptr32++;
            d.data = const_cast<uint8_t*>(dataptr8);
            d.load();
            dataptr8 += d.data.count;
          }
          else
            d = datapointers[*dataptr32++];
          ++slen;
        }
        break;
    }
  }
};

template<typename T>
static bool rspGetResource(SAKArchive& archive,                   // In:  Archive to be used
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
  TRACE("loaded %s\n", filename.c_str());
  return res.operator bool();
}

template<typename T>
static void rspReleaseResource(std::shared_ptr<T>& res) noexcept  // In:  Resource data pointer
{
  res.reset();
}

#endif // RESOURCEMANAGER_H
