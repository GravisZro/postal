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

#include "colormatch.h"

#include <BLUE/Blue.h>
#include <GREEN/Blit/Blit.h> // ONLY for the debug command... can be disabled if nec
#include <GREEN/Image/Image.h>
#include <ORANGE/File/file.h>
#include <ORANGE/QuickMath/FixedPoint.h>

#include <cstring>


//short RAlpha::ms_SetPalette(RImage* pimImage);
int16_t RAlpha::ms_IsPaletteSet = FALSE;
channel_t RAlpha::ms_red  [palette::size] = { 0 };
channel_t RAlpha::ms_green[palette::size] = { 0 };
channel_t RAlpha::ms_blue [palette::size] = { 0 };

channel_t RAlpha::ms_r[palette::size] = { 0 };
channel_t RAlpha::ms_g[palette::size] = { 0 };
channel_t RAlpha::ms_b[palette::size] = { 0 };
channel_t RAlpha::ms_a[palette::size] = { 0 };
channel_t RAlpha::ms_f[palette::size] = { 0 };

int16_t RMultiAlpha::ms_sIsInitialized = FALSE;
channel_t RMultiAlpha::ms_aucLiveDimming[UINT16_MAX];

channel_t rspMatchColorRGB(channel_t red,
                           channel_t green,
                           channel_t blue,
                           uint16_t sStart,
                           uint16_t sNum,
                           channel_t* pred,
                           channel_t* pgreen,
                           channel_t* pblue,
                           std::ptrdiff_t linc)
	{
   int16_t lMatch = 0;
   int32_t lMax = 0x01000000;
   int32_t lDist;
   std::ptrdiff_t lOffset = linc * sStart; // array offset

   pred += lOffset;
   pgreen += lOffset;
   pblue += lOffset;
	
   for(uint16_t i = 0; i < sNum; ++i)
		{
      lDist = SQR(int16_t(*pred  )-red  ) +
              SQR(int16_t(*pgreen)-green) +
              SQR(int16_t(*pblue )-blue );
      pred += linc;
      pgreen += linc;
      pblue += linc;
      if (lDist <= lMax)
			{
			lMax = lDist;
			lMatch = i;
			}
		}

   return channel_t(lMatch + sStart);
	}


// Currently, I am using an image as the file format for a palette.
// This uses the rsp system standard type pixel32_t, which is endian swapped on the PC.
// It counts on RSP SetPalette to go through a gamma correction table on the PC.
// Higher level functions can do actual image conversions.

int16_t RAlpha::Load(const char* pszFile)
	{
	RFile file;

	if (file.Open(pszFile,"rb",RFile::LittleEndian))
		{
		TRACE("RAlpha::Load: Error accessing file %s\n",pszFile);
      return FAILURE;
		}
	
   int16_t sResult = Load(&file);
	file.Close();

   return sResult;
	}

int16_t RAlpha::Save(const char* pszFile)
	{
	RFile *fp = new RFile;

	if (fp->Open(pszFile,"wb",RFile::LittleEndian))
		{
		TRACE("RAlpha::Load: Error accessing file %s\n",pszFile);
      return FAILURE;
		}
	
   int16_t sResult = Save(fp);
	fp->Close();
   return sResult;
	}

int16_t RAlpha::Save(RFile* fp)
	{
	int16_t sVersion = 1;

	if (!fp)
		{
		TRACE("RAlpha::Save: Null RFile!\n");
      return FAILURE;
		}

	fp->Write("RALPHA");
	fp->Write(&sVersion);
	fp->Write(&m_sAlphaDepth);
	/* inverted 
	for (short i=0; i < m_sAlphaDepth; i++)
		fp->Write(m_pAlphas[i],256);
		*/
	for (int16_t i=0; i < 256; i++)
		fp->Write(m_pAlphas[i],m_sAlphaDepth);

	return 0;
	}

int16_t RAlpha::Load(RFile* fp)
	{
	int16_t sVersion = 1;
	char name[20];

#ifdef _DEBUG

	if (!fp)
		{
		TRACE("RAlpha::Load: Null RFile!\n");
      return FAILURE;
		}

#endif

	fp->Read(name);
	if (std::strcmp(name,"RALPHA"))
		{
		TRACE("RAlpha::Load: Not an RAlpha!\n");
      return FAILURE;
		}

	fp->Read(&sVersion);
	if (sVersion != 1)
		{
		TRACE("RAlpha::Load: Not a supported version!\n");
      return FAILURE;
		}

	fp->Read(&m_sAlphaDepth);
	Alloc(m_sAlphaDepth);

	/* ionverted
	for (short i=0; i < m_sAlphaDepth; i++)
		fp->Read(m_pAlphas[i],256);
	*/

	for (int16_t i=0; i < 256; i++)
		{
		if (fp->Read(m_pAlphas[i],m_sAlphaDepth)!=m_sAlphaDepth)
			{
			TRACE("RAlpha::Load: read error!\n");
         return FAILURE;
			}
		}

	return 0;
	}

int16_t RAlpha::ms_SetPalette(RImage* pimImage)
	{
#ifdef _DEBUG

	if (!pimImage)
		{
		TRACE("RAlpha::ms_SetPalette: Null image!\n");
      return FAILURE;
		}

	if (!pimImage->m_pPalette)
		{
		TRACE("RAlpha::ms_SetPalette: image has no palette!\n");
      return FAILURE;
		}

	if (!pimImage->m_pPalette->m_pData)
		{
		TRACE("RAlpha::ms_SetPalette: palette has no data!\n");
      return FAILURE;
		}

#endif

	if (pimImage->m_pPalette->GetEntries(0,256,ms_red,ms_green,ms_blue,1)==0)
		{
		ms_IsPaletteSet = TRUE;
		return 0;
		}

   return FAILURE;
	}

 // do alloc first
