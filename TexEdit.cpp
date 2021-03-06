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
// TexEdit.cpp
// Project: Nostril (aka Postal)
//
// This module implements the texture editor.
//
// History:
//		10/03/99 JMI	Started.
//
//		10/06/99	JMI	Now DoModal() accepts two lights to use and some other 
//							stuff too.
//
//		10/07/99	JMI	Changed the default Mudify settings to that of Assets.mak.
//
//					JMI	Replaced m_fAlt and m_fAzi with a transform so rotations
//							can always be relative to the current orientation.
//							Also added 'D' as an additional paint key so those keys
//							could all be on one hand.
//
//		10/08/99	JMI	Holding down shift now causes the manips to ignore the Y
//							input and holding control causes them to ignore the X 
//							input.
//
//					JMI	Added ValidateTextures() to check for and fix out-of-synch
//							texture files.
//							In Adjust(), we now free the colors array (otherwise, the
//							colors will get saved).  Some probably already exist.
// 
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Includes.
//------------------------------------------------------------------------------

#include <RSPiX.h>

#include "TexEdit.h"

#include "Anim3D.h"
#include "update.h"
#include "game.h"	// For Paths.
#include "localize.h"

//------------------------------------------------------------------------------
// Macros.
//------------------------------------------------------------------------------

const RString	c_strGuiFile	= "res/editor/TexEdit.gui";

const int32_t		c_lIdAnim			= 100;
const int32_t		c_lIdPal				= 200;
const	int32_t		c_lIdSpotLight		= 601;
const	int32_t		c_lIdBrightness	= 602;
const	int32_t		c_lIdAdjust			= 701;
const	int32_t		c_lIdFrequency		= 702;
const	int32_t		c_lIdAmount			= 703;
const int32_t		c_lIdCurColor		= 201;
const int32_t		c_lIdStatus			= 500;
const	int32_t		c_lIdApply			= 301;
const	int32_t		c_lIdSave			= 302;
const	int32_t		c_lIdRevert			= 303;
const int32_t		c_lIdQuit			= 399;
const int32_t		c_lIdTrans			= 401;
const int32_t		c_lIdScale			= 402;
const int32_t		c_lIdRotate			= 403;
const int32_t		c_lIdPaint			= 404;

const double	c_dScale			= 8.0;

const double	c_fTransRate	= 0.25f / c_dScale;
const double	c_fScaleRate	= 0.1f / c_dScale;
const double	c_fRotRate		= 1.0f;

const int16_t		c_sPalStart		= 106;
const int16_t		c_sPalEnd		= 201;
const	int16_t		c_sPalNum		= c_sPalEnd - c_sPalStart + 1;

#if 0	// Swatch center colors only -- to guarantee good matching.
const short		c_sPalColorsPerSwatch	= 8;
const short		c_sPalCols		= 2;
#else	// All swatch colors -- to allow more flexibility when texturing guys.
const int16_t		c_sPalColorsPerSwatch	= 1;
const int16_t		c_sPalCols		= 8;
#endif

const int16_t		c_sPalDispColorOffset	= c_sPalColorsPerSwatch / 2;
const int16_t		g_sPalFirstColor			= c_sPalStart + c_sPalDispColorOffset;

const	int16_t		c_sMinBrightness		= -64;
const	int16_t		c_sMaxBrightness		= 64;
const int16_t		c_sBrightnessRange	= c_sMaxBrightness - c_sMinBrightness + 1;

const	int16_t		c_sDefAdjustFrequency	= 2;
const	float		c_fDefAdjustment			= 0.85f;


