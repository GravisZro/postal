#ifndef RESOURCECONTAINER_H
#define RESOURCECONTAINER_H

#include "resourcemanager.h"

template<typename T> // type T must inherit filedataio_t
struct ResourceContainer : Resource<T>
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

  Resource<T>& operator=(const std::shared_ptr<filedata_t>& other) noexcept
  {
    std::shared_ptr<filedata_t>::operator =(other);
    if(!other->loaded)
    {
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
            datapointers[x]->Load();
          }
          break;
      }

      other->loaded = true;
    }
    return *this;
  }

  T& operator [](uint32_t num) const noexcept
  {
    ASSERT(num < frameCount);
    return *dataptr[num];
  }
};

#endif // RESOURCECONTAINER_H
