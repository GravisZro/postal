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
#include <BLUE/System.h>
#include "pipeline.h"

///////////////////////////////////////////////////////////////
// This is the highest level considered actually part of the 3d engine.
// It is the highest level control -> it decides how 3d pts map to 2d.
// You can customize 3d efects by instantiating your own versions of the 3d pipeline!
///////////////////////////////////////////////////////////////
//	PIPELINE - History
///////////////////////////////////////////////////////////////
//
//	07/23/97	JRD	Added support for generating shadows
//
///////////////////////////////////////////////////////////////


uint32_t RPipeLine::ms_lNumPts = 0;
uint32_t	RPipeLine::ms_lNumPipes = 0;

Vector3D*  RPipeLine::ms_pPts = nullptr;

RPipeLine::RPipeLine()
	{
	ms_lNumPipes++; // Track for deletion purposes!
	Init();
	}

void RPipeLine::Init()
	{
	m_pimClipBuf = nullptr;
	m_pimShadowBuf = nullptr;
	m_pZB = nullptr;
	m_sUseBoundingRect = FALSE;
	m_dShadowScale = 1.0;

   m_tScreen.makeIdentity();
   m_tView.makeIdentity();
   m_tShadow.makeIdentity();
	}

// assume the clip rect is identical situation to zBUF:
//
int16_t RPipeLine::Create(size_t lNum,int16_t sW)
	{
	if (sW)
		{
		// Will only overwrite memory if it needs MORE!
		if ((m_pZB != nullptr) && (sW > m_pZB->m_sW))
			{
			delete m_pZB;
			delete m_pimClipBuf;
			m_pimClipBuf = nullptr;
			m_pZB = nullptr;
			}

		if (m_pZB == nullptr)
			{
			m_pZB = new RZBuffer(sW,sW); // no need to clear it yet!
			m_pimClipBuf = new RImage;
			// clear it when appropriate:
			m_pimClipBuf->CreateImage(sW,sW,RImage::BMP8);
			}
		}

	//----------
	if (!lNum) return SUCCESS;
	
	if ((ms_pPts != nullptr) && (lNum > ms_lNumPts))
		{
       delete ms_pPts;
		ms_pPts = nullptr;
		}

	if (ms_pPts == nullptr)
		{
      ms_pPts = new Vector3D[lNum];
		}

	return SUCCESS;
	}

int16_t RPipeLine::CreateShadow(int16_t sAngleY,
						double dTanDeclension,int16_t sBufSize)
	{
	ASSERT( (sAngleY >=0 ) && (sAngleY < 360) );
	ASSERT(dTanDeclension > 0.0);

	// Create the shadow transform:
   m_tShadow.makeIdentity();
	m_dShadowScale = dTanDeclension;
   m_tShadow.matdata[rowcol(0, 1)] = real_t(m_dShadowScale * rspCos(sAngleY));
   m_tShadow.matdata[rowcol(1, 1)] = 0.0;
   m_tShadow.matdata[rowcol(2, 1)] = real_t(m_dShadowScale * rspSin(sAngleY));

	// Allocate the buffer, if applicable:
	if (sBufSize <= 0)	// default case:
		{
		if (m_pimShadowBuf == nullptr) // do a default allocation
			{
			if (m_pimClipBuf) // use as reference:
				{
				m_pimShadowBuf = new RImage;
				m_pimShadowBuf->CreateImage(
					m_pimClipBuf->m_sWidth * 2,
					m_pimClipBuf->m_sWidth * 2,
					RImage::BMP8);
				return SUCCESS;
				}
			TRACE("RPipeLine::CreateShadow: warning - no buffer set!\n");
			return SUCCESS;
			}
		else
			{
			// No allocatione needed
			return SUCCESS;
			}
		}
	else	// specified buffer size:
		{
		if (m_pimShadowBuf)
			{
			if (m_pimShadowBuf->m_sWidth >= sBufSize) 
				{
				return SUCCESS;	// don't need to expand it
				}
			else
				{
				delete m_pimShadowBuf;
				m_pimShadowBuf = nullptr;
				}
			}
		m_pimShadowBuf = new RImage;
		m_pimShadowBuf->CreateImage(sBufSize,sBufSize,RImage::BMP8);
		}

	return SUCCESS;
	}

