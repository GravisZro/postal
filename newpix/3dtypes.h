#ifndef THREEDTYPES_H
#define THREEDTYPES_H

#include <BLUE/System.h>

#include "sakarchive.h"

#include <cmath>

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
      const uint8_t* dataptr8;
      const T*       dataptrT;
    };

    dataptr8 = data;
    raw = const_cast<T*>(dataptrT);
    ++dataptrT;
    data.setSize(dataptr8 - data);
    setLoaded();
  }

  T* raw;
};

struct Vector3D
{
  real_t x;
  real_t y;
  real_t z;
  real_t w;

  inline Vector3D(real_t _x = 0.0,
              real_t _y = 0.0,
              real_t _z = 0.0,
              real_t _w = 1.0)
    : x(_x), y(_y), z(_z), w(_w) { }

  inline Vector3D& operator =(const Vector3D& other)
  {
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
  }

  inline real_t dot(const Vector3D& other) const
  {
    return x * other.x +
           y * other.y +
           z * other.z +
           w * other.w;
  }

  inline Vector3D cross(const Vector3D& other) const
  {
    return Vector3D(y * other.z - z * other.y,
                    z * other.x - x * other.z,
                    x * other.y - y * other.x);
  }

  inline Vector3D operator +(const Vector3D& other) const
  {
    return Vector3D(x + other.x,
                    y + other.y,
                    z + other.z);
  }

  inline Vector3D operator -(const Vector3D& other) const
  {
    return Vector3D(x - other.x,
                    y - other.y,
                    z - other.z);
  }

  inline Vector3D operator *(real_t scale) const
  {
    return Vector3D(x * scale,
                    y * scale,
                    z * scale);
  }

  inline Vector3D operator /(real_t inverse_scale) const
  {
    ASSERT(inverse_scale != 0.0);
    return Vector3D(x / inverse_scale,
                    y / inverse_scale,
                    z / inverse_scale);
  }

  inline Vector3D& operator +=(const Vector3D& other)
    { return *this = operator +(other); }

  inline Vector3D& operator -=(const Vector3D& other)
    { return *this = operator -(other); }

  inline Vector3D& operator *=(real_t scale)
    { return *this = operator *(scale); }

  inline Vector3D& operator /=(real_t inverse_scale)
    { return *this = operator /(inverse_scale); }

  inline real_t magnatude(void) const
  {
    return std::sqrt(SQR(x) +
                     SQR(y) +
                     SQR(z));
  }

  inline Vector3D parallel(const Vector3D& other) const
    { return other * (dot(other) / SQR(other.magnatude())); }

  inline Vector3D perpendicular(const Vector3D& other) const
    { return *this - parallel(other); }

  inline Vector3D& makeHomogeneous(void) // factor out the w component
  {
    operator /=(w);
    w = 1.0;
    return *this;
  }

  // scale down the vector to have a magnatude of 1.0
  inline Vector3D& makeUnit(void)
    { return operator /=(magnatude()); }
};

static_assert(sizeof(Vector3D) == sizeof(real_t) * 4, "bad size!");


class RTexture : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  enum flags_t : uint16_t
  {
    none     = 0x0000,
    indexarr = 0x0001,
    colorarr = 0x0002,
  };

  flags_t  m_flags;    // option flags
  uint16_t m_count;    // size of array(s)
  shared_arr<uint8_t>  indexes; // Array of indices
  shared_arr<RPixel32> colors; // Array of colors (unused by Postal)

public:
  RTexture(uint32_t sz = 0)
    : filedata_t(sz), m_flags(flags_t::none) { }

  void setSize(uint16_t cnt);
  uint16_t size(void) const { return m_count; }

  uint8_t getIndex(uint32_t idx) const { return indexes[idx]; }
  void setIndex(uint32_t idx, uint8_t val) { indexes[idx] = val; }


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

class RMesh : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  shared_arr<triangle_t> triangles; // Array of triangles

public:
  RMesh(uint32_t sz = 0) : filedata_t(sz) { }

  void load(void);

  bool operator ==(const RMesh& other) const;
};

class RSop : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  shared_arr<Vector3D> points;  // Array of points

public:
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
  void Transform(Vector3D& p) const;

  // Transform an actual point, and places the answer into a different pt
  void TransformInto(const Vector3D& src, Vector3D& dest) const;

  void Rz(int16_t sDeg); // CCW!

  void Rx(int16_t sDeg); // CCW!

  void Ry(int16_t sDeg); // CCW!

  // a 3d ORTHOGONAL mapping from real_t box1 to box2
  // useful in screen and orthogonal view xforms
  // Use rspSub to create w vertices (w,h,d)
  // x1 BECOMES x2.  Note that w1 must NOT have any 0's.
  //
  void MakeBoxXF(Vector3D& x1, Vector3D& w1, Vector3D& x2, Vector3D& w2);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotTo(Vector3D point, Vector3D up);

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotFrom(Vector3D point, Vector3D up);
};

#endif // THREEDTYPES_H
