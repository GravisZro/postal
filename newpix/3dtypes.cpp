#include "3dtypes.h"

#include <RSPiX/ORANGE/color/colormatch.h>
#include <RSPiX/ORANGE/QuickMath/QuickMath.h>

#include <cstring>

template<typename T, typename S>
inline bool arrays_match(T* a, T* b, S count)
{
  return !count ||                            // if array size is zero OR
         ((a == nullptr) == (b == nullptr) && // (they are the same AND
          (a == nullptr ||                    //  (both are null OR
           std::memcmp(a, b, sizeof(T) * count) == SUCCESS));  //   memory does match))
}

RTexture::RTexture(void)
  : count(0),
    flags(none),
    indexptr(nullptr),
    colorptr(nullptr)
{
}

RTexture::~RTexture(void)
{
  if(allocated(indexptr))
    delete[] indexptr;
  indexptr = nullptr;

  if(allocated(colorptr))
    delete[] colorptr;
  colorptr = nullptr;
}

void RTexture::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const RPixel32* dataptrP32;
  };

  dataptr8 = dataptr;
  count   = *dataptr16++;
  flags   = static_cast<flags_t>(*dataptr16++);

  if(flags & indexes)
  {
    indexptr = const_cast<uint8_t*>(dataptr8);
    dataptr8 += count;
  }
  if(flags & colors)
  {
    colorptr = const_cast<RPixel32*>(dataptrP32);
    dataptrP32 += count;
  }

  size = dataptr8 - dataptr;
  loaded = true;
}

bool RTexture::operator ==(const RTexture& other) const
{
  return count == other.count && // same number of elements AND
         arrays_match(indexptr, other.indexptr, count) && // indexptr arrays match AND
         arrays_match(colorptr, other.colorptr, count); //  colorptr arrays match
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
  ASSERT(colorptr != nullptr);

  if (indexptr == nullptr)
    indexptr = new uint8_t[count];

  for (int16_t i = 0; i < count; i++)
  {
    indexptr[i] = rspMatchColorRGB(
                    int32_t(colorptr[i].red),
                    int32_t(colorptr[i].green),
                    int32_t(colorptr[i].blue),
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
  ASSERT(indexptr != nullptr);

  if (colorptr == nullptr)
    colorptr = new RPixel32[count];

  uint8_t*  pu8    = indexptr;
  RPixel32* ppix   = colorptr;
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
  ASSERT(colorptr);
  ASSERT(fAdjustment >= 0.0f);

  RPixel32* ppix = colorptr;
  int16_t	sCount = count / lInc;
  while (sCount--)
  {
    ppix->red   = clampColorChannel(ppix->red   * fAdjustment);
    ppix->green = clampColorChannel(ppix->green * fAdjustment);
    ppix->blue  = clampColorChannel(ppix->blue  * fAdjustment);
    ppix += lInc;
  }
}


RMesh::RMesh(void)
  : count(0),
    triptr(nullptr)
{
}

RMesh::~RMesh(void)
{
  if(allocated(triptr))
    delete[] triptr;
  triptr = nullptr;
}

void RMesh::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const triangle_t* dataptrTri;
  };

  dataptr8 = dataptr;
  count   = *dataptr16++;
  triptr = const_cast<triangle_t*>(dataptrTri);
  dataptrTri += count;

  size = dataptr8 - dataptr;
  loaded = true;
}

bool RMesh::operator ==(const RMesh& other) const
{
  return count == other.count && // same number of elements AND
         arrays_match(triptr, other.triptr, count); // triptr arrays match
}


RSop::RSop(void)
  : count(0),
    pointptr(nullptr)
{
}

RSop::~RSop(void)
{
  if(allocated(pointptr))
    delete[] pointptr;
  pointptr = nullptr;
}

void RSop::load(void)
{
  union
  {
    const uint8_t*  dataptr8;
    const uint16_t* dataptr16;
    const RP3d*     dataptr3d;
  };

  dataptr8 = dataptr;
  count   = *dataptr16++;
  pointptr = const_cast<RP3d*>(dataptr3d);
  dataptr3d += count;

  size = dataptr8 - dataptr;
  loaded = true;
}