void RAlpha::StartEffect()
	{
	// clear scratch space
	for (int16_t i=0; i< 256;i++)
		{
		ms_f[i] = ms_r[i]  = ms_g[i] = ms_b[i] = ms_a[i] = 0;
		}
	}

int16_t RAlpha::MarkEffect(int16_t sLev,int16_t sChannel,uint8_t ucLev)
	{
	if (sLev >= m_sAlphaDepth)
		{
		TRACE("RAlpha::MarkEffect: Level out of range!\n");
      return FAILURE;
		}

	if (sChannel >= csLAST_EFFECT) 
		{
		TRACE("RAlpha::MarkEffect: Channel not supported\n");
      return FAILURE;
		}

	ms_f[sLev] |= sChannel;
	switch (sChannel)
		{
	case csALPHA:
		ms_a[sLev] = ucLev;
	break;
	case csRED:
		ms_r[sLev] = ucLev;
	break;
	case csGREEN:
		ms_g[sLev] = ucLev;
	break;
	case csBLUE:
		ms_b[sLev] = ucLev;
	break;

		default:
			TRACE("RAlpha::MarkEffect:: BAD CHANNEL VALUE!\n");
         return FAILURE;
		}

	return 0;
	}

// will create it for you.
// This is designed to interpolate channels independently!
//
void RAlpha::FinishEffect(int16_t sPalStart, int16_t sPalLen)
	{
	// Interpolate each channel freely
	int16_t sChannel = 1;
	// ( THE TWO ENDS of the channels ARE ASSUMED MARKED!!!)
   ms_f[0] = ms_f[m_sAlphaDepth-1] = 0xFF;

	while (sChannel < csLAST_EFFECT)
		{
		uint8_t* pucChannel = nullptr;

		switch (sChannel)
			{
		case csALPHA:
			pucChannel = &ms_a[0];
		break;
		case csRED:
			pucChannel = &ms_r[0];
		break;
		case csGREEN:
			pucChannel = &ms_g[0];
		break;
		case csBLUE:
			pucChannel = &ms_b[0];
		break;

			default:
				TRACE("RAlpha::FinishEffect:: BAD CHANNEL VALUE!\n");
			}

		// Interpolate the channel!
		int16_t sIndex = 0;
		int16_t sBaseIndex = 0;
		while (sIndex < m_sAlphaDepth)
			{
			if ((ms_f[sIndex] & sChannel) != 0)
				{
				if (sIndex - sBaseIndex > 1)
					{
					// fill in inbetween stuff
					double dDelta = double(pucChannel[sIndex] - 
						pucChannel[sBaseIndex]) / double(sIndex - sBaseIndex);
					double dVal = double(pucChannel[sBaseIndex])+0.5;

					for (int16_t j = sBaseIndex; j< sIndex;j++)
						{
						pucChannel[j] = uint8_t(dVal);
						dVal += dDelta;
						}
					}
				sBaseIndex = sIndex;
				}
			sIndex++;
			}

		sChannel <<= 1;
		}

	// NOW CREATE THE ACTUAL ALPHAS!
	CreateLightEffectRGB(sPalStart,sPalLen);
	
	}

// Uses the current system palette:
//
int16_t RAlpha::ms_SetPalette()
	{
	rspGetPaletteEntries(0,256,ms_red,ms_green,ms_blue,1);
	ms_IsPaletteSet = TRUE;
	return 0;
	}

int16_t RAlpha::Alloc(int16_t sDepth)
	{
	/* Inverted
	m_pAlphas = (uint8_t**) calloc(m_sAlphaDepth,sizeof (uint8_t*));
	for (short i=0; i < m_sAlphaDepth; i++)
		m_pAlphas[i] = (uint8_t*) calloc(1,256);
	*/
	m_sAlphaDepth = sDepth;

	for (int16_t i=0; i < 256; i++)
		m_pAlphas[i] = (uint8_t*) calloc(1,m_sAlphaDepth);

	return 0;
	}

RAlpha::RAlpha()
	{
	//m_pAlphas = nullptr;
   for (int16_t i = 0 ;i < 256; ++i)
		m_pAlphas[i] = nullptr;
	m_sAlphaDepth = 0;
	}

void RAlpha::Erase()
	{
//	if (m_pAlphas)
//		{
      for (int16_t i = 0; i < 256; ++i)
			{
			if (m_pAlphas[i]) 
				{
				free(m_pAlphas[i]);
				m_pAlphas[i] = nullptr;
				}
			}
		//free(m_pAlphas);
//		}
	}

RAlpha::~RAlpha()
	{
	Erase();
	}

// ************ COMMENT THIS OUT TO REMOVE DEPENDENCY ON BLIT!
// For debugging an alpha, you should set the current palette to the
// default one...
void RAlpha::Dump(RImage* pimDst,int16_t sX,int16_t sY) 
	{
	int16_t i,j;

#ifdef _DEBUG
	if (!pimDst)
		{
		TRACE("RAlpha::Dump: null image passed!\n");
		return;
		}
#endif

   for (j = 0; j < m_sAlphaDepth; ++j)
      for (i = 0 ;i < 0x00FF; ++i)
         rspPlot<uint8_t>(m_pAlphas[i][j], pimDst, sX + i, sY + j);
	}

// ************ COMMENT THIS OUT TO REMOVE DEPENDENCY ON BLIT!
// For debugging an alpha, you should set the current palette to the
// default one...
void RAlpha::DumpPalette(RImage* pimDst, int16_t sX, int16_t sY)
{
  int16_t i;

#ifdef _DEBUG
  if (!pimDst)
  {
    TRACE("RAlpha::DumpPalette: null image passed!\n");
    return;
  }
#endif

  for (i = 0; i < 0x00FF; ++i)
  {
    rspLine(uint8_t(i), pimDst, sX + i, sY, sX + i, sY + 0x00FF);
  }
}

