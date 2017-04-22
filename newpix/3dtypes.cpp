#include "3dtypes.h"

#include <ORANGE/color/colormatch.h>
#include <ORANGE/QuickMath/QuickMath.h>

#include <cstring>
#include <cmath>


RP3d::RP3d(real_t _x,
           real_t _y,
           real_t _z,
           real_t _w)
  : x(_x), y(_y), z(_z), w(_w) { }

const RP3d& RP3d::operator =(const RP3d& other)
{
  x = other.x;
  y = other.y;
  z = other.z;
  return other;
}

RP3d RP3d::operator *(const RP3d& other) const
{
  return RP3d(y * other.z - z * other.y,
              z * other.x - x * other.z,
              x * other.y - y * other.x);
}

real_t RP3d::dot(const RP3d& other) const
{
  return x * other.x +
      y * other.y +
      z * other.z +
      w * other.w;
}

RP3d& RP3d::operator -=(const RP3d& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

RP3d& RP3d::operator +=(const RP3d& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

RP3d& RP3d::scale(real_t s)
{
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

RP3d& RP3d::makeHomogeneous(void) // factor out the w component
{
  ASSERT(w != 0.0);
  x /= w;
  y /= w;
  z /= w;
  w = 1.0;
  return *this;
}

// adjusts the length of a vector, ignoring w component
RP3d& RP3d::makeUnit(void)
{
  real_t l = std::sqrt(SQR(x)+SQR(y)+SQR(z));
  ASSERT(l != 0.0);
  x /= l;
  y /= l;
  z /= l;
  return *this;
}

//==============================================================================

void RTexture::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const RPixel32* dataptrP32;
  };

  dataptr8 = data;
  count = *dataptr16++;
  flags = static_cast<flags_t>(*dataptr16++);

  if(flags & flags_t::indexarr)
  {
    indexes = const_cast<uint8_t*>(dataptr8);
    indexes.count = count;
    dataptr8 += count;
  }
  if(flags & flags_t::colorarr)
  {
    colors = const_cast<RPixel32*>(dataptrP32);
    colors.count = count;
    dataptrP32 += count;
  }

  data.count = dataptr8 - data;
  loaded = true;
}

bool RTexture::operator ==(const RTexture& other) const
{
  return indexes == other.indexes &&
         colors == other.colors;
}

// Map colors onto the specified palette.  For each color, the best
// matching color is found in the  palette, and the associated palette
// index is written to the array of indices.  If the array of indices
// doesn't exist, it will be created.
void RTexture::remap(
    palindex_t sStartIndex,
    palindex_t sNumIndex,
    channel_t* pr,
    channel_t* pg,
    channel_t* pb,
    uint32_t linc)
{
  ASSERT(colors != nullptr);

  if (indexes == nullptr)
    indexes.allocate(count);

  for (uint32_t i = 0; i < count; i++)
  {
    indexes[i] = rspMatchColorRGB(
                    int32_t(colors[i].red),
                    int32_t(colors[i].green),
                    int32_t(colors[i].blue),
                    sStartIndex,sNumIndex,
                    pr,pg,pb,linc);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Unmap colors from the specified palette and put them into the colors
// array.  If the array of colors doesn't exist, it will be created.
////////////////////////////////////////////////////////////////////////////////
void RTexture::unmap(
    channel_t* pr,
    channel_t* pg,
    channel_t* pb,
    uint32_t lInc)
{
  UNUSED(lInc);
  ASSERT(indexes != nullptr);

  if (colors == nullptr)
    colors.allocate(count);

  uint8_t*  pu8    = indexes;
  RPixel32* ppix   = colors;
  int16_t   sCount = count;
  while (sCount--)
  {
    ppix->red   = pr[*pu8];
    ppix->green = pg[*pu8];
    ppix->blue  = pb[*pu8];

    ppix++;
    pu8++;
  }
}

constexpr channel_t clampColorChannel(float fColor)
  { return fColor < 0xFF ? channel_t(fColor + 0.5f) : 0xFF; }

////////////////////////////////////////////////////////////////////////////////
// Muddy or brighten or darken.  Applies the specified brightness value
// to every nth color (where n == lInc).
////////////////////////////////////////////////////////////////////////////////
void RTexture::adjust(
   float fAdjustment,	// In:  Adjustment factor (1.0 == same, < 1 == dimmer, > 1 == brighter).
   uint32_t lInc)				// In:  Number of colors to skip.
{
  ASSERT(colors);
  ASSERT(fAdjustment >= 0.0f);

  RPixel32* ppix = colors;
  int16_t	sCount = colors.count / lInc;
  while (sCount--)
  {
    ppix->red   = clampColorChannel(ppix->red   * fAdjustment);
    ppix->green = clampColorChannel(ppix->green * fAdjustment);
    ppix->blue  = clampColorChannel(ppix->blue  * fAdjustment);
    ppix += lInc;
  }
}

//==============================================================================

void RMesh::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const triangle_t* dataptrTri;
  };

  dataptr8 = data;
  triangles.count = *dataptr16++;
  triangles = const_cast<triangle_t*>(dataptrTri);
  dataptrTri += triangles.count;

  data.count = dataptr8 - data;
  loaded = true;
}

bool RMesh::operator ==(const RMesh& other) const
{
  return triangles == other.triangles;
}

//==============================================================================

void RSop::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint32_t* dataptr32;
    const RP3d*     dataptr3d;
  };

  dataptr8 = data;
  points.count = *dataptr32++;
  points = const_cast<RP3d*>(dataptr3d);
  dataptr3d += points.count;

  data.count = dataptr8 - data;
  loaded = true;
}