void RPipeLine::Destroy()
	{
	if (m_pZB) delete m_pZB;
	if (m_pimClipBuf) delete m_pimClipBuf;
	if (m_pimShadowBuf) delete m_pimShadowBuf;
	}

RPipeLine::~RPipeLine()
	{
	Destroy();
	ms_lNumPipes--; // prepare for full death:
	if (!ms_lNumPipes)	// free ms_pPts
		{
		if (ms_pPts) free(ms_pPts);
		ms_lNumPts = 0;
		ms_pPts = nullptr;
		}
	}

void RPipeLine::Transform(RSop* pPts,RTransform& tObj)
	{
   RTransform tFull;
   // Use to stretch to z-buffer!

   tFull.makeIdentity();
   tFull.Mul(m_tView.matdata,tObj.matdata);
	// If there were inhomogeneous transforms, you would need to 
	// trasnform each pt by two transforms separately!
   tFull.PreMulBy(m_tScreen.matdata);

   for (size_t i = 0; i < pPts->points.size(); i++)
		{
      tFull.TransformInto(pPts->points[i],ms_pPts[i]);
		// Note that you can now use RP3d directly with the renderers! 
		}
	}

// Need to create a slightly more complex pipe:
void RPipeLine::TransformShadow(RSop* pPts,RTransform& tObj,
		int16_t sHeight,int16_t *psOffX,int16_t *psOffY)
	{
	ASSERT(m_pimShadowBuf);

   RTransform tFull;
	// Use to stretch to z-buffer!

   tFull.makeIdentity();
	// 1) Create Shadow
   tFull.Mul(m_tShadow.matdata,tObj.matdata);
	// 2) Add in normal view
   tFull.PreMulBy(m_tView.matdata);
	// If there were inhomogeneous transforms, you would need to 
	// trasnform each pt by two transforms separately!

	if (psOffX || psOffY) // calculate shadow offset
		{
		// (1) convert to 3d shadow point:
      Vector3D	pOffset = {0.0,static_cast<float>(sHeight),0.0,};
#ifdef UNUSED_VARIABLES
      double dOffX = sHeight * m_tShadow.data[rowcol(0, 1)];
		double dOffY = 0.0;
      double dOffZ = sHeight * m_tShadow.data[rowcol(2, 1)]; // y is height here
#endif
		// (2) partially project
		m_tShadow.Transform(pOffset);
		m_tView.Transform(pOffset);
		// Undo randy slide:
      Vector3D pTemp = {m_tView.matdata[rowcol(0, 3)],m_tView.matdata[rowcol(1, 3)],
         m_tView.matdata[rowcol(2, 3)]};

      pOffset -= pTemp;
		// Just use screen for scale:
      pOffset.x *= m_tScreen.matdata[rowcol(0, 0)];
      pOffset.y *= m_tScreen.matdata[rowcol(1, 1)];

		// store result
		*psOffX = int16_t (pOffset.x);
		*psOffY = int16_t (pOffset.y);
		}

	// 3) Project to the "screen"
   tFull.PreMulBy(m_tScreen.matdata);

	// 4) Adjust the screen transform to keep scaling and mirroring
	// from normal screen transform, but adjust size for shadow buffer
	// This is hard coded to the postal coordinate system
   tFull.Translate(0.0,m_pimShadowBuf->m_sHeight-m_tScreen.matdata[rowcol(1, 3)],0.0);

   for (size_t i = 0; i < pPts->points.size(); i++)
		{
      tFull.TransformInto(pPts->points[i],ms_pPts[i]);
		// Note that you can now use RP3d directly with the renderers! 
		}

	}