// dOpacity for now is between 0.0 (background) and 1.0 (foreground)
int16_t RAlpha::CreateAlphaRGB(double dOpacity,int16_t sPalStart, int16_t sPalLen)
	{
#ifdef _DEBUG
	if (!ms_IsPaletteSet)
		{
		TRACE("RAlpha::CreateAlphaRGB: First set your palette with ms_SetPalette\n");
      return FAILURE;
		}
#endif

	// could erase first.
	Alloc(256);

	// If you use a 256 x 256 table for calculating BYTE * OPACITY = BYTE, you may double
	// your net speed
	int16_t lSrc = (int32_t)256 * dOpacity;
	int16_t lDst = 256 - lSrc;

	// If you use a 256 x 256 table for calculating BYTE * OPACITY = BYTE, you may double
	RFixedU16 r,g,b;

	int16_t s,d;
   for (s = 0; s < 256; ++s)
		{
      for (d = 0; d < 256; ++d)
			{
			// This can be replaced with a 256K table
         r.val = uint16_t( ms_red  [s] * lSrc + ms_red  [d] * lDst);
			g.val = uint16_t( ms_green[s] * lSrc + ms_green[d] * lDst);
         b.val = uint16_t( ms_blue [s] * lSrc + ms_blue [d] * lDst);

         m_pAlphas[s][d] = rspMatchColorRGB(r.mod,g.mod,b.mod,sPalStart,sPalLen,
					ms_red,ms_green,ms_blue,1);					
			}
		}
	
	return 0;
	}

// dOpacity for now is between 0 (background) and 255 (foreground)
// Input description should be arrays at least sAlphaDepth long.
int16_t RAlpha::CreateLightEffectRGB(channel_t* pa,
                                     channel_t* pr,
                                     channel_t* pg,
                                     channel_t* pb,
                                     std::ptrdiff_t linc,
                                     int16_t sPalStart,
                                     int16_t sPalLen,
                                     int16_t sAlphaDepth)
	{
  UNUSED(linc);
#ifdef _DEBUG
	if (!ms_IsPaletteSet)
		{
		TRACE("RAlpha::CreateLightEffectRGB: First set your palette with ms_SetPalette\n");
      return FAILURE;
		}
#endif

	// could erase first.
	Alloc(sAlphaDepth);

	// If you use a 256 x 256 table for calculating BYTE * OPACITY = BYTE, you may double
	RFixedU16 r,g,b;
	int32_t lSrc,lFog;

	int16_t s,f;
	for (s=0;s<256;s++)
		{
		for (f = 0;f < sAlphaDepth;f++)
			{
			lSrc = int32_t(pa[f]);
			lFog = 255 - lSrc;
			// This can be replaced with a 256K table
         r.val = uint16_t( ms_red  [s] * lSrc + pr[f] * lFog);
			g.val = uint16_t( ms_green[s] * lSrc + pg[f] * lFog);
         b.val = uint16_t( ms_blue [s] * lSrc + pb[f] * lFog);

         m_pAlphas[s][f] = rspMatchColorRGB(r.mod,g.mod,b.mod,sPalStart,sPalLen,
					ms_red,ms_green,ms_blue,1);					
			}
		}
	
	return 0;
	}

int16_t RAlpha::CreateLightEffectRGB(int16_t sPalStart, int16_t sPalLen)
	{
#ifdef _DEBUG
	if (!ms_IsPaletteSet)
		{
		TRACE("RAlpha::CreateLightEffectRGB: First set your palette with ms_SetPalette\n");
      return FAILURE;
		}
#endif

	RFixedU16 r,g,b;
	int32_t lSrc,lFog;

	int16_t s,f;
	for (s=0;s<256;s++)
		{
		for (f = 0;f < m_sAlphaDepth;f++)
			{
			lSrc = int32_t(ms_a[f]);
			lFog = 255 - lSrc;

			// This can be replaced with a 256K table
         r.val = uint16_t( ms_red  [s] * lSrc + ms_r[f] * lFog);
			g.val = uint16_t( ms_green[s] * lSrc + ms_g[f] * lFog);
         b.val = uint16_t( ms_blue [s] * lSrc + ms_b[f] * lFog);

         m_pAlphas[s][f] = rspMatchColorRGB(r.mod,g.mod,b.mod,sPalStart,sPalLen,
					ms_red,ms_green,ms_blue,1);					
			}
		}
	
	return 0;
	}


// **************************************************************************
// **************************************************************************
// MultiAlpha!
// **************************************************************************
// **************************************************************************

RMultiAlpha::RMultiAlpha(void)
	{
	// Handle the one time initiation of the dimmer control:
	if (!ms_sIsInitialized)
		{
		// FILL THE TABLE!
		int32_t lSrcVal,lDimVal,lCurValue,lNumerator;
      channel_t* pCur = ms_aucLiveDimming;
		// This is DimVal major!

      for (lDimVal = 0; lDimVal < 256; lDimVal++)
			{
			lNumerator = 127;	// for rounding
			lCurValue = 0;

         for (lSrcVal = 0; lSrcVal < 256; lSrcVal++,pCur++)
				{
				lNumerator += lDimVal;

				if (lNumerator >= 255) 
					{
					lNumerator -= 255; 
					lCurValue++;
					}

            *pCur = channel_t(lCurValue);
				}
			}
		
		ms_sIsInitialized = TRUE;
		}

	Erase();
	}

RMultiAlpha::~RMultiAlpha(void)
{
  for (uint8_t i = 0; i < m_sNumLevels; ++i)
    if (m_pAlphaList[i] != nullptr)
      delete m_pAlphaList[i];

  free (m_pAlphaList);
  free (m_pLevelOpacity);

  Erase();
}