bool RSop::operator ==(const RSop& other) const
{
  return count == other.count && // same number of elements AND
         arrays_match(pointptr, other.pointptr, count); // triptr arrays match
}


inline void MatrixMultiply(real_t* matOut, real_t* matA, real_t* matB, int row, int col)
{
  matOut[(row * 4) + col] =
      matA[(row * 4) + 0] * matB[0x0 + col] +
      matA[(row * 4) + 1] * matB[0x4 + col] +
      matA[(row * 4) + 2] * matB[0x8 + col] +
      matA[(row * 4) + 3] * matB[0xC + col];
}

inline void MatrixScale(real_t* mat, real_t x, real_t y, real_t z, int col)
{
  mat[(0 * 4) + col] *= x;
  mat[(1 * 4) + col] *= y;
  mat[(2 * 4) + col] *= z;
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

#if 0
template<typename Func, typename... Args>
inline void ExecOp4(Func op, Args... args)
{
  op(args..., 0);
  op(args..., 1);
  op(args..., 2);
  op(args..., 3);
}

template<typename Func, typename... Args>
inline void ExecOp3x4(Func op, Args... args)
{
  ExecOp4(op, args..., 0);
  ExecOp4(op, args..., 1);
  ExecOp4(op, args..., 2);
}
#endif


constexpr real_t identity_data[16] = { 1.0,0.0,0.0,0.0,
                                       0.0,1.0,0.0,0.0,
                                       0.0,0.0,1.0,0.0,
                                       0.0,0.0,0.0,1.0 };

RTransform::RTransform(void) // init to an identity transform
{
  size = 16 * sizeof(real_t);
  makeIdentity();
}

RTransform::~RTransform(void)
{
}

void RTransform::makeIdentity(void) // identity matrix
{
  std::memcpy(data, identity_data, sizeof(real_t) * 16);
}

void RTransform::makeNull(void) // null matrix
{
  std::memset(data, 0, sizeof(real_t) * 15);
  data[15] = 1;
}

//------------------------

/*
  void PreMulBy(real_t* M)
  {
    //real_t* MLine = M;
    //real_t* TCol = T;
    //real_t tot;
    int16_t r,c;
    // Unroll this puppy!
    // Much optimizing needed!
    for (r = 0; r < 3; ++r) // 3/4 XFORM!
      for (c = 0; c < 4; ++c)
      {
        data[ROW[r] + c] =
            M[ROW[r]] * data[c] +
            M[ROW[r] + 1] * data[ROW1 + c] +
            M[ROW[r] + 2] * data[ROW2 + c] +
            M[ROW[r] + 3] * data[ROW3 + c];
      }
  }

  // Oversets the current Transform with the resultant!
  // = A * B
  void Mul(real_t* A, real_t* B) // 4x4 transforms:
  {
    int16_t r,c;
    // Unroll this puppy!
    // Much optimizing needed!
    for (r = 0; r < 3; ++r) // 3/4 XFORM!
      for (c = 0; c < 4; ++c)
      {
        data[ROW[r] + c] =
            A[ROW[r] + 0] * B[ROW0 + c] +
            A[ROW[r] + 1] * B[ROW1 + c] +
            A[ROW[r] + 2] * B[ROW2 + c] +
            A[ROW[r] + 3] * B[ROW3 + c];
      }
  }

  void Scale(real_t x,real_t y, real_t z)
  {
    for (int16_t i = 0; i < 4; ++i)
    {
      data[ROW0 + i] *= x;
      data[ROW1 + i] *= y;
      data[ROW2 + i] *= z;
    }
  }
*/


// A Partial transform, assuming R3 = {0,0,0,1};
void RTransform::PreMulBy(real_t* M)
{
  ExecOp3x4(MatrixMultiply, data, M, data);
}

// Oversets the current data with the result!
void RTransform::Mul(real_t* A, real_t* B) // 4x4 transforms:
{
  ExecOp3x4(MatrixMultiply, data, A, B);
}

// Assumes R3 = {0,0,0,1}
void RTransform::Translate(real_t x,real_t y,real_t z)
{
  data[(0 * 4) + 3] += x;
  data[(1 * 4) + 3] += y;
  data[(2 * 4) + 3] += z;
}

void RTransform::Scale(real_t x,real_t y, real_t z)
{
  ExecOp4(MatrixScale, data, x, y, z);
}


// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotTo(RP3d point, RP3d up)
{
  RP3d third;
  point.makeUnit();
  up.makeUnit();
  third = point.multiply(up);

  // store as columns
  makeNull();
  data[(0 * 4) + 0] = point.x;
  data[(1 * 4) + 0] = point.y;
  data[(2 * 4) + 0] = point.z;

  data[(0 * 4) + 1] = up.x;
  data[(1 * 4) + 1] = up.y;
  data[(2 * 4) + 1] = up.z;

  data[(0 * 4) + 2] = third.x;
  data[(1 * 4) + 2] = third.y;
  data[(2 * 4) + 2] = third.z;

}

// This is NOT hyper fast, and the result IS a rotation matrix
// For now, point is it's x-axis and up i s it's y-axis.
void RTransform::MakeRotFrom(RP3d point, RP3d up)
{
  RP3d third;
  point.makeUnit();
  up.makeUnit();
  third = point.multiply(up);

  // store as rows
  makeNull();
  data[(0 * 4) + 0] = point.x;
  data[(0 * 4) + 1] = point.y;
  data[(0 * 4) + 2] = point.z;

  data[(1 * 4) + 0] = up.x;
  data[(1 * 4) + 1] = up.y;
  data[(1 * 4) + 2] = up.z;

  data[(2 * 4) + 0] = third.x;
  data[(2 * 4) + 1] = third.y;
  data[(2 * 4) + 2] = third.z;
}

// Transform an actual point ( overwrites old point )
void RTransform::Transform(RP3d& p) const
{
  RP3d temp;
  temp += p.dot(*reinterpret_cast<const RP3d*>(data + 0));
  temp += p.dot(*reinterpret_cast<const RP3d*>(data + 4));
  temp += p.dot(*reinterpret_cast<const RP3d*>(data + 8));
  p = temp;
}

// Transform an actual point, and places the answer into a different pt
void RTransform::TransformInto(const RP3d& src, RP3d& dest) const
{
  RP3d temp;
  temp += src.dot(*reinterpret_cast<const RP3d*>(data + 0));
  temp += src.dot(*reinterpret_cast<const RP3d*>(data + 4));
  temp += src.dot(*reinterpret_cast<const RP3d*>(data + 8));
  dest = temp;
}

void RTransform::Rz(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (int16_t i = 0; i < 4; ++i)
  {
    register real_t row1 = data[4 + i];
    register real_t row0 = data[0 + i];
    data[4 + i] = row0 * S + row1 * C;
    data[0 + i] = row0 * C - row1 * S;
  }
}

void RTransform::Rx(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (int16_t i = 0; i < 4; ++i)
  {
    register real_t row1 = data[4 + i];
    register real_t row2 = data[8 + i];
    data[8 + i] = row1 * S + row2 * C;
    data[4 + i] = row1 * C - row2 * S;
  }
}

void RTransform::Ry(int16_t sDeg) // CCW!
{
  register real_t S = rspfSin(sDeg);
  register real_t C = rspfCos(sDeg);

  for (int16_t i = 0; i < 4; ++i)
  {
    register real_t row0 = data[0 + i];
    register real_t row2 = data[8 + i];
    data[8 + i] = -row0 * S + row2 * C;
    data[0 + i] =  row0 * C + row2 * S;
  }
}

// a 3d ORTHOGONAL mapping from real_t box1 to box2
// useful in screen and orthogonal view xforms
// Use rspSub to create w vertices (w,h,d)
// x1 BECOMES x2.  Note that w1 must NOT have any 0's.
//
void RTransform::MakeBoxXF(RP3d &x1, RP3d &w1, RP3d &x2, RP3d &w2)
{
  // NOT OF MAXIMUM SPEED!
  makeIdentity();
  Translate(-x1.x, -x1.y, -x1.z);
  Scale(w2.x/w1.x, w2.y/w1.y, w2.z/w1.z);
  Translate(x2.x, x2.y, x2.z);
}
