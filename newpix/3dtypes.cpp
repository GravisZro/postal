#include "3dtypes.h"

#include <ORANGE/color/colormatch.h>
#include <ORANGE/QuickMath/QuickMath.h>

#include <cstring>

void RTexture::setSize(uint16_t cnt) noexcept
{
  m_count = cnt;
  if(indexes.size())
    indexes.setSize(cnt);
  if(colors.size())
    colors.setSize(cnt);
}

void RTexture::load(void) noexcept
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const RPixel32* dataptrP32;
  };

  dataptr8 = data;
  m_count = *dataptr16++;
  m_flags = static_cast<flags_t>(*dataptr16++);

  if(m_flags & flags_t::indexarr)
  {
    indexes = const_cast<uint8_t*>(dataptr8);
    indexes.setSize(m_count);
    dataptr8 += m_count;
  }
  if(m_flags & flags_t::colorarr)
  {
    colors = const_cast<RPixel32*>(dataptrP32);
    colors.setSize(m_count);
    dataptrP32 += m_count;
  }

  data.setSize(dataptr8 - data);
  setLoaded();
}

bool RTexture::operator ==(const RTexture& other) const noexcept
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
    uint32_t linc) noexcept
{
  ASSERT(colors != nullptr);

  if (indexes == nullptr)
    indexes.allocate(m_count);

  for (uint32_t i = 0; i < m_count; i++)
  {
    indexes[i] = rspMatchColorRGB(
                     colors[i].red,
                     colors[i].green,
                     colors[i].blue,
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
    uint32_t lInc) noexcept
{
  UNUSED(lInc);
  ASSERT(indexes != nullptr);

  if (colors == nullptr)
    colors.allocate(m_count);

  uint8_t*  pu8    = indexes;
  RPixel32* ppix   = colors;
  int16_t   sCount = m_count;
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
   uint32_t lInc) noexcept				// In:  Number of colors to skip.
{
  ASSERT(colors);
  ASSERT(fAdjustment >= 0.0f);

  RPixel32* ppix = colors;
  int16_t	sCount = colors.size() / lInc;
  while (sCount--)
  {
    ppix->red   = clampColorChannel(ppix->red   * fAdjustment);
    ppix->green = clampColorChannel(ppix->green * fAdjustment);
    ppix->blue  = clampColorChannel(ppix->blue  * fAdjustment);
    ppix += lInc;
  }
}

//==============================================================================

void RMesh::load(void) noexcept
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const triangle_t* dataptrTri;
  };

  dataptr8 = data;
  triangles.setSize(*dataptr16++);
  triangles = const_cast<triangle_t*>(dataptrTri);
  dataptrTri += triangles.size();

  data.setSize(dataptr8 - data);
  setLoaded();
}

bool RMesh::operator ==(const RMesh& other) const noexcept
{
  return triangles == other.triangles;
}

//==============================================================================

void RSop::load(void) noexcept
{
  union
  {
    const uint8_t*  dataptr8;
    const uint32_t* dataptr32;
    const Vector3D*     dataptr3d;
  };

  dataptr8 = data;
  points.setSize(*dataptr32++);
  points = const_cast<Vector3D*>(dataptr3d);
  dataptr3d += points.size();

  data.setSize(dataptr8 - data);
  setLoaded();
}

bool RSop::operator ==(const RSop& other) const noexcept
{
  return points == other.points;
}

//==============================================================================

inline void MatrixMultiply(real_t* matOut, real_t* matA, real_t* matB, uint8_t row, uint8_t col) noexcept
{
  matOut[rowcol(row, col)] =
      matA[rowcol(row, 0)] * matB[rowcol(0, col)] +
      matA[rowcol(row, 1)] * matB[rowcol(1, col)] +
      matA[rowcol(row, 2)] * matB[rowcol(2, col)] +
      matA[rowcol(row, 3)] * matB[rowcol(3, col)];
}

inline void MatrixScale(real_t* mat, real_t x, real_t y, real_t z, uint8_t col) noexcept
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
  reinterpret_cast<Vector3D*>(matdata.get())[row] = val;

