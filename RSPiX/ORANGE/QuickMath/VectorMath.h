////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 RWS Inc, All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as published by
// the Free Software Foundation
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
////////////////////////////////////////////////////////////////////////////////
//
// VectorMath.h
// Project: RSPiX
//
// This module implements high speed manipulations of standard mathematical
// vectors and matrices.
//
// History:
//		??/??/?? JRD	Started.
//
//		02/11/97	JMI	Added Load() and Save() for compatability with RChannel,
//							RResMgr, and other such file-based thingers.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <BLUE/System.h>
#include <ORANGE/File/file.h>
#include "QuickMath.h"

//===================================
// This file contains data and operations
// for high speed manipulations of standard
// mathematical vectors and matrices.

/*===================================
RP3d; // This is a 3d point.
inline void rspMakeHom(RP3d& p) // do if in question
inline void rspCopy(RP3d& a,RP3d& b)
inline void rspMakeUnit(RP3d& p)
inline real_t rspDot(RP3d& a,RP3d& b)
inline void rspCross(RP3d& a,RP3d& b,RP3d& c) // c = a x b
RTransform 
	{ T[16],
	RTransform() // init to an identity transform
   RTransform(real_t* M) // init to a copy of another transform
	void Make1()
	void Make0()
   void PreMulBy(real_t* M)
   void Mul(real_t* A,real_t* B) // 4x4 transforms:
	void Transform(RP3d &p)
	void TransformInto(RP3d &XF, RP3d& p) XF = this x p
   void Trans(real_t x,real_t y,real_t z)
   void Scale(real_t a,real_t b, real_t c)
	void Rz(short sDeg) // CCW!
	void Rx(short sDeg) // CCW!
	void Ry(short sDeg) // CCW!
	void MakeScreenXF(
      real_t x1,real_t y1,real_t w1,real_t h1,
      real_t x2,real_t y2,real_t w2,real_t h2)
	void MakeRotTo(RP3d point,RP3d up)
	void MakeRotFrom(RP3d point,RP3d up)
	}
//=================================*/

typedef float real_t; // float conserves internal memory..
// Note that if real_t is double, than it should somehow hook
// double sized quick trig, so it doesn't have to do
// conversions each time!

// This is an aggregate type used with RTransform
typedef union
	{
	// 06/30/97 MJR - For the metrowerks compiler, initializations don't
	// seem to work right unless the array comes before the struct!
   real_t v[4];
	struct
		{
      real_t x;
      real_t y;
      real_t z;
      real_t w;
		};

	int16_t Load(RFile* pfile)
		{ return pfile->Read(v, 4) != 4; }

	int16_t Save(RFile* pfile)
		{ return pfile->Write(v, 4) != 4; }

	} Vector3D; // This is a 3d point.

inline int operator==(const Vector3D& lhs, const Vector3D& rhs)
	{
	if (lhs.v == rhs.v)
      return TRUE;
   return FALSE;
	}


// divides out the w component: (makes homogeneous)
inline void rspMakeHom(Vector3D& p)
	{
#ifdef _DEBUG
	if (p.w == 0.0)
		{
		TRACE("FATAL ERROR - POINT AT INFINITY!");
		return;
		}
#endif
	
   real_t w = p.w;
	p.x /= w;
	p.y /= w;
	p.z /= w;
	p.w = 1.00;
	}

// Can be useful:
// adjusts the length of a vector, ignoring w component
inline void rspMakeUnit(Vector3D& p)
	{
   real_t l = std::sqrt(SQR(p.x)+SQR(p.y)+SQR(p.z));
#ifdef _DEBUG
	if (l == 0.0)
		{
      TRACE("FATAL ERROR - nullptr VECTOR!");
		return;
		}
#endif
	p.x /= l;
	p.y /= l;
	p.z /= l;
	}

// returns a dot b
inline real_t rspDot(Vector3D& a,Vector3D& b)
	{
	return a.x*b.x + a.y*b.y + a.z*b.z;
	}

// a = b, does NOT deal with w!
inline void rspCopy(Vector3D& a,Vector3D& b)
	{
	a.x = b.x;
	a.y = b.y;
	a.z = b.z;
	}

// c = a x b
inline void rspCross(Vector3D& a,Vector3D& b,Vector3D& c)
	{
	c.x = a.y * b.z - a.z * b.y;
	c.z = a.x * b.y - a.y * b.x;
	c.y = a.z * b.x - a.x * b.z;
	}

// a -= b;
inline void rspSub(Vector3D& a,Vector3D& b)
	{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	}

// a += b;
inline void rspAdd(Vector3D& a,Vector3D& b)
	{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	}