bool RSop::operator ==(const RSop& other) const
{
  return points == other.points;
}

//==============================================================================

inline void MatrixMultiply(real_t* matOut, real_t* matA, real_t* matB, uint8_t row, uint8_t col)
{
  matOut[rowcol(row, col)] =
      matA[rowcol(row, 0)] * matB[rowcol(0, col)] +
      matA[rowcol(row, 1)] * matB[rowcol(1, col)] +
      matA[rowcol(row, 2)] * matB[rowcol(2, col)] +
      matA[rowcol(row, 3)] * matB[rowcol(3, col)];
}

inline void MatrixScale(real_t* mat, real_t x, real_t y, real_t z, uint8_t col)
{
  mat[rowcol(0, col)] *= x;
  mat[rowcol(1, col)] *= y;
  mat[rowcol(2, col)] *= z;
}

#define ExecOp4(op, ...) \
  op(__VA_ARGS__, 0); \
  op(__VA_ARGS__, 1); \
  op(__VA_ARGS__, 2); \
  op(__VA_ARGS__, 3)


#define ExecOp3x4(op, ...) \
  op(__VA_ARGS__, 0, 0); \
  op(__VA_ARGS__, 0, 1); \
  op(__VA_ARGS__, 0, 2); \
  op(__VA_ARGS__, 0, 3); \
  op(__VA_ARGS__, 1, 0); \
  op(__VA_ARGS__, 1, 1); \
  op(__VA_ARGS__, 1, 2); \
  op(__VA_ARGS__, 1, 3); \
  op(__VA_ARGS__, 2, 0); \
  op(__VA_ARGS__, 2, 1); \
  op(__VA_ARGS__, 2, 2); \
  op(__VA_ARGS__, 2, 3)

#define setRow(row, val) \
  reinterpret_cast<RP3d*>(matdata.get())[row] = val;

#define setColumn(col, val) \
  matdata[rowcol(0, col)] = val.x; \
  matdata[rowcol(1, col)] = val.y; \
  matdata[rowcol(2, col)] = val.z;


#ifndef _MSC_VER
constexpr
#endif
real_t identity_data[16] = { 1.0, 0.0, 0.0, 0.0,
                             0.0, 1.0, 0.0, 0.0,
                             0.0, 0.0, 1.0, 0.0,
                             0.0, 0.0, 0.0, 1.0 };

RTransform::RTransform(uint32_t sz)  // init to an identity transform
  : filedata_t(sz)
{
  if(!data)
    matdata.allocate(16);
  makeIdentity();
}

void RTransform::load(void)
{
  data.count = 16 * sizeof(real_t);
  matdata = reinterpret_cast<real_t*>(data.get());
  matdata.count = 16;
  loaded = true;
}

