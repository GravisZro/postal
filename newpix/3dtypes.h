#ifndef _3DTYPES_H_
#define _3DTYPES_H_

#include <RSPiX/BLUE/System.h>

#include "sakarchive.h"

typedef float real_t;
struct RP3d;

struct RTexture : filedata_t
{
  enum flags_t : uint16_t
  {
    none    = 0x0000,
    indexes = 0x0001,
    colors  = 0x0002,
  };

  uint16_t  count;    // Number of colors in array(s)
  flags_t   flags;    // option flags
  uint8_t*  indexptr; // Array of indices
  RPixel32* colorptr; // Array of colors

  RTexture(void);
 ~RTexture(void);

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

typedef uint16_t triangle_t[3]; // a triangle is 3 index values for RSop data

struct RMesh : filedata_t
{
  uint16_t    count;  // Number of triangles in array
  triangle_t* triptr; // Array of triangles

  RMesh(void);
 ~RMesh(void);

  void load(void);

  bool operator ==(const RMesh& other) const;
};

struct RSop : filedata_t
{
  uint32_t count;     // Number of points in array (only 65536 currently accessible)
  RP3d*    pointptr;  // Array of points

  RSop(void);
 ~RSop(void);

  void load(void);

  bool operator ==(const RSop& other) const;
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
       real_t _w = 1.0)
    : x(_x), y(_y), z(_z), w(_w) { }

  bool operator ==(const RP3d& other)
  {
    return x == other.x &&
           y == other.y &&
           z == other.z &&
           w == other.w;
  }

  const RP3d& operator =(const RP3d& other) // ignore w!
  {
    x = other.x;
    y = other.y;
    z = other.z;
    return other;
  }

  RP3d operator *(const RP3d& other)
  {
    return RP3d(y * other.z - z * other.y,
                x * other.y - y * other.x,
                z * other.x - x * other.z);
  }

  RP3d& operator -=(const RP3d& other)
  {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  RP3d& operator +=(const RP3d& other)
  {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  RP3d& scale(real_t s)
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  RP3d& makeHomogeneous(void)
  {
    ASSERT(w != 0.0);
    x /= w;
    y /= w;
    z /= w;
    w = 1.0;
    return *this;
  }

  // adjusts the length of a vector, ignoring w component
  void makeUnit(void)
  {
    real_t l = std::sqrt(SQR(x)+SQR(y)+SQR(z));
    ASSERT(l != 0.0);
    x /= l;
    y /= l;
    z /= l;
  }
};





// And some useful constants for manipulation:
const int16_t ROW0 = 0;
const int16_t ROW1 = 4;
const int16_t ROW2 = 8;
const int16_t ROW3 = 12;


// NOW, the class based transform allows matrix
// multiplication to occur WITHOUT multiplying
// 2 matrices together.  This prevents a malloc
// nightmare:
//
struct RTransform : filedata_t
{
  real_t data[16]; // This is compatible with the aggregate transform

  RTransform(void);
 ~RTransform(void);

  void makeIdentity(void); // identity matrix
  void makeNull(void); // null matrix

  //------------------------
  // ALL TRANSFORMATIONS ARE PRE-MULTIPLIES,
  // A Partial transform, assuming R3 = {0,0,0,1};
  //
  void PreMulBy(real_t* M);

  // Oversets the current data with the result!
  void Mul(real_t* A, real_t* B); // 4x4 transforms:

  // Assumes R3 = {0,0,0,1}
  void Translate(real_t x,real_t y,real_t z);

  void Scale(real_t x,real_t y, real_t z);


  // Transform an actual point ( overwrites old point )
  // Does a premultiply!
  void Transform(RP3d &p);

  // Transform an actual point, and places the answer into a different pt
  // Does a premultiply!
  void TransformInto(const RP3d& vSrc, RP3d& vDst);

  void Rz(int16_t sDeg); // CCW!

  void Rx(int16_t sDeg); // CCW!

  void Ry(int16_t sDeg); // CCW!

  // a 3d ORTHOGONAL mapping from real_t box1 to box2
  // useful in screen and orthogonal view xforms
  // Use rspSub to create w vertices (w,h,d)
  // x1 BECOMES x2.  Note that w1 must NOT have any 0's.
  //
  void MakeBoxXF(RP3d &x1,RP3d &w1,RP3d &x2,RP3d &w2);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotTo(RP3d point, RP3d up);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotFrom(RP3d point, RP3d up);

};

#endif // _3DTYPES_H_