//------------------------------------------------------------------------------
// Functions.
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Set the state of the specified push button.
////////////////////////////////////////////////////////////////////////////////
inline
void
SetPushBtnState(
    RGuiItem* pguiRoot,		// In:  Root GUI.
    int32_t lBtnId,				// In:  ID of btn whose state will be set.
    RPushBtn::State state)	// In:  New state for btn.
{
  ASSERT(pguiRoot);
  RPushBtn*	pbtn = (RPushBtn*)pguiRoot->GetItemFromId(lBtnId);
  if (pbtn)
  {
    pbtn->m_state	= state;
    pbtn->Compose();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Checks to see if v1 and v2 are going in the same sign direction.
////////////////////////////////////////////////////////////////////////////////
bool					// Returns true if x, y, and z of v1 and v2 are the same (respective 3) signs.
SameSigns_Vector(
    Vector3D *v1, 	// In: vector 1
    Vector3D *v2)	// In: vector 1
{
  //this is a slooow check!, check the sign bit, or something
  if(((int)v1->x() >= 0 && (int)v2->x() >= 0) || ((int)v1->x() < 0 && (int)v2->x() < 0))
  {
    if(((int)v1->y() >= 0 && (int)v2->y() >= 0) || ((int)v1->y() < 0 && (int)v2->y() < 0))
    {
      if(((int)v1->z() >= 0 && (int)v2->z() >= 0) || ((int)v1->z() < 0 && (int)v2->z() < 0))
        return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Using the equation in comments below, this returns the value for 't' from the 
//	parametric equations of line seg 0--1 and the plane of a, b, c, and D.
// if true, then you can use t, else, t will not have been assigned anything.
//         -(a*x0 + b*y0 + c*z0 + D)        = t
//	      a*(x1-x0)+b*(y1-y0)+c*(z1-z0)
////////////////////////////////////////////////////////////////////////////////
bool				// Returns: true of t valid, false otherwise
GetIntersectSubT(
    Vector3D &pt0, // In: line segment point 0
    Vector3D &ptSub, // In: line segment point 1
    Vector3D &ptCoeff,
    //double a, double b, double c, double D,
    // In: coefficients
    float fD,
    float &t)  // In: return t
{
  float fdenom=ptCoeff.x()*(ptSub.x()) + ptCoeff.y()*(ptSub.y()) + ptCoeff.z()*(ptSub.z());
  if(fdenom!=0)
  {
    t= (ptCoeff.x()*pt0.x() + ptCoeff.y()*pt0.y() + ptCoeff.z()*pt0.z() + fD)/fdenom;
    t*=-1;
    return true;
  }
  else
  {
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Checks to see if a point is inside (in the plane of and contained in) the
// given triangle.
////////////////////////////////////////////////////////////////////////////////
bool					// Return true if the given point is inside the given triangle
PointInsideTri(
    Vector3D& pt0A, 	// In: point 1 of triangle
    Vector3D& pt1A, 	// In: point 2 of triangle
    Vector3D& pt2A, 	// In: point 3 of triangle
    Vector3D& hitpoint)	// In: point to check inside triangle
{
  /*
   if(cheapequalsRP3d(pt0A, hitpoint)
      || cheapequalsRP3d(pt1A, hitpoint)
      || cheapequalsRP3d(pt2A, hitpoint))
      return true;
   */


  Vector3D pt0, pt1, pt2;//pt0=pt0A, pt1=pt1A, pt2=pt2A;
  Vector3D main_cross;
  Vector3D result_cross;

  //if(cheapequalsRP3d(pt0, hitpoint) || cheapequalsRP3d(pt1, hitpoint)
  //	|| cheapequalsRP3d(pt2, hitpoint))
  //	return true;

  pt0 = pt0A - pt2A;
  pt1 = pt1A - pt2A;
  main_cross = pt0.cross(pt1);


  Vector3D hit=hitpoint;


  //pt2A is the origin
  pt0 = pt0A - pt2A;
  hitpoint -= pt2A;
  result_cross = pt0.cross(hitpoint); //check 0 to hitpoint

  if(SameSigns_Vector(&result_cross, &main_cross))
  {
    //pt0 is the origin
    pt1 = pt1A - pt0A;
    hitpoint = hit - pt0A;
    result_cross = pt1.cross(hitpoint); //check 1 to hitpoint

    if(SameSigns_Vector(&result_cross, &main_cross))
    {
      //pt1 is the origin
      pt2 = pt2A - pt1A;
      hitpoint = hit - pt1A;
      result_cross = pt2.cross(hitpoint); //check 2 to hitpoint

      if(SameSigns_Vector(&result_cross, &main_cross))
      {
        hitpoint=hit;
        return true;//point is definitely inside the triangle!
      }//check 2
    }//check 1
  }//check 0
  hitpoint=hit;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Checks if a line segment and a triangle intersect
////////////////////////////////////////////////////////////////////////////////
bool							// Returns true if the two shapes intersect.
TriAIntersectsLineSegmentB(
    Vector3D & normalA,		// In: normal vector of triangle A
    float fBigA,				// In: constant from parametric equation of A
    Vector3D & pt0A,			// In: three points of triangle A
    Vector3D & pt1A,
    Vector3D & pt2A,
    Vector3D & pt0B,			// In: two points of line segment B
    Vector3D & pt1B,
    Vector3D & hitpoint)		// Out: Exact point where line hits triangle
{
  float intersect_t;
  // point 0--1
  Vector3D ptSub;
  ptSub = pt1B - pt0B;

  if(GetIntersectSubT(pt0B, ptSub, normalA, fBigA, intersect_t))
  {
    if(intersect_t>=0 &&
       intersect_t<=1)
    {
      //GetIntersectPointSub(pt0B, ptSub, intersect_t, hitpoint);
      hitpoint = (pt0B + ptSub) * intersect_t;

      if(PointInsideTri(pt0A, pt1A, pt2A, hitpoint))
        return true;
    }
  }

  //hitpoint.x=hitpoint.y=hitpoint.z=0;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool shorterdist(Vector3D &pt1, Vector3D &pt2)
// true if pt1 is shorter dist than pt2
{
  return ((pt1.x()*pt1.x() + pt1.y()*pt1.y() + pt1.z()*pt1.z()) < (pt2.x()*pt2.x() + pt2.y()*pt2.y() + pt2.z()*pt2.z()));
}
void sqrdist(Vector3D &pt1, float &fdist)
// returns sqrd dist of point
{
  fdist=(pt1.x()*pt1.x() + pt1.y()*pt1.y() + pt1.z()*pt1.z());
}
////////////////////////////////////////////////////////////////////////////////
bool
TrianglesIntersectLineSegment(
    Vector3D &linept1,				// In: line segment point 1
    Vector3D &linept2, 			// In: line segment point 2 closest to this point. this should be the first point.
    vertex_t* ptri, 					// In: mesh
    Vector3D* soparr, 				// In: points for mesh
    int16_t smeshNum,			// In: number of points in mesh
    Vector3D &hitpoint,			// Out: point where line hit triangle
    int32_t &lHitTriIndex)		// Out: index of triangle it hit

{
  lHitTriIndex=-1;
  int16_t sJ=0;
  Vector3D normalA;
  Vector3D pt0A, pt1A, pt2A;
  Vector3D t_pt1A, t_pt2A;
  float fBigA=-1;
  Vector3D ptclosest, ptwork;
  float fclosest=(float)INT_MAX, fdist;
  bool bhit=false;

  hitpoint.setX(0.0);
  hitpoint.setY(0.0);
  hitpoint.setZ(0.0);
  hitpoint.setW(1.0);
  normalA.setW(1.0);

  for(sJ=0; sJ<smeshNum; sJ++)
  {
    //assign points
    pt0A=soparr[ptri[sJ][0]];
    pt1A=soparr[ptri[sJ][1]];
    pt2A=soparr[ptri[sJ][2]];

    //get the normals of triangle 1
    t_pt1A = pt1A - pt0A;
    t_pt2A = pt2A - pt0A;
    normalA = t_pt1A.cross(t_pt2A);

    //and the constant
    fBigA=(-normalA.x()*pt0A.x() - normalA.y()*pt0A.y() - normalA.z()*pt0A.z());

    // Linesegment B intersects polygon A -------------------------------------------------------------------
    if(TriAIntersectsLineSegmentB(normalA, fBigA, pt0A, pt1A, pt2A, linept1, linept2, hitpoint))
    {
      ptwork = linept2;
      ptwork -= hitpoint;
      sqrdist(ptwork, fdist);
      if(fdist < fclosest)
      {
        fclosest=fdist;
        lHitTriIndex=sJ;
        ptclosest = hitpoint;
        bhit=true;
      }
      //return true;
    }
  }//sJ for-loop

  if(bhit)
  {
    hitpoint = ptclosest;
  }
  return bhit;
}


////////////////////////////////////////////////////////////////////////////////
// Transform psopSrc into psopDst with the supplied transform in addition to
// the screen/view pipeline.
////////////////////////////////////////////////////////////////////////////////
void Transform(RSop* psopSrc, RSop* psopDst, RPipeLine* ppipe, RTransform& tObj)
{
#if 0
  RTransform tFull;
  // Use to stretch to z-buffer!

  tFull.makeIdentity();
  tFull.Mul(ppipe->m_tView.matdata,tObj.matdata);
  // If there were inhomogeneous transforms, you would need to
  // trasnform each pt by two transforms separately!
  tFull.PreMulBy(ppipe->m_tScreen.matdata);
  // Add this in to get the model in the correctly offset spot.
  const int16_t sMisUnderstoodValueX	= -85;	// *** WTF is this not sOffsetX?  No time for that now.
  const int16_t sMisUnderstoodValueY	= -20;	// *** WTF is this not sOffsetY?  No time for that now.

#if 0	// Find the magic offset.
  static short sOffX = 0;
  static short sOffY = 0;
  static uint8_t*	pau8KeyStatus = rspGetKeyStatusArray();
  if (pau8KeyStatus[RSP_SK_LEFT] & 1)
    sOffX--;
  if (pau8KeyStatus[RSP_SK_RIGHT] & 1)
    sOffX++;
  if (pau8KeyStatus[RSP_SK_UP] & 1)
    sOffY--;
  if (pau8KeyStatus[RSP_SK_DOWN] & 1)
    sOffY++;
  tFull.Translate(sMisUnderstoodValueX + sOffX, sMisUnderstoodValueY + sOffY, 0);
#else
  tFull.Translate(sMisUnderstoodValueX, sMisUnderstoodValueY, 0);
#endif


  for (size_t i = 0; i < psopSrc->points.size(); i++)
  {
    tFull.TransformInto(psopSrc->points[i], psopDst->points[i]);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Create a 3D palette in the specified GUI.
////////////////////////////////////////////////////////////////////////////////
void
CreatePalette(
    RGuiItem* pgui)		// In:  GUI to contain palette.
{
  int16_t sCells	= c_sPalNum / c_sPalColorsPerSwatch;
  int16_t	sCols		= c_sPalCols;
  int16_t	sRows		= sCells / sCols;

  RRect	rcClient;
  pgui->GetClient(&rcClient.sX, &rcClient.sY, &rcClient.sW, &rcClient.sH);

  int16_t sCellW	= rcClient.sW / sCols;
  int16_t	sCellH	= rcClient.sH / sRows;

  int16_t	sRow, sCol;
  int16_t	sX, sY;
  int16_t sColor = g_sPalFirstColor;
  for (sRow = 0, sY = rcClient.sY; sRow < sRows; sRow++, sY += sCellH)
  {
    for (sCol = 0, sX = rcClient.sX; sCol < sCols; sCol++, sX += sCellW)
    {
      rspRect(
            sColor,
            &pgui->m_im,
            sX, sY,
            sCellW, sCellH,
            nullptr);

      sColor += c_sPalColorsPerSwatch;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Set the specified GUI's text and recompose it to reflect the change.
////////////////////////////////////////////////////////////////////////////////
void
SetText(
    RGuiItem* pguiRoot,		// In:  Root GUI.
    int32_t	lId,					// In:  ID of GUI to update.
    const char* pszFrmt,		// In:  Format specifier ala sprintf.
    ...)							// In:  Arguments as specified by format.
{
  RGuiItem*	pgui = pguiRoot->GetItemFromId(lId);
  if (pgui)
  {
    va_list val;
    va_start(val, pszFrmt);

    vsprintf(pgui->m_szText, pszFrmt, val);

    // Recompose with new text.
    pgui->Compose();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Some texture files were out of date when Special Delivery was made.  As a
// result, it's _possible_ (however unlikely) that a crash could occur when
// reading the ends of the gramps textures.
//
// It appears the problem is that a cane was added to the gramps' meshes after
// the extra various textures were exported but the extra textures were never
// re-exported so they don't contain enough entries for the cane.
////////////////////////////////////////////////////////////////////////////////
void
ValidateTextures(
    RTexture*	ptex,	// In:  Texture to validate.
    int16_t			sNum)	// In:  Number of textures it should have.
{
  if (ptex->size() < sNum)
  {
    int16_t sResult = rspMsgBox(
                        RSP_MB_ICN_QUERY | RSP_MB_BUT_YESNO,
                        "Incorrect Texture File",
                        "This texture file does not have enough entries to cover the entire mesh.\n"
                        "Do you want this utility to recreate the texture scheme filling in the\n"
                        "empty entries with bright green?");
    switch (sResult)
    {
      case RSP_MB_RET_YES:
      {
        int16_t	sOrigNum	= ptex->size();

        // Create temp space for the existing colors.
        uint8_t* pau8 = new uint8_t[sOrigNum];

        // Duplicate the existing colors.
        int16_t sColor;
        for (sColor = 0; sColor < sOrigNum; sColor++)
        {
          pau8[sColor] = ptex->getIndex(sColor);//->indexes[sColor];
        }

        // Free the existing colors.
        //ptex->FreeIndices();

        // Resize.
        ptex->setSize(sNum);
        //ptex->AllocIndices();

        // Copy the original colors back.
        for (sColor = 0; sColor < sOrigNum; sColor++)
        {
          ptex->setIndex(sColor, pau8[sColor]);
        }

        // Fill the remaining colors as bright green.
        for ( ; sColor < sNum; sColor++)
        {
          ptex->setIndex(sColor, 250); // Part of static Postal palette.
        }

        delete[] pau8;
        pau8 = nullptr;

        break;
      }
      case RSP_MB_RET_NO:
        break;
    }
  }
}


//------------------------------------------------------------------------------
// Construction.
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Default constructor.
////////////////////////////////////////////////////////////////////////////////
CTexEdit::CTexEdit(void)
{
  m_pguiRoot			= nullptr;
  m_pguiAnim			= nullptr;
  m_pguiCurColor		= nullptr;
  m_pguiPal			= nullptr;

  m_scene.SetupPipeline(nullptr, nullptr, c_dScale);

  m_manip	= Trans;

  m_bDragging = false;

  m_sCursorResetX = 0;
  m_sCursorResetY = 0;

  ResetTransformation();

  m_bQuit	= false;

  m_u8Color	= g_sPalFirstColor;

  m_lTriIndex	= -1;

  m_ptexSrc		= nullptr;
  //m_ptexchanSrc	= nullptr;

  m_bModified	= false;

  m_bSpotLight	= false;
  m_sBrightness	= 0;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
////////////////////////////////////////////////////////////////////////////////
CTexEdit::~CTexEdit(void)
{
}

//------------------------------------------------------------------------------
// Methods.
//------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Edit the texture of the specified 3D animation in a modal dialog.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::DoModal(
    CAnim3D* panim,			// In:  3D animation to paint on.
    RAlpha* pltAmbient,		// In:  Ambient light.
    RAlpha* pltSpot,			// In:  Spot light.
    const RString& strFile)	// In:  Filename to save modified texture as.
{
#if 0
  m_pguiRoot = RGuiItem::LoadInstantiate(FullPathVD(c_strGuiFile) );
  if (m_pguiRoot)
  {
    RProcessGui gm;

    // Get composite buffer.
    RImage* pimComposite;
    rspNameBuffers(&pimComposite);

    // Remember center.
    m_sCursorResetX	= pimComposite->m_sWidth / 2;
    m_sCursorResetY	= pimComposite->m_sHeight / 2;

    // Center.
    m_pguiRoot->Move(
          pimComposite->m_sWidth / 2 - m_pguiRoot->m_im.m_sWidth / 2,
          pimComposite->m_sHeight / 2 - m_pguiRoot->m_im.m_sHeight / 2);

    // Get the 3D render surface.
    m_pguiAnim = m_pguiRoot->GetItemFromId(c_lIdAnim);
    ASSERT(m_pguiAnim);

    // Get the palette surface.
    m_pguiPal = m_pguiRoot->GetItemFromId(c_lIdPal);
    ASSERT(m_pguiPal);

    CreatePalette(m_pguiPal);

    m_pguiCurColor = m_pguiRoot->GetItemFromId(c_lIdCurColor);
    ASSERT(m_pguiCurColor);

    SetColor(g_sPalFirstColor);

    m_bSpotLight	= false;

    // Get light to use.
    RRect rcClient;
    m_pguiAnim->GetClient(&rcClient.sX, &rcClient.sY, &rcClient.sW, &rcClient.sH);

    int16_t sOffsetX = rcClient.sX + rcClient.sW / 2;
    int16_t sOffsetY = rcClient.sY + rcClient.sH - 5;

    ResetTransformation();

    SetManip(Paint);

    // Even though there's many channels of many textures per person, they are all the
    // very same resource (on disk & in memory).
    m_ptexchanSrc	= panim->m_ptextures;
    m_ptexSrc		= &m_ptexchanSrc->atTime(0);

    // Validate texture thinger.
    ValidateTextures(m_ptexSrc, panim->m_pmeshes->atTime(0).triangles.size());


    // Duplicate into a care-free work area.
    m_texWork	= *m_ptexSrc;

    m_bModified	= false;

    m_strFileName	= strFile;

    // Set notifications.
    SetToNotify(c_lIdQuit,			QuitCall_Static);
    SetToNotify(c_lIdTrans,			ManipCall_Static);
    SetToNotify(c_lIdScale,			ManipCall_Static);
    SetToNotify(c_lIdRotate,		ManipCall_Static);
    SetToNotify(c_lIdPaint,			ManipCall_Static);
    SetToNotify(c_lIdPal,			ColorCall_Static);
    SetToNotify(c_lIdApply,			ApplyCall_Static);
    SetToNotify(c_lIdSave,			SaveCall_Static);
    SetToNotify(c_lIdRevert,		RevertCall_Static);
    SetToNotify(c_lIdSpotLight,	SpotCall_Static);
    SetToNotify(c_lIdBrightness,	BrightnessCall_Static);
    SetToNotify(c_lIdAdjust,		AdjustCall_Static);
    SetToNotify(c_lIdAnim,			AnimCall_Static);

    // Set initial brightness.
    RScrollBar*	psb	= (RScrollBar*)m_pguiRoot->GetItemFromId(c_lIdBrightness);
    ASSERT(psb);
    ASSERT(psb->m_type == RGuiItem::ScrollBar);
    psb->SetPos(50);

    SetText(m_pguiRoot, c_lIdFrequency, "%hd", c_sDefAdjustFrequency);
    SetText(m_pguiRoot, c_lIdAmount, "%g", c_fDefAdjustment);

    // Get up to two controls that can end the processing that can be
    // passed on the DoModal() line.  More buttons can be set though.
    // Set up ptrs and erase buffer.
    gm.Prepare(m_pguiRoot, nullptr, nullptr);

    RInputEvent	ie;

    m_bQuit	= false;

    m_lTriIndex	= -1;

    RTransform	trans;
    CSprite3		sprite;
    RSop			sopView;
    int32_t			lTime = 0;

    // Process GUI.
    while (m_bQuit == false)
    {
      // System Update //////////////////////////////////////
      ie.type	= RInputEvent::None;

      // Critical callage.
      UpdateSystem();
      rspGetNextInputEvent(&ie);

      // Setup current 3D info ///////////////////////////////
      sprite.m_pmesh			= &panim->m_pmeshes->atTime(lTime);
      sprite.m_psop			= &panim->m_psops->atTime(lTime);
      sprite.m_ptrans		= &trans;
      sprite.m_ptex			= &m_texWork;
      sprite.m_psphere		= &panim->m_pbounds->atTime(lTime);
      sprite.m_sBrightness	= m_sBrightness;

      // Transformation //////////////////////////////////////
      trans.makeIdentity();
      ComposeTransform(trans);

      // View Space SOP //////////////////////////////////////

      // Create View Space SOP for checking mouse against triangles and
      // drawing feedback for such.
      if (sopView.points.size() != sprite.m_psop->points.size())
        sopView.points.allocate(sprite.m_psop->points.size());
        //sopView.Alloc(sprite.m_psop->size());

      Transform(sprite.m_psop, &sopView, &m_scene.m_pipeline, trans);

      // Process User Manips /////////////////////////////////

      ProcessManip(m_pguiAnim->m_sPressed != 0, &ie, &sprite, &sopView);

      // Output //////////////////////////////////////////////
      rspRect(
            0,
            &m_pguiAnim->m_im,
            0, 0,
            m_pguiAnim->m_im.m_sWidth,
            m_pguiAnim->m_im.m_sHeight,
            &rcClient);

      DoOutput(&sprite, &sopView, trans, m_bSpotLight ? pltSpot : pltAmbient, &m_pguiAnim->m_im, sOffsetX, sOffsetY, rcClient);

      gm.DoModeless(m_pguiRoot, &ie, pimComposite);

      // If quiting . . .
      if (m_bQuit)
      {
        // If modified . . .
        if (m_bModified)
        {
          // Query if user wants to apply the work textures (and not lose changes).
          int16_t sResult	= rspMsgBox(
                               RSP_MB_ICN_QUERY | RSP_MB_BUT_YESNOCANCEL,
                               g_pszAppName,
                               "Apply changes before exiting texture editor?");

          switch (sResult)
          {
            case RSP_MB_RET_YES:		// Yes - apply.
              Apply();
              break;
            case RSP_MB_RET_NO:		// No - don't apply.
              break;
            case RSP_MB_RET_CANCEL:	// Cancel - don't quit.
              m_bQuit	= false;
              rspSetQuitStatus(FALSE);
              break;
          }
        }
      }
    }

    // Clean up ptrs, erase buffer, and dirty rect list.
    gm.Unprepare();

    m_ptexSrc		= nullptr;
    //m_ptexchanSrc	= nullptr;
    //m_texWork.FreeIndices();

    delete m_pguiRoot;
    m_pguiRoot = nullptr;
    m_pguiAnim = nullptr;
    m_pguiCurColor = nullptr;
    m_pguiPal = nullptr;
  }
  else
  {
    rspMsgBox(
          RSP_MB_ICN_STOP | RSP_MB_BUT_OK,
          g_pszAppName,
          g_pszFileOpenError_s,
          FullPathVD(c_strGuiFile) );
  }
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Render the 3D animation at the specified time.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::DoOutput(
    CSprite3* psprite,	// In:  3D data to render.
    RSop* psopView,		// In:  View space SOP.
    RTransform& trans,	// In:  Transformation.
    RAlpha* palphaLight,	// In:  Light.
    RImage* pimDst,		// In:  Destination for result.
    int16_t sOffsetX,		// In:  X offset.
    int16_t sOffsetY,		// In:  Y offset.
    RRect& rcClip)			// In:  Dst clip rect.
{
#if 0
  UNUSED(trans);
  m_scene.Render3D(
        pimDst,       // Destination image.
        sOffsetX,     // Destination 2D x coord.
        sOffsetY,     // Destination 2D y coord.
        psprite,      // 3D sprite to render.
        palphaLight,  // Light to render with.
        &rcClip);     // Dst clip rect.

#if 0	// Draw wire frame.
  RMesh*	pmesh	= psprite->m_pmesh;
  short sTris = pmesh->m_sNum;
  uint16_t* pu16Vertex	= pmesh->m_pArray;
  while (sTris--)
  {
    const RP3d& v1 = psopView->m_pArray[*pu16Vertex++];
    const RP3d& v2 = psopView->m_pArray[*pu16Vertex++];
    const RP3d& v3 = psopView->m_pArray[*pu16Vertex++];
    rspLine(255, pimDst, v1.x, v1.y, v2.x, v2.y, &rcClip);
    rspLine(255, pimDst, v2.x, v2.y, v3.x, v3.y, &rcClip);
    rspLine(255, pimDst, v3.x, v3.y, v1.x, v1.y, &rcClip);
  }
#endif

  // If cursor shown . . .
  if (m_manip == Paint)
  {
    if (m_lTriIndex >= 0)
    {
      RMesh* pmesh = psprite->m_pmesh;
      ASSERT(uint32_t(m_lTriIndex) < pmesh->triangles.size());

      if (uint32_t(m_lTriIndex) < pmesh->triangles.size())
      {
        const Vector3D& v1 = psopView->points[pmesh->triangles[m_lTriIndex][0]];
        const Vector3D& v2 = psopView->points[pmesh->triangles[m_lTriIndex][1]];
        const Vector3D& v3 = psopView->points[pmesh->triangles[m_lTriIndex][2]];
        rspLine(255, pimDst, v1.x, v1.y, v2.x, v2.y, &rcClip);
        rspLine(255, pimDst, v2.x, v2.y, v3.x, v3.y, &rcClip);
        rspLine(255, pimDst, v3.x, v3.y, v1.x, v1.y, &rcClip);
      }
    }
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Processes drags and keys into transform stuff.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::ProcessManip(
    bool bButtonDown,		// In:  true if mouse button down.
    RInputEvent* pie,		// In:  Current input event.
    CSprite3* psprite,	// In:  3D data to process.
    RSop* psopView)		// In:  View space SOP.
{
#if 0
  // Transformation Manipulation Processing /////////////////
  if (bButtonDown && m_manip != Paint)
  {
    // If first time . . .
    if (m_bDragging == false)
    {
      rspSetMouse(m_sCursorResetX, m_sCursorResetY);
      rspHideMouseCursor();
      m_bDragging = true;
    }

    int16_t sCursorX, sCursorY;
    rspGetMouse(&sCursorX, &sCursorY, nullptr);
    rspSetMouse(m_sCursorResetX, m_sCursorResetY);

    int16_t sDeltaX = sCursorX - m_sCursorResetX;
    int16_t sDeltaY = m_sCursorResetY - sCursorY;

    static uint8_t* pau8KeyStatus = rspGetKeyStatusArray();

    if (pau8KeyStatus[RSP_SK_SHIFT] & 1)
      sDeltaY = 0;
    if (pau8KeyStatus[RSP_SK_CONTROL] & 1)
      sDeltaX = 0;

    switch (m_manip)
    {
      UNHANDLED_SWITCH;
      case Trans:
        m_fX += sDeltaX * c_fTransRate;
        m_fY += sDeltaY * c_fTransRate;
        break;
      case Scale:
        if (sDeltaY)
        {
          m_fScale	+= sDeltaY * c_fScaleRate;

          if (m_fScale < 0.1f)
            m_fScale = 0.1f;
          if (m_fScale > 1.0f)
            m_fScale = 1.0f;
        }
        break;
      case Rot:
        m_transRot.Ry(rspMod360(sDeltaX * c_fRotRate) );
        m_transRot.Rx(rspMod360(-sDeltaY * c_fRotRate) );
        break;
    }
  }
  else
  {
    if (m_bDragging == true)
    {
      rspShowMouseCursor();
      m_bDragging	= false;
    }
  }

  // Paint Processing /////////////////////////////////
  if (m_manip == Paint)
  {
    Vector3D	hitpoint;
    Vector3D	linept1, linept2;

    int16_t sMouseX, sMouseY;
    rspGetMouse(&sMouseX, &sMouseY, nullptr);
    m_pguiAnim->TopPosToChild(&sMouseX, &sMouseY);

    linept1.x	= sMouseX;
    linept1.y	= sMouseY;
    linept1.z	= -SHRT_MAX;
    linept2.x	= sMouseX;
    linept2.y	= sMouseY;
    linept2.z	= SHRT_MAX;

    int32_t	lTriIndex;
    bool bHit = TrianglesIntersectLineSegment(
                  linept1,								// In: line segment point 1
                  linept2, 							// In: line segment point 2 closest to this point. this should be the first point.
                  psprite->m_pmesh->triangles,	// In: mesh
                  psopView->points, 				// In: points for mesh
                  psprite->m_pmesh->triangles.size(),		// In: number of points in mesh
                  hitpoint,							// Out: point where line hit triangle
                  lTriIndex);							// Out: index of triangle it hit

    if (bHit)
    {
      m_lTriIndex	= lTriIndex;

      SetStatusText("Triangle %i", m_lTriIndex);

      if (bButtonDown)
      {
        RTexture*	ptex	= psprite->m_ptex;
        // Get into texture and replace current triangle index with our current color.
        if (ptex->size())
        {
          ASSERT(m_lTriIndex < ptex->size());

          // Set new color for this texture.
          ptex->setIndex(m_lTriIndex, m_u8Color);

          // Note modification.
          m_bModified	= true;
        }
      }
    }
    else
    {
      // If last time we found a triangle . . .
      if (m_lTriIndex >= 0)
      {
        // Clear status text.
        SetStatusText("");
      }
      else
      {
        // Otherwise, text does not refer to a triangle.  So leave it.
      }

      m_lTriIndex = -1;
    }
  }

  // Color Palette Processing ///////////////////
  if (m_pguiPal->m_sPressed)
  {
    int16_t sMouseX, sMouseY;
    rspGetMouse(&sMouseX, &sMouseY, nullptr);
    m_pguiPal->TopPosToChild(&sMouseX, &sMouseY);

    // Get color directly out of GUI.
    uint8_t	u8Color	= *(m_pguiPal->m_im.m_pData + (sMouseY * m_pguiPal->m_im.m_lPitch) + sMouseX);
    SetColor(u8Color);
    // Go into paint mode when a color is chosen.  Feels right somehow.
    SetManip(Paint);
  }

  // Input processing ///////////////////////////
  if (pie->sUsed == FALSE)
  {
    if (pie->type == RInputEvent::Key)
    {
      switch (pie->lKey)
      {
        case 'T':
        case 't':
          SetManip(Trans);
          break;
        case 'S':
        case 's':
          SetManip(Scale);
          break;
        case 'R':
        case 'r':
          SetManip(Rot);
          break;
        case 'P':
        case 'p':
        case 'D':
        case 'd':
          SetManip(Paint);
          break;

        case 'I':
        case 'i':
          ResetTransformation();
          break;

        case 27:
          m_bQuit = true;
          break;

        case '\r':
          m_transRot.makeIdentity();
          break;
      }
    }
  }

  if (rspGetQuitStatus() != FALSE)
  {
    m_bQuit	= true;
  }
#endif
}

//------------------------------------------------------------------------------
// Querries.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Internal Functions.
//------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Apply work colors to real thing.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::Apply(void)
{
  if (m_ptexSrc)
  {
    *m_ptexSrc	= m_texWork;
    m_bModified	= false;
    SetStatusText("Applied.");
  }
}

//////////////////////////////////////////////////////////////////////////////
// Revert work colors to real thing.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::Revert(void)
{
  if (m_ptexSrc)
  {
    m_texWork	= *m_ptexSrc;
    m_bModified	= false;

    SetStatusText("Reverted.");
  }
}


//////////////////////////////////////////////////////////////////////////////
// Save work colors to real thing.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::Save(void)
{
  Apply();
/*
  if (m_ptexchanSrc)
  {
    if (rspEZSave(&m_ptexchanSrc, m_strFileName) == SUCCESS)
    {
      SetStatusText("Applied; Saved \"%s\".", (const char*)m_strFileName);
    }
    else
    {
      SetStatusText("Applied; Failed to save \"%s\".", (const char*)m_strFileName);
    }
  }
*/
}

//////////////////////////////////////////////////////////////////////////////
// Set the current manipulation type.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetManip(
    Manip manip)	// In:  New manipulation type.
{
  if (m_pguiRoot)
  {
    SetPushBtnState(m_pguiRoot, m_manip + c_lIdTrans, RPushBtn::Off);

    m_manip	= manip;

    SetPushBtnState(m_pguiRoot, m_manip + c_lIdTrans, RPushBtn::On);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set the current palette color.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetColor(
    uint8_t	u8Color)		// In:  New color index.
{
  // Make sure it's in range . . .
  if (u8Color >= c_sPalStart && u8Color <= c_sPalEnd)
  {
    m_u8Color	= u8Color;

    if (m_pguiCurColor)
    {
      // Show current color in this item.
      int16_t	sX, sY, sW, sH;
      m_pguiCurColor->GetClient(&sX, &sY, &sW, &sH);
      rspRect(
            u8Color,
            &m_pguiCurColor->m_im,
            sX, sY,
            sW, sH,
            nullptr);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Set the specified button to notify.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetToNotify(
    int32_t lBtnId,					// In:  ID of btn whose callback will be set.
    RGuiItem::BtnUpCall pfn)	// In:  Function to notify.
{
  ASSERT(m_pguiRoot);
  RGuiItem*	pgui	= m_pguiRoot->GetItemFromId(lBtnId);
  if (pgui)
  {
    pgui->m_bcUser				= pfn;
    pgui->m_ulUserInstance	= reinterpret_cast<uintptr_t>(this);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set the specified gui to notify.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetToNotify(
    int32_t lId,							// In:  ID of gui whose callback will be set.
    RGuiItem::InputEventCall pfn)	// In:  Function to notify.
{
  ASSERT(m_pguiRoot);
  RGuiItem*	pgui	= m_pguiRoot->GetItemFromId(lId);
  if (pgui)
  {
    pgui->m_fnInputEvent		= pfn;
    pgui->m_ulUserInstance	= reinterpret_cast<uintptr_t>(this);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set the specified scrollbar to notify.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetToNotify(
    int32_t lId,								// In:  ID of scrollbar whose callback will be set.
    RScrollBar::UpdatePosCall pfn)	// In:  Function to notify.
{
  ASSERT(m_pguiRoot);
  RScrollBar*	psb	= (RScrollBar*)m_pguiRoot->GetItemFromId(lId);
  if (psb)
  {
    if (psb->m_type == RGuiItem::ScrollBar)
    {
      psb->m_upcUser			= pfn;
      psb->m_ulUserInstance	= reinterpret_cast<uintptr_t>(this);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set the status text field to that specified.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SetStatusText(
    const char* pszFrmt,		// In:  Format specifier ala sprintf.
    ...)							// In:  Arguments as specified by format.
{
  RGuiItem*	pguiStatus = m_pguiRoot->GetItemFromId(c_lIdStatus);
  if (pguiStatus)
  {
    va_list val;
    va_start(val, pszFrmt);

    vsprintf(pguiStatus->m_szText, pszFrmt, val);

    // Recompose with new text.
    pguiStatus->Compose();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Compose tarnsformations in the specified transform.  Init transform before
// calling as necessary.
//////////////////////////////////////////////////////////////////////////////
void
CTexEdit::ComposeTransform(
    RTransform& trans)		// In:  Transform to compose in.
{
  trans	= m_transRot;
  trans.Scale(m_fScale, m_fScale, m_fScale);
  trans.Translate(m_fX, m_fY, 0.0f);
}

//------------------------------------------------------------------------------
// Callbacks.
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Called when user presses Quit.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::QuitCall(RGuiItem* pgui)
{
  UNUSED(pgui);
  m_bQuit	= true;
}

////////////////////////////////////////////////////////////////////////////////
// Called when user presses a manipulation mode button.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::ManipCall(RGuiItem* pgui)
{
  SetManip(Manip(pgui->m_lId - c_lIdTrans) );
}

////////////////////////////////////////////////////////////////////////////////
// Called when user clicks in the color palette
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::ColorCall(RGuiItem* pgui, RInputEvent* pie)
{
  UNUSED(pgui);
  if (pie->type == RInputEvent::Mouse)
  {
    switch (pie->sEvent)
    {
      case RSP_MB0_PRESSED:
      {
#if 0	// This is now done in ProcessManip().
        // Get color directly out of GUI.
        uint8_t	u8Color	= *(pgui->m_im.m_pData + (pie->sPosY * pgui->m_im.m_lPitch) + pie->sPosX);
        SetColor(u8Color);
        // Go into paint mode when a color is chosen.  Feels right somehow.
        SetManip(Paint);
#endif
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Called when user presses Apply.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::ApplyCall(RGuiItem* pgui)
{
  UNUSED(pgui);
  Apply();
}

////////////////////////////////////////////////////////////////////////////////
// Called when user presses Save.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SaveCall(RGuiItem* pgui)
{
  UNUSED(pgui);
  Save();
}

////////////////////////////////////////////////////////////////////////////////
// Called when user presses Revert.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::RevertCall(RGuiItem* pgui)
{
  UNUSED(pgui);
  Revert();
}

////////////////////////////////////////////////////////////////////////////////
// Called when user presses Spot light button.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::SpotCall(RGuiItem* pgui)
{
  ASSERT(pgui->m_type == RGuiItem::PushBtn);
  RPushBtn*	pbtn	= (RPushBtn*)pgui;
  m_bSpotLight	= (pbtn->m_state == RPushBtn::On) ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
// Called when user adjust Brightness scrollbar.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::BrightnessCall(RScrollBar* psb)
{
  int32_t	lMin, lMax;
  psb->GetRange(&lMin, &lMax);
  float	fRange	= lMax - lMin + 1;

  if (fRange != 0.0f)
    m_sBrightness	= c_sMinBrightness + (psb->GetPos() / fRange) * c_sBrightnessRange;
}


////////////////////////////////////////////////////////////////////////////////
// Called when user presses Adjust button.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::AdjustCall(RGuiItem* pgui)
{
  UNUSED(pgui);
  // Get frequency.
  int32_t	lFreq	= m_pguiRoot->GetVal(c_lIdFrequency);

  char	szText[GUI_MAX_STR];
  m_pguiRoot->GetText(c_lIdAmount, szText, sizeof(szText) );

  float	fAdjust	= strtod(szText, nullptr);

  // Get palette to work with.
  channel_t au8Red  [palette::size];
  channel_t au8Green[palette::size];
  channel_t au8Blue [palette::size];
  rspGetPaletteEntries(
        0,					// Palette entry to start copying from
        palette::size,				// Number of palette entries to do
        au8Red,			// Pointer to first red component to copy to
        au8Green,		// Pointer to first green component to copy to
        au8Blue,			// Pointer to first blue component to copy to
        sizeof(uint8_t) );	// Number of bytes by which to increment pointers after each copy


  // Unmap colors from palette into full color values.
  m_texWork.unmap(
        au8Red,
        au8Green,
        au8Blue,
        sizeof(uint8_t) );

  // Adjust colors.
  m_texWork.adjust(
        fAdjust,
        lFreq);

  // Remap onto palette.
  m_texWork.remap(
        c_sPalStart,
        c_sPalNum,
        au8Red,
        au8Green,
        au8Blue,
        sizeof(uint8_t) );

  // Get rid of full colors.
  //m_texWork.FreeColors();
}

////////////////////////////////////////////////////////////////////////////////
// Called when user clicks in the anim surface.
////////////////////////////////////////////////////////////////////////////////
void
CTexEdit::AnimCall(RGuiItem* pgui, RInputEvent* pie)
{
  UNUSED(pgui);
  if (pie->type == RInputEvent::Mouse)
  {
    switch (pie->sEvent)
    {
      case RSP_MB1_PRESSED:
        // Do NOT Get color directly out of GUI.  The color there has already
        // undergone lighting effects.  Get the color from the actual texture.
        if (m_lTriIndex)
        {
          SetColor(m_texWork.getIndex(m_lTriIndex));
        }
        break;
    }
  }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
