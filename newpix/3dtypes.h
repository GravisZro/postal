#ifndef THREEDTYPES_H
#define THREEDTYPES_H

#include "sakarchive.h"
#include "paltypes.h"

#include <cmath>

// helper types
typedef float real_t;
//typedef uint16_t triangle_t[3]; // a triangle is 3 index values for RSop data

#pragma pack(push, 1)
template<typename T>
struct point3d_t
{
  T x;
  T y;
  T z;

  template<typename V>
  inline T& operator [](V index) { return *(reinterpret_cast<T*>(this) + index); }
}; // a triangle is 3 index values for RSop data
#pragma pack(pop)

using vertex_t = point3d_t<uint16_t>;

// helper constant expressions
template<typename R, typename C>
constexpr uint32_t rowcol(R row, C col) noexcept { return (row * 4) + col; }


// Vector3D is to be treated as a primary type and thus has no encapsulation
class Vector3D
{
private:
  real_t m_x;
  real_t m_y;
  real_t m_z;
  real_t m_w;

public:
  inline Vector3D(real_t _x = 0.0,
                  real_t _y = 0.0,
                  real_t _z = 0.0,
                  real_t _w = 1.0) noexcept
    : m_x(_x), m_y(_y), m_z(_z), m_w(_w) { }

  inline real_t x(void) const { return m_x; }
  inline real_t y(void) const { return m_y; }
  inline real_t z(void) const { return m_z; }
  inline real_t w(void) const { return m_w; }

  inline void setX(real_t _x) { m_x = _x; }
  inline void setY(real_t _y) { m_y = _y; }
  inline void setZ(real_t _z) { m_z = _z; }
  inline void setW(real_t _w) { m_w = _w; }

  inline Vector3D& operator =(const Vector3D& other) noexcept
  {
    m_x = other.m_x;
    m_y = other.m_y;
    m_z = other.m_z;
    return *this;
  }

  inline real_t dot(const Vector3D& other) const noexcept
  {
    return m_x * other.m_x +
           m_y * other.m_y +
           m_z * other.m_z +
           m_w * other.m_w;
  }

  inline Vector3D cross(const Vector3D& other) const noexcept
  {
    return Vector3D(m_y * other.m_z - m_z * other.m_y,
                    m_z * other.m_x - m_x * other.m_z,
                    m_x * other.m_y - m_y * other.m_x);
  }

  inline Vector3D operator +(const Vector3D& other) const noexcept
  {
    return Vector3D(m_x + other.m_x,
                    m_y + other.m_y,
                    m_z + other.m_z);
  }

  inline Vector3D operator +(real_t scalar) const noexcept
  {
    return Vector3D(m_x + scalar,
                    m_y + scalar,
                    m_z + scalar);
  }

  inline Vector3D operator -(const Vector3D& other) const noexcept
  {
    return Vector3D(m_x - other.m_x,
                    m_y - other.m_y,
                    m_z - other.m_z);
  }


  inline Vector3D operator -(real_t scalar) const noexcept
  {
    return Vector3D(m_x - scalar,
                    m_y - scalar,
                    m_z - scalar);
  }

  inline Vector3D operator *(real_t scalar) const noexcept
  {
    return Vector3D(m_x * scalar,
                    m_y * scalar,
                    m_z * scalar);
  }

  inline Vector3D operator /(real_t scalar) const noexcept
  {
    ASSERT(scalar != 0.0);
    return Vector3D(m_x / scalar,
                    m_y / scalar,
                    m_z / scalar);
  }

  inline Vector3D& operator +=(const Vector3D& other) noexcept
  {
    m_x += other.m_x;
    m_y += other.m_y;
    m_z += other.m_z;
    return *this;
  }

  inline Vector3D& operator -=(const Vector3D& other) noexcept
  {
    m_x -= other.m_x;
    m_y -= other.m_y;
    m_z -= other.m_z;
    return *this;
  }

  inline Vector3D& operator *=(real_t scalar) noexcept
  {
    m_x *= scalar;
    m_y *= scalar;
    m_z *= scalar;
    return *this;
  }

  inline Vector3D& operator /=(real_t scalar) noexcept
  {
    ASSERT(scalar != 0.0);
    m_x /= scalar;
    m_y /= scalar;
    m_z /= scalar;
    return *this;
  }

  inline real_t magnatude(void) const noexcept
  {
    return std::sqrt(SQR(m_x) +
                     SQR(m_y) +
                     SQR(m_z));
  }

  inline Vector3D parallel(const Vector3D& other) const noexcept
    { return other * (dot(other) / SQR(other.magnatude())); }

  inline Vector3D perpendicular(const Vector3D& other) const noexcept
    { return *this - parallel(other); }

  inline Vector3D& makeHomogeneous(void) noexcept // factor out the w component
  {
    operator /=(m_w);
    m_w = 1.0;
    return *this;
  }

