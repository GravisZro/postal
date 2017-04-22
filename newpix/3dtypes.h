#ifndef THREEDTYPES_H
#define THREEDTYPES_H

#include <BLUE/System.h>

#include "sakarchive.h"


// helper types
typedef float real_t;
typedef uint16_t triangle_t[3]; // a triangle is 3 index values for RSop data

// helper constant expressions
template<typename R, typename C>
constexpr uint32_t rowcol(R row, C col) { return (row * 4) + col; }

// types
template<typename T>
struct Raw : filedata_t // special type to wrap types that do not inherit filedata_t
{
  Raw(uint32_t sz = 0)
    : filedata_t(sz), raw(nullptr) { }

  const T& operator =(const T& d) { return *raw = d; }
  operator T (void) const { return *raw; }
  T* operator &(void) const { return raw; }

  void load(void)
  {
    union
    {
      const uint8_t*  dataptr8;
      const T*        dataptrT;
    };

    dataptr8 = data;
    raw = const_cast<T*>(dataptrT);
    ++dataptrT;
    data.count = dataptr8 - data;
    loaded = true;
  }

  T* raw;
};

struct RP3d
{
  real_t x;
  real_t y;
  real_t z;
  real_t w;

  RP3d(real_t _x = 0.0,
       real_t _y = 0.0,
       real_t _z = 0.0,
       real_t _w = 1.0);

  const RP3d& operator =(const RP3d& other);

  RP3d  operator  *(const RP3d& other) const;
  RP3d& operator -=(const RP3d& other);
  RP3d& operator +=(const RP3d& other);

  real_t dot(const RP3d& other) const;

  RP3d& scale(real_t s);

  RP3d& makeHomogeneous(void); // factor out the w component

  // adjusts the length of a vector, ignoring w component
  RP3d& makeUnit(void);
};

static_assert(sizeof(RP3d) == sizeof(real_t) * 4, "bad size!");


struct RTexture : filedata_t
{
  enum flags_t : uint16_t
  {
    none     = 0x0000,
    indexarr = 0x0001,
    colorarr = 0x0002,
  };

  flags_t   flags;    // option flags
  uint16_t  count;    // size of array(s)
  shared_arr<uint8_t>  indexes; // Array of indices
  shared_arr<RPixel32> colors; // Array of colors

  RTexture(uint32_t sz = 0)
    : filedata_t(sz), flags(flags_t::none) { }

  void load(void);

  bool operator ==(const RTexture& other) const;

  // Map colors onto the specified palette.  For each color, the best
  // matching color is found in the  palette, and the associated palette
  // index is written to the array of indices.  If the array of indices
  // doesn't exist, it will be created.
  void remap(
      palindex_t sStartIndex,
      palindex_t sNumIndex,
      channel_t* pr,
      channel_t* pg,
      channel_t* pb,
      uint32_t linc);

  ////////////////////////////////////////////////////////////////////////////////
  // Unmap colors from the specified palette and put them into the colors
  // array.  If the array of colors doesn't exist, it will be created.
  ////////////////////////////////////////////////////////////////////////////////
  void unmap(
      channel_t* pr,
      channel_t* pg,
      channel_t* pb,
      uint32_t lInc);

  ////////////////////////////////////////////////////////////////////////////////
  // Muddy or brighten or darken.  Applies the specified brightness value
  // to every nth color (where n == lInc).
  ////////////////////////////////////////////////////////////////////////////////
  void adjust(
     float fAdjustment,	// In:  Adjustment factor (1.0 == same, < 1 == dimmer, > 1 == brighter).
     uint32_t lInc);				// In:  Number of colors to skip.
};

struct RMesh : filedata_t
{
  shared_arr<triangle_t> triangles; // Array of triangles

  RMesh(uint32_t sz = 0) : filedata_t(sz) { }

  void load(void);

  bool operator ==(const RMesh& other) const;
};

struct RSop : filedata_t
{
  shared_arr<RP3d> points;  // Array of points

  RSop(uint32_t sz = 0) : filedata_t(sz) { }

  void load(void);

  bool operator ==(const RSop& other) const;
};


// NOW, the class based transform allows matrix
// multiplication to occur WITHOUT multiplying
// 2 matrices together.  This prevents a malloc
// nightmare:
//
struct RTransform : filedata_t
{
  shared_arr<real_t> matdata; // This is compatible with the aggregate transform

  RTransform(uint32_t sz = 0);

  void load(void);

  void makeIdentity(void); // identity matrix
  void makeNull(void); // null matrix

  // A Partial transform, assuming R3 = {0,0,0,1};
  void PreMulBy(real_t* M);

  // Oversets the current data with the result!
  void Mul(real_t* A, real_t* B); // 4x4 transforms:

  // Assumes R3 = {0,0,0,1}
  void Translate(real_t x, real_t y, real_t z);

  void Scale(real_t x, real_t y, real_t z);


  // Transform an actual point ( overwrites old point )
  void Transform(RP3d& p) const;

  // Transform an actual point, and places the answer into a different pt
  void TransformInto(const RP3d& src, RP3d& dest) const;

  void Rz(int16_t sDeg); // CCW!

  void Rx(int16_t sDeg); // CCW!

  void Ry(int16_t sDeg); // CCW!

  // a 3d ORTHOGONAL mapping from real_t box1 to box2
  // useful in screen and orthogonal view xforms
  // Use rspSub to create w vertices (w,h,d)
  // x1 BECOMES x2.  Note that w1 must NOT have any 0's.
  //
  void MakeBoxXF(RP3d& x1, RP3d& w1, RP3d& x2, RP3d& w2);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotTo(RP3d point, RP3d up);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotFrom(RP3d point, RP3d up);
};

#endif // THREEDTYPES_H
