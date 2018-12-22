#ifndef ALPHAANIMATION_H
#define ALPHAANIMATION_H

#include "sakarchive.h"
#include "2dtypes.h"
//#include "image.h"

typedef RImage Image;

// Simple wrapper class for each frame of alpha animation
struct AlphaAnimation : public filedata_t
{
  point2d_t<uint16_t> point;
  Image image;											// "Normal" 8-bit color image
  shared_arr<Image> alphas;								// Array of alpha image's (could be empty!)

  void load(void) noexcept
  {
    union
    {
      const uint8_t*  dataptr8;
      const uint16_t* dataptr16;
      const uint32_t* dataptr32;
      const Image* dataptrimg;
    };

    dataptr8 = data;
    alphas.setSize(*dataptr32++);
    point.x = *dataptr16++;
    point.y = *dataptr16++;

    setLoaded();
  }
/*
      int16_t Load(RFile* pFile)
         {
         pFile->Read(&m_sNumAlphas);
         pFile->Read(&m_sX);
         pFile->Read(&m_sY);
         m_imColor.Load(pFile);
         Alloc(m_sNumAlphas);
         for (int16_t s = 0; s < m_sNumAlphas; s++)
            m_pimAlphaArray[s].Load(pFile);
         return pFile->Error();
         }
         */
};

#endif // ALPHAANIMATION_H