int16_t RMultiAlpha::Alloc(uint8_t sDepth)
{
  // a nullptr pointer will be left in the zero position.
  if (m_pAlphaList)
  {
    TRACE("RMultiAlpha::alloc: error, already created!\n");
    return FAILURE;
  }

  m_sNumLevels = sDepth;
  m_pAlphaList = (RAlpha**)calloc(m_sNumLevels,sizeof(RAlpha*));
  m_pLevelOpacity = (uint8_t*)calloc(m_sNumLevels, 1);

  return SUCCESS;
}

void RMultiAlpha::Erase(void)
{
  m_sNumLevels = 0;
  m_pAlphaList = nullptr;

  for(uint8_t**& ptr : m_pGeneralAlpha)
    ptr = nullptr;
  for(uint8_t& val : m_pSaveLevels)
    val = 0;

  m_sGeneral = TRUE;
  m_pLevelOpacity = nullptr;
}

// Note: this type can be supported with one alphablit
// by just pointing the first n levels to teh shade
// you desire
// SOON TO BE ARCHAIC!!!!!
//
int16_t RMultiAlpha::AddAlpha(RAlpha* pAlpha,int16_t sLev)
	{
	if (sLev > m_sNumLevels)
		{
		TRACE("RMultiAlpha::AddAlpha: level out of range!\n");
      return FAILURE;
		}
	m_pAlphaList[sLev] = pAlpha;
	return 0;
	}

int16_t RMultiAlpha::Load(RFile* pFile)
	{
	char type[50];
	int16_t sVer;

	pFile->Read(type);
	if (std::strcmp(type,"MALPHA"))
		{
		TRACE("RMultiAlpha::Load: Bad File Type\n");
      return FAILURE;
		}

	pFile->Read(&sVer);

	if (sVer != 2)
		{
		TRACE("RMultiAlpha::Load: Bad Version Number\n");
      return FAILURE;
		}

   pFile->Read(&m_sNumLevels);

   uint8_t empty_data = 0;
   pFile->Read(&empty_data);
   ASSERT(empty_data == 0); // ensure data is unused

   Alloc(m_sNumLevels);

	// General table information:
	pFile->Read(&m_sGeneral);
	// Opacity data
   pFile->Read(m_pLevelOpacity, m_sNumLevels);
	// Level Data
   pFile->Read(m_pSaveLevels, palette::size);

	// Now load the actual sub-alphas
	int16_t i;
   for (i = 0; i < m_sNumLevels; ++i)
		{
		m_pAlphaList[i] = new RAlpha;
		m_pAlphaList[i]->Load(pFile);
		}

	// Now the challange... restore the pointer list:
   for (i = 0; i < palette::size; ++i)
		{
      if (m_pSaveLevels[i]==0 || m_pSaveLevels[i] > m_sNumLevels)
			m_pGeneralAlpha[i] = nullptr;
		else m_pGeneralAlpha[i] = m_pAlphaList[m_pSaveLevels[i] - 1]->m_pAlphas;
		}

	return 0;
	}

int16_t RMultiAlpha::Save(RFile* pFile)
	{
	char type[] = "MALPHA";
	int16_t sVer = 2;

	pFile->Write(type);
   pFile->Write(&sVer);
   pFile->Write(&m_sNumLevels);
   pFile->Write(uint8_t(0));

	// General table information:
	pFile->Write(&m_sGeneral);
	// Opacity data
   pFile->Write(m_pLevelOpacity, m_sNumLevels);
	// Level Data
   pFile->Write(m_pSaveLevels, palette::size);

	// Now save the actual sub-alphas
	for (int16_t i = 0; i < m_sNumLevels; i++)
		{
		m_pAlphaList[i]->Save(pFile);
		}


	return 0;
	}

int16_t RMultiAlpha::Load(const char* pszFile)
	{
	RFile fplocal;
	RFile *fp = &fplocal; // needs to be freed automatically!

	if (m_pAlphaList != nullptr)
		{
		TRACE("RMultiAlpha::Load: MultAlpha NOT EMPTY!\n");
      return FAILURE;
		}

	if (fp->Open(pszFile,"rb",RFile::LittleEndian))
		{
		TRACE("RMultiAlpha::Load: Error accessing file %s\n",pszFile);
      return FAILURE;
		}
	
   int16_t sResult = Load(fp);
	fp->Close();
   return sResult;
	}

int16_t RMultiAlpha::Save(const char* pszFile)
	{
	RFile *fp = new RFile;

	if (fp->Open(pszFile,"wb",RFile::LittleEndian))
		{
		TRACE("RMultiAlpha::Load: Error accessing file %s\n",pszFile);
      return FAILURE;
		}
	
   int16_t sResult = Save(fp);
	fp->Close();
   return sResult;
	}

// dOpacity for now is between 0.0 (background) and 1.0 (foreground)
// if sGeneral is true, then the LOGICAL layer will be mapped from 
// 0-255, otherwise it wil be mapped to the layer number.
// You must first alloc the amount of colors.
//
int16_t RMultiAlpha::CreateLayer(int16_t sLayerNumber,
												double dOpacity,
												int16_t sPalStart, 
												int16_t sPalLen)
	{
	if (sLayerNumber >= m_sNumLevels)
		{
		TRACE("RMultiAlpha::CreateAlphaLayer: Layer out of range.\n");
      return FAILURE;
		}

	if (m_pAlphaList[sLayerNumber] != nullptr)
		{
		TRACE("RMultiAlpha::CreateAlphaLayer: Error: Layer exists.\n");
      return FAILURE;
		}
	// remember how this level was created.
   m_pLevelOpacity[sLayerNumber] = channel_t(255 * dOpacity);
	m_pAlphaList[sLayerNumber] = new RAlpha;
	m_pAlphaList[sLayerNumber]->CreateAlphaRGB(dOpacity,sPalStart,sPalLen);

	return 0;
	}

