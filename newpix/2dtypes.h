#ifndef TWODTYPES_H
#define TWODTYPES_H

#include "sakarchive.h"

template<typename T = uint16_t>
struct point2d_t
{
  T x;
  union
  {
    T y;
    T z;
  };

  template<typename V>
  inline T& operator [](V index) { return *(reinterpret_cast<T*>(this) + index); }
};

class ImageResource : public filedata_t
{
public:
  ImageResource(uint32_t sz = 0) noexcept
    : filedata_t(sz) { }

  void load(void) noexcept;
};

class SpriteArray : public filedata_t
{
//  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
//  shared_arr<Sprite> m_sprites; // Array of triangles

public:
  SpriteArray(uint32_t sz = 0) noexcept
    : filedata_t(sz) { }

  void load(void) noexcept;

  bool operator ==(const RMesh& other) const noexcept;
};

#endif // TWODTYPES_H