void RTransform::makeIdentity(void) // identity matrix
{
  ASSERT(matdata.count >= 16);
  std::memcpy(matdata, identity_data, sizeof(real_t) * 16);
}

void RTransform::makeNull(void) // null matrix
{
  ASSERT(matdata.count >= 16);
  std::memset(matdata, 0, sizeof(real_t) * 15);
  matdata[15] = 1;
}

// A Partial transform, assuming R3 = {0,0,0,1};
void RTransform::PreMulBy(real_t* M)
{
  ExecOp3x4(MatrixMultiply, matdata, M, matdata);
}

// Oversets the current data with the result!
void RTransform::Mul(real_t* A, real_t* B) // 4x4 transforms:
{
  ExecOp3x4(MatrixMultiply, matdata, A, B);
}

void RTransform::Scale(real_t x,real_t y, real_t z)
{
  ExecOp4(MatrixScale, matdata, x, y, z);
}

// Assumes R3 = {0,0,0,1}
void RTransform::Translate(real_t x, real_t y, real_t z)
{
  matdata[rowcol(0, 3)] += x;
  matdata[rowcol(1, 3)] += y;
  matdata[rowcol(2, 3)] += z;
}

// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotTo(RP3d point, RP3d up)
{
  point.makeUnit();
  up.makeUnit();
  makeNull();
  RP3d tmp = up * point;
  setColumn(0, point);
  setColumn(1, up);
  setColumn(2, tmp);
}

// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotFrom(RP3d point, RP3d up)
{
  point.makeUnit();
  up.makeUnit();
  makeNull();
  setRow(0, point);
  setRow(1, up);
  setRow(2, point * up);
}

// Transform an actual point ( overwrites old point )
void RTransform::Transform(RP3d& p) const
{
  RP3d* d = reinterpret_cast<RP3d*>(matdata.get());
  RP3d temp;
  temp.x = p.dot(d[0]);
  temp.y = p.dot(d[1]);
  temp.z = p.dot(d[2]);
  p = temp;
  p.w = 1.0;
}

// Transform an actual point, and places the answer into a different pt
void RTransform::TransformInto(const RP3d& src, RP3d& dest) const
{
  RP3d* d = reinterpret_cast<RP3d*>(matdata.get());
  dest.x = src.dot(d[0]);
  dest.y = src.dot(d[1]);
  dest.z = src.dot(d[2]);
  dest.w = 1.0;
}

void RTransform::Rz(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (uint8_t i = 0; i < 4; ++i)
  {
    register real_t row1 = matdata[rowcol(1, i)];
    register real_t row0 = matdata[rowcol(0, i)];
    matdata[rowcol(1, i)] = row0 * S + row1 * C;
    matdata[rowcol(0, i)] = row0 * C - row1 * S;
  }
}

void RTransform::Rx(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (uint8_t i = 0; i < 4; ++i)
  {
    register real_t row1 = matdata[rowcol(1, i)];
    register real_t row2 = matdata[rowcol(2, i)];
    matdata[rowcol(2, i)] = row1 * S + row2 * C;
    matdata[rowcol(1, i)] = row1 * C - row2 * S;
  }
}

void RTransform::Ry(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (uint8_t i = 0; i < 4; ++i)
  {
    register real_t row0 = matdata[rowcol(0, i)];
    register real_t row2 = matdata[rowcol(2, i)];
    matdata[rowcol(2, i)] = -row0 * S + row2 * C;
    matdata[rowcol(0, i)] =  row0 * C + row2 * S;
  }
}

// a 3d ORTHOGONAL mapping from real_t box1 to box2
// useful in screen and orthogonal view xforms
// Use rspSub to create w vertices (w,h,d)
// x1 BECOMES x2.  Note that w1 must NOT have any 0's.
//
void RTransform::MakeBoxXF(RP3d& x1, RP3d& w1, RP3d& x2, RP3d& w2)
{
  // NOT OF MAXIMUM SPEED!
  makeIdentity();
  Translate(-x1.x, -x1.y, -x1.z);
  Scale(w2.x/w1.x, w2.y/w1.y, w2.z/w1.z);
  Translate(x2.x, x2.y, x2.z);
}