// This interpolates the general table information
//
int16_t RMultiAlpha::Finish(int16_t sGeneral)
	{
	int16_t i;

	// At this level, might as well using floating point math for now.
	if (sGeneral == FALSE) // make a level based identity mapping:
		{
		m_sGeneral = FALSE;

		m_pGeneralAlpha[0] = nullptr; // shift things up one.
		for (i=0;i<m_sNumLevels;i++) // make it look like (m_sNumLevels+2)
			{
			m_pSaveLevels[i] = i;
			// Use nullptr as a code for opacity, 0 is hooked as transparent
         if (m_pLevelOpacity[i] == 255)
           m_pGeneralAlpha[i+1] = nullptr;
         else
           m_pGeneralAlpha[i+1] = m_pAlphaList[i]->m_pAlphas;
			}
		// make everything off the end opaque
		m_pSaveLevels[m_sNumLevels] = m_sNumLevels;
		for (i = m_sNumLevels + 1; i < 256;i++) 
			{
			m_pGeneralAlpha[i] = nullptr;
			m_pSaveLevels[i] = m_sNumLevels + 1;
			}
		}
   else // spread out the values from 0x00 to 0xFF:
		{
		m_sGeneral = TRUE;

		for (i=0;i<256;i++) m_pSaveLevels[i] = uint8_t(0);

		for (i=0;i<m_sNumLevels;i++)
			{
			// Use nullptr as a code for opacity, 0 is hooked as transparent
			m_pSaveLevels[m_pLevelOpacity[i]] = (i+1); // the zeroth level is implied!
			}

		// set ends as a default:
		if (m_pSaveLevels[255] == 0) m_pSaveLevels[255] = m_sNumLevels+1;

		int16_t sIndex = 0;
		int16_t sBaseIndex = 0;
		while (sIndex < 256)
			{
			if (m_pSaveLevels[sIndex] != 0)
				{
				if (sIndex - sBaseIndex > 1)
					{
					// fill in inbetween stuff
					double dDelta = double(m_pSaveLevels[sIndex] - 
						m_pSaveLevels[sBaseIndex]) / double(sIndex - sBaseIndex);
					double dVal = double(m_pSaveLevels[sBaseIndex])+0.5;

					for (int16_t j = sBaseIndex; j< sIndex;j++)
						{
						m_pSaveLevels[j] = uint8_t(dVal);
						dVal += dDelta;
						}
					}
				sBaseIndex = sIndex;
				}
			sIndex++;
			}
		// Now, link it up!
		for (i=0;i<256;i++) 
			{
			if ((m_pSaveLevels[i]==0) || (m_pSaveLevels[i] > m_sNumLevels)) 
				m_pGeneralAlpha[i] = nullptr;
			else m_pGeneralAlpha[i] = m_pAlphaList[m_pSaveLevels[i] - 1]->m_pAlphas;
			}
		}

	return 0;
	}

///////////////////////////////////////////////////////////////////////////
// This is designed for getting an entire multialpha table into L2 cache.
// It does not behave entirely within the official MultiAlpha genre, and
// so it is best to create an uncompressed MultiAlpha First, and then to 
// create a compact one.  
//
// The compressed Multialpha does more than merely use less memory - it
// ASSURES a single contiguous block of memory is used, making it easier
// to ensure caching.
//
// The main memory savings is that you only need to store alpha lists for 
// source olors you anticipate.  If your source only has 64 colors, than
// your alpha table needs only 1/4 the memory as if it realied on 256 colors.
// NOTE:  In release mode, using such a table incorrectly will crash the 
// machine.
//
// Other than these points, it will behave very closely to a normal
// multialpha.
///////////////////////////////////////////////////////////////////////////
//  UINPUT:  the source color range (Start Index, Len) and destination
//				range (start index and len)
///////////////////////////////////////////////////////////////////////////
//  OUPUT:  the actual FastMultiAlpha pointer in the form of a (uint8_t***)
//          optional:  the size of the data in bytes.
///////////////////////////////////////////////////////////////////////////
uint8_t*** RMultiAlpha::pppucCreateFastMultiAlpha(
		int16_t sStartSrc,int16_t sNumSrc,	// color indices
		int16_t sStartDst,int16_t sNumDst,
		int32_t*	plAlignedSize)
	{
	// Assumes a fully correct this pointer!
	int32_t lTotSize = 1024 + int32_t(m_sNumLevels) * sNumSrc * (4L + sNumDst);
	// align it to 4096 to save cache memory!
	uint8_t*** pppucFastMem = (uint8_t***) calloc(1,lTotSize + 4095);
	// WARNING:  these arrays of pointers are 4 bytes in size, so be careful
	// how you seek!

	// I am adding on in BYTES, not words...
   uint8_t*** pppucFastAligned = (uint8_t***) ((uintptr_t(pppucFastMem)+4095) & (~4095) );

	if (!pppucFastMem) return nullptr;
	uint8_t* pInfo = (uint8_t*)pppucFastAligned;
	// For freeing:
   int32_t lMemOffset = uintptr_t(pppucFastAligned) - uintptr_t(pppucFastMem);

	// Remember offsets for each main memory structure:

	// Jump over the array of pointers to alpha levels
	// I AM COUNTING ON 4 byte array increments! (+ 1024 bytes)
   uint8_t** ppucFirstSrcArray = reinterpret_cast<uint8_t**>(pppucFastAligned + 256);
	uint8_t** ppucCurSrc = ppucFirstSrcArray;
	// Jump over the source arrays of pointers destination lists:

	// I AM COUNTING ON 4 byte array increments!
	// ( + 4LS bytes)
   uint8_t* pucData = reinterpret_cast<uint8_t*>(ppucFirstSrcArray + int32_t(m_sNumLevels) * sNumSrc);
	uint8_t* pData = pucData;

	//------------------------------------------------------------------------
	// Create the needed pointers into the normal MultiAlpha:
	//------------------------------------------------------------------------
	uint8_t** ppucLevel = nullptr;

	// Copy the abridged data from the current MultiAlpha in 
	// level majorest, source major, destination minor form:
	int16_t a = 0,l,s,d;
	int16_t sOldL = 0;

	while (a < 255)
		{
		a++;
		//------------------------- Find unique alphas
		l = m_pSaveLevels[a];
		pppucFastAligned[a] = pppucFastAligned[a-1]; // repeat the pointers
		if (l == sOldL) continue;
		sOldL = l;
		//-------------------------
		// Start a new Alpha Level:
		pppucFastAligned[a] = ppucCurSrc - sStartSrc; // sStartSrc Biased!

		ppucLevel = m_pGeneralAlpha[a];
		if (!ppucLevel) {pppucFastAligned[a] = nullptr; continue;} // Final 100% alphas are not stored

		for (s = 0; s < sNumSrc; s++,ppucCurSrc++)
			{
			*ppucCurSrc = pData - sStartDst; // sStartDst Biased!
			for (d = 0; d < sNumSrc; d++)
				{
				*pData++ = ppucLevel[s + sStartSrc][d + sStartDst];
				}
			}
		}

	// Create the minimum possible data header in BYTES:
	// Set the minimum cut off at one half the minimum alpha level. (MUST ROUND UP!)
	pInfo[0] = uint8_t((*m_pLevelOpacity + 1)>>1); // The minimum non-fully-transparent alpha value
	pInfo[1] = uint8_t(sNumSrc);  // from this you can calculate remaining values
   pInfo[2] = uint8_t(lMemOffset & UINT8_MAX); // low order byte of mem offset
   pInfo[3] = uint8_t((lMemOffset >> 8) & UINT8_MAX); // high order byte of mem offset
	
	if (plAlignedSize) *plAlignedSize = lTotSize;
	return pppucFastAligned;
	}

