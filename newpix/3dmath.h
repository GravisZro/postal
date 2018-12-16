#ifndef THREEDMATH_H
#define THREEDMATH_H

#include <ORANGE/QuickMath/QuickMath.h>

///////////////////////////////////////////////////////////////////////////////
// Maps a 3D coordinate onto the viewing plane provided the view angle
// (~angle of projection).
///////////////////////////////////////////////////////////////////////////////
template <class TIn, class TOut>
void Map3Dto2D(	// Returns nothing.
   TIn x_in,				// In.
   TIn y_in,				// In.
   TIn z_in,				// In.
   TOut& x_out,			// Out.
   TOut& y_out,			// Out.
   int16_t	sViewAngle)	// In:  View angle in degrees.
   {
   x_out	= x_in;
   y_out	= SINQ[sViewAngle] * z_in - COSQ[sViewAngle] * y_in;
   }

///////////////////////////////////////////////////////////////////////////////
// Scales a Z coordinate onto the viewing plane provided the
// view angle (~angle of projection).
///////////////////////////////////////////////////////////////////////////////
template <class TIn, class TOut>
void MapZ3DtoY2D(		// Returns nothing.
   TIn	z_in,			// In.
   TOut& y_out,		// Out.
   int16_t	sViewAngle)	// In:  View angle in degrees.
   {
   ASSERT(sViewAngle >= 0 && sViewAngle < 360);

   y_out	= SINQ[sViewAngle] * z_in;
   }

///////////////////////////////////////////////////////////////////////////////
// Scales a Y coordinate from the viewing plane provided the
// view angle (~angle of projection).
///////////////////////////////////////////////////////////////////////////////
template <class TIn, class TOut>
void MapY2DtoZ3D(		// Returns nothing.
   TIn	y_in,			// In.
   TOut& z_out,		// Out.
   int16_t	sViewAngle)	// In:  View angle in degrees.
   {
   ASSERT(sViewAngle >= 0 && sViewAngle < 360);

   real_t	rSin	= SINQ[sViewAngle];
   if (rSin != 0.0)
      {
      z_out	= y_in / rSin;
      }
   else
      {
      z_out	= 0;
      }
   }

///////////////////////////////////////////////////////////////////////////////
// Scales a Y coordinate onto the viewing plane provided the
// view angle (~angle of projection).
///////////////////////////////////////////////////////////////////////////////
template <class TIn, class TOut>
void MapY3DtoY2D(		// Returns nothing.
   TIn	y_in,			// In.
   TOut& y_out,		// Out.
   int16_t	sViewAngle)	// In:  View angle in degrees.
   {
   ASSERT(sViewAngle >= 0 && sViewAngle < 360);

   *y_out	= COSQ[sViewAngle] * y_in;
   }

///////////////////////////////////////////////////////////////////////////////
// Scales a Y coordinate from the viewing plane provided the
// view angle (~angle of projection).
///////////////////////////////////////////////////////////////////////////////
template <class TIn, class TOut>
void MapY2DtoY3D(		// Returns nothing.
   TIn	y_in,			// In.
   TOut& y_out,		// Out.
   int16_t	sViewAngle)	// In:  View angle in degrees.
   {
   ASSERT(sViewAngle >= 0 && sViewAngle < 360);

   real_t	rCos	= COSQ[sViewAngle];
   if (rCos != 0.0)
      {
      y_out	= y_in / rCos;
      }
   else
      {
      y_out	= 0;
      }
   }

#endif // THREEDMATH_H