// returns 0 if pts are ClockWise! (Hidden)
// (to be used AFTER the view or screen transformation)
int16_t RPipeLine::NotCulled(Vector3D *p1,Vector3D *p2,Vector3D *p3)
	{
   real_t ax,ay,bx,by;
	ax = p2->x - p1->x;
	ay = p2->y - p1->y;
	bx = p3->x - p1->x;
	by = p3->y - p1->y;

   if ( (ax*by - ay*bx) >= 0)
     return FALSE;
   return TRUE;
	}

// Currently (sDstX,sDstY) allgns with the upper left half of the z-buffer
// Uses the static transformed point buffer.
//
void RPipeLine::Render(RImage* pimDst,int16_t sDstX,int16_t sDstY,
		RMesh* pMesh,uint8_t ucColor) // wire!
   {
	int32_t v1,v2,v3;
   triangle_t* psVertex = pMesh->triangles;
	int32_t lNumHidden = 0;

   for (uint32_t i = 0; i < pMesh->triangles.size(); ++i)
		{
      v1 = psVertex[i][0];
      v2 = psVertex[i][1];
      v3 = psVertex[i][2];

		if (NotCulled(ms_pPts+v1,ms_pPts+v2,ms_pPts+v3))
			{
			// Render the sucker!
			DrawTri_wire(pimDst,sDstX,sDstY,
				ms_pPts+v1,ms_pPts+v2,ms_pPts+v3,ucColor);
			}
		else
			{
			lNumHidden++; // cull debug
			}
		}
	//TRACE("Number culled was %i\n",lNumHidden);
	}

// Currently (sDstX,sDstY) allgns with the upper left half of the z-buffer
// Uses the static transformed point buffer.
//
void RPipeLine::RenderShadow(RImage* pimDst,RMesh* pMesh,uint8_t ucColor)
   {
	int32_t v1,v2,v3;
   triangle_t* psVertex = pMesh->triangles;

   for (uint32_t i = 0; i < pMesh->triangles.size(); ++i)
		{
     v1 = psVertex[i][0];
     v2 = psVertex[i][1];
     v3 = psVertex[i][2];

		if (NotCulled(ms_pPts+v1,ms_pPts+v2,ms_pPts+v3))
			{
			// Render the sucker!
			DrawTri(pimDst->m_pData,pimDst->m_lPitch,
				ms_pPts+v1,ms_pPts+v2,ms_pPts+v3,ucColor);
			}
		}
	}

// YOU clear the z-buffer before this if you so desire!!!
// Currently (sDstX,sDstY) allgns with the upper left half of the z-buffer
// Uses the static transformed point buffer.
//
void RPipeLine::Render(RImage* pimDst,int16_t sDstX,int16_t sDstY,
		RMesh* pMesh,RZBuffer* pZB,RTexture* pTexColors,
		int16_t sFogOffset,RAlpha* pAlpha,
		int16_t sOffsetX/* = 0*/,		// In: 2D offset for pimDst and pZB.
		int16_t sOffsetY/* = 0*/) 	// In: 2D offset for pimDst and pZB.
   {
	int32_t v1,v2,v3;
   triangle_t* psVertex = pMesh->triangles;
   uint8_t *pColor = pTexColors->indexes;
	int32_t lDstP = pimDst->m_lPitch;
	uint8_t* pDst = pimDst->m_pData + (sDstX + sOffsetX) + lDstP * (sDstY + sOffsetY);

   for (uint32_t i = 0; i < pMesh->triangles.size(); ++i, ++pColor)
		{
     v1 = psVertex[i][0];
     v2 = psVertex[i][1];
     v3 = psVertex[i][2];

		if (1)//NotCulled(ms_pPts+v1,ms_pPts+v2,ms_pPts+v3))
			{
			// Render the sucker!
			DrawTri_ZColorFog(pDst,lDstP,
				ms_pPts+v1,ms_pPts+v2,ms_pPts+v3,pZB,
				pAlpha->m_pAlphas[*pColor] + sFogOffset,
				sOffsetX,		// In: 2D offset for pZB.
				sOffsetY);	 	// In: 2D offset for pZB.
			}
		}
	}