  // scale down the vector to have a magnatude of 1.0
  inline Vector3D& makeUnit(void) noexcept
    { return operator /=(magnatude()); }
};

static_assert(sizeof(Vector3D) == sizeof(real_t) * 4, "bad size!");


// types
template<typename T>
class Raw : public filedata_t // special type to wrap types that do not inherit filedata_t
{
private:
  T* raw;

public:
  Raw(uint32_t sz = 0) noexcept
    : filedata_t(sz), raw(nullptr) { }

  const T& operator =(const T& d) noexcept { return *raw = d; }
  operator T (void) const noexcept { return *raw; }
  T* operator &(void) const noexcept { return raw; }

  void load(void) noexcept
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
};

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
  shared_arr<pixel32_t> colors; // Array of colors

public:
  RTexture(uint32_t sz = 0) noexcept
    : filedata_t(sz), m_flags(flags_t::none) { }

  void setSize(uint16_t cnt) noexcept;
  uint16_t size(void) const noexcept { return m_count; }

  uint8_t getIndex(uint32_t idx) const noexcept { return indexes[idx]; }
  void setIndex(uint32_t idx, uint8_t val) noexcept { indexes[idx] = val; }


  void load(void) noexcept;

  bool operator ==(const RTexture& other) const noexcept;

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
      uint32_t linc) noexcept;

  ////////////////////////////////////////////////////////////////////////////////
  // Unmap colors from the specified palette and put them into the colors
  // array.  If the array of colors doesn't exist, it will be created.
  ////////////////////////////////////////////////////////////////////////////////
  void unmap(
      channel_t* pr,
      channel_t* pg,
      channel_t* pb,
      uint32_t lInc) noexcept;

  ////////////////////////////////////////////////////////////////////////////////
  // Muddy or brighten or darken.  Applies the specified brightness value
  // to every nth color (where n == lInc).
  ////////////////////////////////////////////////////////////////////////////////
  void adjust(
     float fAdjustment,	// In:  Adjustment factor (1.0 == same, < 1 == dimmer, > 1 == brighter).
     uint32_t lInc) noexcept;				// In:  Number of colors to skip.
};

class RMesh : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  shared_arr<vertex_t> triangles; // Array of triangles

public:
  RMesh(uint32_t sz = 0) noexcept
    : filedata_t(sz) { }

  void load(void) noexcept;

  bool operator ==(const RMesh& other) const noexcept;
};

class RSop : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  shared_arr<Vector3D> points;  // Array of points

public:
  RSop(uint32_t sz = 0) noexcept
    : filedata_t(sz) { }

  void load(void) noexcept;

  bool operator ==(const RSop& other) const noexcept;
};


// NOW, the class based transform allows matrix
// multiplication to occur WITHOUT multiplying
// 2 matrices together.  This prevents a malloc
// nightmare:
//
class RTransform : public filedata_t
{
  friend class RPipeLine; // allow encapsulation to be violated for speed
private:
  shared_arr<real_t> matdata; // This is compatible with the aggregate transform

public:
  RTransform(uint32_t sz = 0) noexcept;

  void load(void) noexcept;

  void makeIdentity(void) noexcept; // identity matrix
  void makeNull(void) noexcept; // null matrix

  // A Partial transform, assuming R3 = {0,0,0,1};
  void PreMulBy(const RTransform& M) noexcept;

  // Oversets the current data with the result!
  void Mul(const RTransform& A, const RTransform& B) noexcept; // 4x4 transforms:

  // Assumes R3 = {0,0,0,1}
  void Translate(real_t x, real_t y, real_t z) noexcept;

  void Scale(real_t x, real_t y, real_t z) noexcept;


  // Transform an actual point ( overwrites old point )
  void Transform(Vector3D& p) const noexcept;

  // Transform an actual point, and places the answer into a different pt
  void TransformInto(const Vector3D& src, Vector3D& dest) const noexcept;

  void Rz(int16_t sDeg) noexcept; // CCW!

  void Rx(int16_t sDeg) noexcept; // CCW!

  void Ry(int16_t sDeg) noexcept; // CCW!

  // a 3d ORTHOGONAL mapping from real_t box1 to box2
  // useful in screen and orthogonal view xforms
  // Use rspSub to create w vertices (w,h,d)
  // x1 BECOMES x2.  Note that w1 must NOT have any 0's.
  //
  void MakeBoxXF(Vector3D& x1, Vector3D& w1, Vector3D& x2, Vector3D& w2) noexcept;

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotTo(Vector3D point, Vector3D up) noexcept;

  // This is NOT hyper fast, and the result IS a rotation matrix
  // For now, point is it's x-axis and up i s it's y-axis.
  void MakeRotFrom(Vector3D point, Vector3D up) noexcept;
};

#endif // THREEDTYPES_H