#define setColumn(col, val) \
  matdata[rowcol(0, col)] = val.x(); \
  matdata[rowcol(1, col)] = val.y(); \
  matdata[rowcol(2, col)] = val.z();


#ifndef _MSC_VER
constexpr
#endif
real_t identity_data[16] = { 1.0, 0.0, 0.0, 0.0,
                             0.0, 1.0, 0.0, 0.0,
                             0.0, 0.0, 1.0, 0.0,
                             0.0, 0.0, 0.0, 1.0 };

RTransform::RTransform(uint32_t sz) noexcept  // init to an identity transform
  : filedata_t(sz)
{
  if(!data)
    matdata.allocate(16);
  makeIdentity();
}

void RTransform::load(void) noexcept
{
  data.setSize(16 * sizeof(real_t));
  matdata = reinterpret_cast<real_t*>(data.get());
  matdata.setSize(16);
  setLoaded();
}

void RTransform::makeIdentity(void) noexcept // identity matrix
{
  ASSERT(matdata.size() >= 16);
  std::memcpy(matdata, identity_data, sizeof(real_t) * 16);
}

void RTransform::makeNull(void) noexcept // null matrix
{
  ASSERT(matdata.size() >= 16);
  std::memset(matdata, 0, sizeof(real_t) * 15);
  matdata[15] = 1;
}

// A Partial transform, assuming R3 = {0,0,0,1};
void RTransform::PreMulBy(const RTransform& M) noexcept
{
  ExecOp3x4(MatrixMultiply, matdata, M.matdata, matdata);
}

// Oversets the current data with the result!
void RTransform::Mul(const RTransform& A, const RTransform& B) noexcept // 4x4 transforms:
{
  ExecOp3x4(MatrixMultiply, matdata, A.matdata, B.matdata);
}

void RTransform::Scale(real_t x,real_t y, real_t z) noexcept
{
  ExecOp4(MatrixScale, matdata, x, y, z);
}

// Assumes R3 = {0,0,0,1}
void RTransform::Translate(real_t x, real_t y, real_t z) noexcept
{
  matdata[rowcol(0, 3)] += x;
  matdata[rowcol(1, 3)] += y;
  matdata[rowcol(2, 3)] += z;
}

// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotTo(Vector3D point, Vector3D up) noexcept
{
  point.makeUnit();
  up.makeUnit();
  makeNull();
  Vector3D tmp = up.cross(point);
  setColumn(0, point);
  setColumn(1, up);
  setColumn(2, tmp);
}

// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotFrom(Vector3D point, Vector3D up) noexcept
{
  point.makeUnit();
  up.makeUnit();
  makeNull();
  setRow(0, point);
  setRow(1, up);
  setRow(2, point.cross(up));
}

// Transform an actual point ( overwrites old point )
void RTransform::Transform(Vector3D& p) const noexcept
{
  Vector3D* d = reinterpret_cast<Vector3D*>(matdata.get());
  p = Vector3D(p.dot(d[0]), p.dot(d[1]), p.dot(d[2]));
  p.setW(1.0);
}

// Transform an actual point, and places the answer into a different pt
void RTransform::TransformInto(const Vector3D& src, Vector3D& dest) const noexcept
{
  Vector3D* d = reinterpret_cast<Vector3D*>(matdata.get());
  dest.setX(src.dot(d[0]));
  dest.setY(src.dot(d[1]));
  dest.setY(src.dot(d[2]));
  dest.setW(1.0);
}

void RTransform::Rz(int16_t sDeg) noexcept // CCW!
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

void RTransform::Rx(int16_t sDeg) noexcept // CCW!
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

void RTransform::Ry(int16_t sDeg) noexcept // CCW!
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
void RTransform::MakeBoxXF(Vector3D& x1, Vector3D& w1, Vector3D& x2, Vector3D& w2) noexcept
{
  // NOT OF MAXIMUM SPEED!
  makeIdentity();
  Translate(-x1.x(), -x1.y(), -x1.z());
  Scale(w2.x()/w1.x(), w2.y()/w1.y(), w2.z()/w1.z());
  Translate(x2.x(), x2.y(), x2.z());
}