// YOU clear the z-buffer before this if you so desire!!!
// Currently (sDstX,sDstY) allgns with the upper left half of the z-buffer
// FLAT SHADE MODE
//
void RPipeLine::Render(RImage* pimDst,int16_t sDstX,int16_t sDstY,
		RMesh* pMesh,RZBuffer* pZB,RTexture* pTexColors,
		int16_t sOffsetX/* = 0*/,		// In: 2D offset for pimDst and pZB.
		int16_t sOffsetY/* = 0*/) 	// In: 2D offset for pimDst and pZB.
   {
	int32_t v1,v2,v3;
   triangle_t* psVertex = pMesh->triangles;
   uint8_t *pColor = pTexColors->indexes;
	int32_t lDstP = pimDst->m_lPitch;
	uint8_t* pDst = pimDst->m_pData + (sDstX + sOffsetX) + lDstP * (sDstY + sOffsetY);

   for (uint32_t i = 0; i < pMesh->triangles.size(); ++i, ++pColor)
		{
     v1 = psVertex[i][0];
     v2 = psVertex[i][1];
     v3 = psVertex[i][2];

		if (NotCulled(ms_pPts+v1,ms_pPts+v2,ms_pPts+v3))
			{
			// Render the sucker!
			DrawTri_ZColor(pDst,lDstP,
				ms_pPts+v1,ms_pPts+v2,ms_pPts+v3,pZB,
				*pColor,
				sOffsetX,		// In: 2D offset for pZB.
				sOffsetY);	 	// In: 2D offset for pZB.
			}
		}
	}

// THIS IS HACKED!  WILL NOT WORK WITH DISTORTED GUYS!
//
void RPipeLine::BoundingSphereToScreen(Vector3D& ptCenter, Vector3D& ptRadius,
		RTransform& tObj)
	{
	// THIS IS HARD WIRED TO WORK WITH OUR CURRENT STYLE OF
	// PROJECTION:
	RTransform tFull;
   tFull.makeIdentity();
   tFull.Mul(m_tView.matdata,tObj.matdata); // hold off on screen -> get raw distance:
   tFull.PreMulBy(m_tScreen.matdata);

	// THIS IS IN UNSCALED OBJECT VIEW
	double dModelRadius = std::sqrt(
		SQR(ptCenter.x - ptRadius.x) + 
		SQR(ptCenter.y - ptRadius.y) + 
		SQR(ptCenter.z - ptRadius.z) ); // Randy Units

	// Convert from Model To Screen...
   double dScreenRadius = dModelRadius * m_tScreen.matdata[0];

	// Project the center onto the screen:

   Vector3D ptCen/*,ptEnd*/;
	tFull.TransformInto(ptCenter,ptCen); // z is now distorted

	// store in pieline variables...(ALL OF THEM)
	m_sCenX = int16_t(ptCen.x);
	m_sCenY = int16_t(ptCen.y);
	m_sCenZ = int16_t(ptCen.z / 256.0); // Scale Z's by 256 for lighting later

	int16_t	sScreenRadius = int16_t(dScreenRadius+1);
	
	m_sX = m_sCenX - sScreenRadius;
	m_sY = m_sCenY - sScreenRadius;
	m_sZ = m_sCenZ - sScreenRadius;

   m_sW = m_sH = (sScreenRadius * 2);
   m_sD = (sScreenRadius * 2);

	m_sUseBoundingRect = TRUE;
	}

void RPipeLine::ClearClipBuffer()
	{
	if (m_pimClipBuf == nullptr) return;

   rspRect(static_cast<uint32_t>(0),m_pimClipBuf,0,0,
		m_pimClipBuf->m_sWidth,m_pimClipBuf->m_sHeight);
	}

void RPipeLine::ClearShadowBuffer()
	{
	if (m_pimShadowBuf == nullptr) return;

   rspRect(static_cast<uint32_t>(0),m_pimShadowBuf,0,0,
		m_pimShadowBuf->m_sWidth,m_pimShadowBuf->m_sHeight);
	}