// a += s;
inline void rspScale(Vector3D& a,real_t s)
	{
	a.x *= s;
	a.y *= s;
	a.z *= s;
	}



// And some useful constants for manipulation:
const int16_t ROW[4] = { 0, 4, 8, 12 };
const int16_t ROW0 = 0;
const int16_t ROW1 = 4;
const int16_t ROW2 = 8;
const int16_t ROW3 = 12;

const real_t Identity[16] = { 1.0,0.0,0.0,0.0,
                              0.0,1.0,0.0,0.0,
                              0.0,0.0,1.0,0.0,
                              0.0,0.0,0.0,1.0 };

// NOW, the class based transform allows matrix
// multiplication to occur WITHOUT multiplying
// 2 matrices together.  This prevents a malloc
// nightmare:
//
class RTransform
	{
public:
   real_t T[16]; // This is compatible with the aggregate transform
	RTransform() // init to an identity transform
		{ 
      for (int16_t i = 0; i < 16; ++i)
			T[i]=Identity[i];
		}

   RTransform(real_t* M) // init to a copy of another transform
		{ 
      for (int16_t i = 0; i < 16; ++i)
			T[i]=M[i];
		}

   ~RTransform(void){}

	int operator==(const RTransform& rhs) const
		{
		if (T == rhs.T)
         return TRUE;
      return FALSE;
		}

	void Make1() // identity matrix
		{
      for (uint8_t i = 0; i < 16; ++i)
			T[i]=Identity[i];
		}

	void Make0() // null matrix
		{
      for (uint8_t i = 0; i < 15; ++i)
         T[i] = 0;
      T[15] = 1;
		}

	//------------------------
	// ALL TRANSFORMATIONS ARE PRE-MULTIPLIES,
	// A Partial transform, assuming R3 = {0,0,0,1};
	// 
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
            T[ROW[r] + c] =
               M[ROW[r]] * T[c] +
               M[ROW[r] + 1] * T[ROW1 + c] +
               M[ROW[r] + 2] * T[ROW2 + c] +
               M[ROW[r] + 3] * T[ROW3 + c];
				}
		}

	// Oversets the current Transform with the resultant!
	// = A * B
   void Mul(real_t* A,real_t* B) // 4x4 transforms:
		{
		int16_t r,c;
		// Unroll this puppy!
		// Much optimizing needed!
      for (r = 0; r < 3; ++r) // 3/4 XFORM!
         for (c = 0; c < 4; ++c)
				{
            T[ROW[r] + c] =
               A[ROW[r]] * B[c] +
               A[ROW[r] + 1] * B[ROW1 + c] +
               A[ROW[r] + 2] * B[ROW2 + c] +
               A[ROW[r] + 3] * B[ROW3 + c];
				}
		}

	// Transform an actual point ( overwrites old point )
	// Doex a premultiply!
	void Transform(Vector3D &p)
		{
      Vector3D temp = { 0.0, 0.0, 0.0, 1.0 }; // asume 3 row form!
      real_t *pT = T,*pV;
		int16_t i,j;

      for (j = 0; j < 3; ++j) // asume 3 row form!
         for (i = 0,pV = p.v; i < 4; ++i)
            temp.v[j] += (*pV++) * (*pT++);
		// overwrite original
      for (i = 0; i < 4; ++i)
        p.v[i] = temp.v[i];
		}

	// Transform an actual point, and places the answer into a different pt
	// Doex a premultiply!
	void TransformInto(Vector3D& vSrc, Vector3D& vDst)
		{
      vDst.v[0] = vDst.v[1] = vDst.v[2] = 0.0;
      vDst.v[3] = 1.0;

      real_t *pT = T,*pV;
		int16_t i,j;

		for (j=0;j<3;j++) // asume 3 row form!
			for (i=0,pV = vSrc.v;i<4;i++)
				{
				vDst.v[j] += (*pV++) * (*pT++);
				}
		}

	// Assumes R3 = {0,0,0,1}
   void Trans(real_t x,real_t y,real_t z)
		{
      T[ROW0 + 3] += x;
      T[ROW1 + 3] += y;
      T[ROW2 + 3] += z;
		}

   void Scale(real_t a,real_t b, real_t c)
		{
      for (int16_t i = 0; i < 4; ++i)
			{
         T[ROW0 + i] *= a;
         T[ROW1 + i] *= b;
         T[ROW2 + i] *= c;
			}
		}

	void Rz(int16_t sDeg) // CCW!
		{
      real_t S = rspfSin(sDeg);
      real_t C = rspfCos(sDeg);
      real_t NewVal; // two vertical numbers depend on each other

      for (int16_t i = 0; i < 4; ++i)
         {
         NewVal = T[ROW0 + i] * C - T[ROW1 + i] * S;
         T[ROW1 + i] = T[ROW0 + i] * S + T[ROW1 + i] * C;
         T[ROW0 + i] = NewVal;
			}
		}

	void Rx(int16_t sDeg) // CCW!
		{
      real_t S = rspfSin(sDeg);
      real_t C = rspfCos(sDeg);
      real_t NewVal; // two vertical numbers depend on each other

      for (int16_t i = 0; i < 4; ++i)
			{
         NewVal = T[ROW1 + i] * C - T[ROW2 + i] * S;
         T[ROW2 + i] = T[ROW1 + i] * S + T[ROW2 + i] * C;
         T[ROW1 + i] = NewVal;
			}
		}

	void Ry(int16_t sDeg) // CCW!
		{
      real_t S = rspfSin(sDeg);
      real_t C = rspfCos(sDeg);
      real_t NewVal; // two vertical numbers depend on each other

      for (int16_t i = 0; i < 4; ++i)
			{
         NewVal = T[ROW0 + i] * C + T[ROW2 + i] * S;
         T[ROW2 + i] = -T[ROW0 + i] * S + T[ROW2 + i] * C;
         T[ROW0 + i] = NewVal;
			}
		}

   // a 3d ORTHOGONAL mapping from real_t box1 to box2
	// useful in screen and orthogonal view xforms
	// Use rspSub to create w vertices (w,h,d)
	// x1 BECOMES x2.  Note that w1 must NOT have any 0's.
	//
	void MakeBoxXF(Vector3D &x1,Vector3D &w1,Vector3D &x2,Vector3D &w2)
		{
		// NOT OF MAXIMUM SPEED!
		Make1();
		Trans(-x1.x,-x1.y,-x1.z);
		Scale(w2.x/w1.x,w2.y/w1.y,w2.z/w1.z);
		Trans(x2.x,x2.y,x2.z);
		}

	// This is NOT hyper fast, and the result IS a rotation matrix
	// For now, point is it's x-axis and up i s it's y-axis.
	void MakeRotTo(Vector3D point,Vector3D up)
		{
		Vector3D third;

		rspMakeUnit(point);
		rspMakeUnit(up);
		rspCross(third,point,up);
		// store as columns
		Make0();
      T[0 + ROW[0]] = point.x;
      T[0 + ROW[1]] = point.y;
      T[0 + ROW[2]] = point.z;

      T[1 + ROW[0]] = up.x;
      T[1 + ROW[1]] = up.y;
      T[1 + ROW[2]] = up.z;

      T[2 + ROW[0]] = third.x;
      T[2 + ROW[1]] = third.y;
      T[2 + ROW[2]] = third.z;

		}

	// This is NOT hyper fast, and the result IS a rotation matrix
	// For now, point is it's x-axis and up i s it's y-axis.
	void MakeRotFrom(Vector3D point,Vector3D up)
		{
		Vector3D third;

		rspMakeUnit(point);
		rspMakeUnit(up);
		rspCross(third,point,up);
		// store as rows
		Make0();
      T[0 + ROW[0]] = point.x;
      T[1 + ROW[0]] = point.y;
      T[2 + ROW[0]] = point.z;

      T[0 + ROW[1]] = up.x;
      T[1 + ROW[1]] = up.y;
      T[2 + ROW[1]] = up.z;

      T[0 + ROW[2]] = third.x;
      T[1 + ROW[2]] = third.y;
      T[2 + ROW[2]] = third.z;
		}

	// Loads instance data for this Transform from the specified
	// file.
	int16_t Load(				// Returns 0 on success.
		RFile*	pfile)	// In:  Ptr to file to load from.  Must be open with
								// read access.
		{
		// Read the entire matrix in one kerchunk.  Bang!  I mean Kerchunk!
		pfile->Read(T, sizeof(T) / sizeof(T[0]) );
		// Success can be measured in terms of I/O errors.
		// Also, we should not have read past the end yet.
		return pfile->Error() | pfile->IsEOF();
		}

	// Saves instance data for this Transform to the specified
	// file.
	int16_t Save(				// Returns 0 on success.
		RFile*	pfile)	// In:  Ptr to file to save to.  Must be open with
								// write access.
		{
		// Write the entire matrix in one kerchunk.  Bang!  I mean Kerchunk!
		pfile->Write(T, sizeof(T) / sizeof(T[0]) );
		// Success can be measured in terms of I/O errors.
		return pfile->Error();
		}

	};


//===================================
#endif
