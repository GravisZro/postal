#ifndef ANIMATEDRESOURCE_H
#define ANIMATEDRESOURCE_H

#include "sakarchive.h"


template<typename T> // type T must inherit filedata_t
struct AnimatedResource : filedata_t
{
  enum class consts : uint32_t
  {
    magic_number = 0x4E414843, // "CHAN"
    version = 1,
    newframe = UINT32_MAX,
  };
  template<typename S>
  friend constexpr bool operator == (S a, consts b)
    { return a == S(b); }

  enum class loop_t : uint16_t
  {
    uninitialized = 0xFFF0,
    none          = 0x0000,
    backtofront   = 0x0001,
    fronttoback   = 0x0002,
  };

  friend constexpr bool operator &(loop_t a, loop_t b)
    { return uint16_t(a) & uint16_t(b); }

  enum class type_t : uint16_t
  {
    uninitialized     = 0xFFF0,
    stillframe        = 0x0000,
    animation         = 0x0001,
    complexanimation  = 0x0002,
  };

  friend constexpr bool operator &(type_t a, type_t b)
    { return uint16_t(a) & uint16_t(b); }

  uint32_t    magic_number;
  uint16_t    version;
  type_t      type;
  std::string name;
  uint16_t    revision;
  loop_t      loopFlags;
  milliseconds_t totalTime;
  milliseconds_t interval;
  uint32_t       frameCount;
  std::vector<T> datapointers;

  AnimatedResource(uint32_t sz = 0)
    : filedata_t(sz),
      magic_number(0),
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

    magic_number = *dataptr32++;
    ASSERT(magic_number == consts::magic_number);
    version      = *dataptr16++;
    ASSERT(version == consts::version);
    type         = type_t(*dataptr16++);

    for(uint32_t slen = *dataptr32++; slen; --slen) // read length of the name then countdown
      name.push_back(*dataptrChar++); // read name into a string

    if(!name.empty()) // if the file was named
      revision = *dataptr16++; // read file revision number

    loopFlags = loop_t(*dataptr16++);

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
        d.setData(const_cast<uint8_t*>(dataptr8),   // use this data
                  data.size() - (dataptr8 - data)); // size is the rest of the file
        d.load();
        dataptr8 += d.dataSize();
        break;
      }

      case type_t::animation:
        for(T& d : datapointers)
        {
          d.setData(const_cast<uint8_t*>(dataptr8), 0);
          d.load();
          dataptr8 += d.dataSize();
        }
        break;

      case type_t::complexanimation:
        for(T& d : datapointers)
        {
          if(*dataptr32 == consts::newframe) // new frame data
          {
            dataptr32++;
            d.setData(const_cast<uint8_t*>(dataptr8), 0);
            d.load();
            dataptr8 += d.dataSize();
          }
          else // use existing frame data
            d = datapointers[*dataptr32++];
        }
        break;
    }
  }
};

#endif // ANIMATEDRESOURCE_H