///////////////////////////////////////////////////////////////////////////
//  Delete Fast MultiAlpha ============  IMPORTANT!!!!!!!
//
//  This is the ONLY way a Fast MultiAlpha can be deleted!
//  Returns FAILURE or SUCCESS
///////////////////////////////////////////////////////////////////////////
int16_t	RMultiAlpha::DeleteFastMultiAlpha(uint8_t ****pfmaDel)
	{
	ASSERT(*pfmaDel);

	uint8_t ***pAligned = *pfmaDel;
	int32_t lDelta = 0;
	uint8_t* pByteAligned = (uint8_t*)pAligned;
	lDelta = pByteAligned[2] + (pByteAligned[3] << 8);

	ASSERT(lDelta < 4096); // best error checking I can do.

	free(pByteAligned - lDelta);
	*pfmaDel = nullptr;

	return SUCCESS;
	}

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
//  
//  Query Fast MultiAlpha -	determines optimum alpha layers for a given
//										cache.
//
///////////////////////////////////////////////////////////////////////////
//  Input:  # of source colors, # of dest colors, desired # of total bytes
//
//  Ouput:  Max # of layers (= levels - 2)
//				optional:  separate sizes for header and data
///////////////////////////////////////////////////////////////////////////
int16_t RMultiAlpha::QueryFastMultiAlpha(int16_t sNumSrcCol, int16_t sNumDstCol,
													int32_t lTotMem, int32_t* plHeaderSize,
													int32_t* plDataSize)
	{
	// This is for version I, naturally.
	int32_t lLevMem;
	int32_t lDataPerLev;
	int32_t lHeaderPerLev;
	int16_t sNumLev;
	
	int32_t lHeaderMem = 1024; // ucClear & deltaMemFree16, + 255 pointer array (uint8_t**)
	lDataPerLev = int32_t(sNumSrcCol) * sNumDstCol; // Src * Dst BYTE table
	lHeaderPerLev = 4L * sNumSrcCol; // uint8_t* array of len Src
	lLevMem = lDataPerLev + lHeaderPerLev;

	sNumLev = (lTotMem - lHeaderMem) / lLevMem;

	if (plHeaderSize) *plHeaderSize = lHeaderPerLev * sNumLev +  lHeaderMem;
	if (plDataSize) *plDataSize = lDataPerLev * sNumLev;

	return sNumLev;
	}

///////////////////////////////////////////////////////////////////////////
//	Here is a class wrapper for the Fast Multi Alpha:
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//  
//	RFastMulitAlphaWrapper
//
///////////////////////////////////////////////////////////////////////////

void	RFastMultiAlphaWrapper::Clear()
	{
	m_sAccess = FALSE;

	m_sStartSrc = 
	m_sStartDst =
	m_sNumSrc = 
	m_sNumDst = 
	m_sNumLayers = 0;
	m_sDelete = FALSE; // wil not delete data unless loaded
	}

RFastMultiAlphaWrapper::RFastMultiAlphaWrapper()
	{
	m_pppucFastMultiAlpha = nullptr;
	Clear();
	}

void	RFastMultiAlphaWrapper::Erase()
	{
	if (m_sDelete == TRUE) // if I have responsibility for the data
		{
		if (m_pppucFastMultiAlpha) 
			RMultiAlpha::DeleteFastMultiAlpha(&m_pppucFastMultiAlpha);
		}

	Clear();
	}

RFastMultiAlphaWrapper::~RFastMultiAlphaWrapper()
	{
	Erase();
	}

///////////////////////////////////////////////////////////////////////////
//  
//	Attach:  After creating an FMA, give it this wrapper:
// IMPORTANT:  If you ATTACH an FMA, the wrapper will NOT delete it.
//					Only if you LOAD one will the FMA take responsibility.
//
// NOTE:  The information MUST MATCH the FMA, as there is no way to 
//			verify it!
//
///////////////////////////////////////////////////////////////////////////

int16_t RFastMultiAlphaWrapper::Attach(uint8_t	***pppucFMA,int16_t sStartSrc,
		int16_t sNumSrc,int16_t sStartDst,int16_t sNumDst,
		int16_t sNumLayers)
	{
	ASSERT(m_pppucFastMultiAlpha == nullptr);
	ASSERT(sStartSrc >= 0);
	ASSERT(sNumSrc > 0);
	ASSERT(sStartDst >= 0);
	ASSERT(sNumDst > 0);
	ASSERT(sNumLayers > 0);

	m_pppucFastMultiAlpha = pppucFMA;

	m_sStartSrc = sStartSrc;
	m_sNumSrc = sNumSrc;
	m_sStartDst = sStartDst;
	m_sNumDst = sNumDst;
	m_sNumLayers = sNumLayers;

	m_sAccess = TRUE;
	m_sDelete = FALSE;

	return SUCCESS;
	}

///////////////////////////////////////////////////////////////////////////
//
//	pppucGetFMA:  This returns your actual FMA.  You MUST refer to it
//						directly to get the benefits.  To avoid the temptation
//						to refer to this wrapper INSTEAD of the true FMA, you
//						are only permitted to grab it ONCE.
//
///////////////////////////////////////////////////////////////////////////

uint8_t***	RFastMultiAlphaWrapper::pppucGetFMA()
	{
	ASSERT(m_pppucFastMultiAlpha);

	if (m_sAccess == TRUE) 
		{
		m_sAccess = FALSE;
		return m_pppucFastMultiAlpha;
		}

	TRACE("YOU MAY NOT EVER TRY TO GET THE FAST MULTI ALPHA MOE THAN ONCE!\n");
	TRACE("YOU MUST INSTEAD STORE THE POINTER SOMEWHERE AND REUSE IT!\n");

	ASSERT(FALSE);
	return nullptr;
	}

///////////////////////////////////////////////////////////////////////////
//
//	Save:  Save a File version of the FMA
//
///////////////////////////////////////////////////////////////////////////

int16_t RFastMultiAlphaWrapper::Save(RFile* pf)
	{
	ASSERT(pf);
	ASSERT(m_pppucFastMultiAlpha);

	pf->Write("__FMA__");
	int16_t sVer = 1;
	pf->Write(sVer);

	pf->Write(m_sNumLayers);
	pf->Write(m_sStartSrc);
	pf->Write(m_sNumSrc);
	pf->Write(m_sStartDst);
	pf->Write(m_sNumDst);

	// RESERVED:
	pf->Write(int16_t(0)); 
	pf->Write(int16_t(0));
	pf->Write(int16_t(0));
	pf->Write(int16_t(0));
	pf->Write(int32_t(0));

	//======================= Write out the secret header from the FMA:
	uint8_t	*pucHeader = (uint8_t*)m_pppucFastMultiAlpha;
	pf->Write(*pucHeader); // save ucMin

	//========================== Write out the LAYER distribution list: 
	void* pvTemp = (void*)-1;
	int16_t sLevel = -1;
	int16_t i;

	// Assume the FIRST level has a value of ZERO!
	pf->Write(uint8_t(0));

	for (i=1;i < 256;i++) // SKIPPING the zeroth element!
		{
		if (m_pppucFastMultiAlpha[i] != pvTemp) // there's a change!
			{
			pvTemp = m_pppucFastMultiAlpha[i];
			sLevel++;
			}

		pf->Write(uint8_t(sLevel));
		}

	//==========  CALCULATE THE START OF THE DATA  ===============

	uint8_t *pucData = &(m_pppucFastMultiAlpha[*pucHeader][m_sStartSrc][m_sStartDst]);
	int32_t lDataLen = int32_t(m_sNumLayers) * int32_t(m_sNumSrc) * int32_t(m_sNumDst);

	//==========  Write out the DATA
	pf->Write(pucData,lDataLen);

	return SUCCESS;
	}

///////////////////////////////////////////////////////////////////////////
//
//	Load:  Load a File version of the FMA and CREATE a memory Version!
// NOTE:  Only by loading an FMA will the wrapper delete it on destruction.
//
///////////////////////////////////////////////////////////////////////////

int16_t RFastMultiAlphaWrapper::Load(RFile* pf)
	{
	ASSERT(pf);
	ASSERT(m_pppucFastMultiAlpha == nullptr);

	int16_t sVer = 1;
	char	szType[32];

	pf->Read(szType);
	if (std::strcmp(szType,"__FMA__"))
		{
		TRACE("RFastMulitAlphaWrapper::Load:  Wrong file type!\n");
		return FAILURE;
		}

	pf->Read(&sVer);
	if (sVer != 1)
		{
		TRACE("RFastMulitAlphaWrapper::Load:  Can't load FMA version %hd!\n",sVer);
		return FAILURE;
		}

	pf->Read(&m_sNumLayers);
	pf->Read(&m_sStartSrc);
	pf->Read(&m_sNumSrc);
	pf->Read(&m_sStartDst);
	pf->Read(&m_sNumDst);

	// RESERVED:
	int16_t sResult;
	int32_t lRes;

	pf->Read(&sResult); 
	pf->Read(&sResult);
	pf->Read(&sResult);
	pf->Read(&sResult);
	pf->Read(&lRes);

	//---------------------- HEADER ------------------
	// Read in the secret header from the FMA:
	uint8_t	ucHeader;
	pf->Read(&ucHeader);

	//-------------------- LAYER INFO ----------------
	// Read in the LEVEL list:
	uint8_t ucLevels[256];  
	pf->Read(ucLevels,256); // we never save element zero!

	//==========  Allocate the FMA  ===============
	int32_t lDataLen = int32_t(m_sNumLayers) * int32_t(m_sNumSrc) * int32_t(m_sNumDst);

	// Assumes a fully correct this pointer!
	int32_t lTotSize = 1024 + int32_t(m_sNumLayers) * m_sNumSrc * (4L + m_sNumDst);
	// align it to 4096 to save cache memory!
	uint8_t*** pppucFastMem = (uint8_t***) calloc(1,lTotSize + 4095);
	if (!pppucFastMem) return FAILURE;

	// WARNING:  these arrays of pointers are 4 bytes in size, so be careful
	// how you seek!

	// I am adding on in BYTES, not words...
   uint8_t*** pppucFastAligned = reinterpret_cast<uint8_t***>((reinterpret_cast<uintptr_t>(pppucFastMem)+4095) & (~4095));

	// For freeing (in bytes):
   int32_t lMemOffset = reinterpret_cast<uintptr_t>(pppucFastAligned) - reinterpret_cast<uintptr_t>(pppucFastMem);

	//==============  Idenitfy the different data sections  ===============
	// NOTE:  once debugged, remove ppucFirstSrcArray,pucData
	
   uint8_t** ppucFirstSrcArray = reinterpret_cast<uint8_t**>(pppucFastAligned + 256); // in 4 byte units!
	uint8_t** ppucCurSrc = ppucFirstSrcArray;

	// ( + 4LS bytes)
   uint8_t* pucData = reinterpret_cast<uint8_t*>(ppucFirstSrcArray + int32_t(m_sNumLayers) * m_sNumSrc);
	uint8_t* pData = pucData;

	//=============  Insert the actual data  ==================

	pf->Read(pData,lDataLen); // one big contiguous chunck

	//=============  Populate the source table  ===============
	int16_t sL,sS,sD;
	uint8_t	**ppucLayers[256]; // only need m_lNumLayers, but this way is fine

	for (sL = 0; sL < m_sNumLayers; sL++)
		{
		ppucLayers[sL] = ppucCurSrc - m_sStartSrc;
		for (sS = 0; sS < m_sNumSrc; sS++)
			{
			*ppucCurSrc++ = pData - m_sStartDst;// Hack test!
			for (sD = 0; sD < m_sNumDst; sD++)
				{
				pf->Read(pData++);
				}
			}
		}

	// Now populate the alpha table based on the stored level numbers:
	for (int16_t i=1;i < 256; i++)
		{
		uint8_t ucLev = ucLevels[i];
		if ((ucLev > 0) && (ucLev <= m_sNumLayers))
			{
			pppucFastAligned[i] = ppucLayers[ucLev-1];
			}
		}

	//=============  Populate the index tables  ===============

	/*
	long lNumSrcTable = m_sNumLayers * m_sNumSrc;
	long lSrcPtrOffset = m_sNumDst;

	uint8_t* pSrcTableValue = pData - m_sStartDst; // offset base dest

	for (long i=0;i < lNumSrcTable;i++,pSrcTableValue += lSrcPtrOffset)
		{
		*ppucCurSrc++ = pSrcTableValue;
		}

	//============  Populate the alpha table  ==================
	long lAlphaPtrOffset = m_sNumSrc; // SHOULD advance by 4
	uint8_t** ppucCurPtrVal = ppucFirstSrcArray - m_sStartSrc; // offset source
	uint8_t*** pppAlphaEntry = pppucFastAligned;

	for (i=0; i < m_sNumLayers; i++,ppucCurPtrVal += lAlphaPtrOffset)
		{
		*pppAlphaEntry++ = ppucCurSrc;
		}

	*/
	// Create the internal header!

	uint8_t* pInfo = (uint8_t*)pppucFastAligned;
	// Remember offsets for each main memory structure:

	// Create the minimum possible data header in BYTES:
	pInfo[0] = ucHeader; // The minimum non-fully-transparent alpha value
	pInfo[1] = uint8_t(m_sNumSrc);  // from this you can calculate remaining values
   pInfo[2] = uint8_t(lMemOffset & UINT8_MAX); // low order byte of mem offset
   pInfo[3] = uint8_t((lMemOffset >> 8) & UINT8_MAX); // high order byte of mem offset

	m_pppucFastMultiAlpha = pppucFastAligned;  // INSTALL!
	m_sAccess = TRUE;
	m_sDelete = TRUE; // NOW the wrapper will delete itself

	return SUCCESS;
	}

///////////////////////////////////////////////////////////////////////////
//
//	IsValid:  Check the source Image's color Range to see if it can be used
// (not a real time operation)
//
///////////////////////////////////////////////////////////////////////////

int16_t RFastMultiAlphaWrapper::IsSrcValid(RImage* pimSrc)
	{
	ASSERT(pimSrc);
	ASSERT(pimSrc->m_sWidth);
	ASSERT(pimSrc->m_sHeight);

	if (pimSrc->m_type != RImage::BMP8) return FALSE;

	int16_t i,j,sStartSrc,sLastSrc;
	GetSrcRange(&sStartSrc,&sLastSrc);

	// The ever so standard 2d memory loop:
	uint8_t* pDst,*pDstLine = pimSrc->m_pData;
	int32_t lP = pimSrc->m_lPitch;
	int16_t	sW = pimSrc->m_sWidth;

	for (j = pimSrc->m_sHeight;j; j--,pDstLine += lP)
		{
		pDst = pDstLine;
		for (i = sW;i;i--,pDst++)
			{
			uint8_t ucPix = *pDst;
			if ((ucPix < sStartSrc) || (ucPix > sLastSrc)) return FALSE;
			}
		}

	return TRUE;
	}
