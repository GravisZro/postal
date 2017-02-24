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

#include "BLIT.H"
#include <ORANGE/QuickMath/Fractions.h>

// Now, 1 bit scaled BLiTting into an BMP1 from an FSPR1
// NO CLIPPING, no colro info!
//
int16_t rspBlitToMono(
				  RImage* pimSrc,
				  RImage* pimDst,
				  int16_t sDstX,
				  int16_t sDstY,
				  int16_t sDstW,
				  int16_t sDstH
				  )
	{
#ifdef _DEBUG

   if ((pimSrc == nullptr) || (pimDst == nullptr))
		{
		TRACE("BLiT: null CImage* passed\n");
      return FAILURE;
		}

	if (pimSrc->m_type != RImage::FSPR1)
		{
		TRACE("BLiT: This form of BLiT is designed for FSPR1 type images!\n");
      return FAILURE;
		}

	if (pimDst->m_type != RImage::BMP1)
		{
		TRACE("BLiT: This functions only BLiTs to BMP1 images.\n");
      return FAILURE;
		}

#endif

	if ( (sDstW < 1) || (sDstH < 1))
		{
		TRACE("BLiT: Zero or negative area passed.\n");
      return FAILURE;
		}

	int32_t	lDstP = pimDst->m_lPitch;

	if ( (sDstX < 0) || (sDstY < 0) ||
		( (sDstX + sDstW) > pimDst->m_sWidth) ||
		( (sDstY + sDstH) > pimDst->m_sHeight) )
		{
		TRACE("BLiT: This BLiT does not yet clip!\n");
      return FAILURE;
		}


	uint8_t	*pDst,*pDstLine,*pCode,ucCount;
	pDstLine = pimDst->m_pData + lDstP * sDstY + (sDstX>>3);
	RSpecialFSPR1*	pHead = (RSpecialFSPR1*)(pimSrc->m_pSpecial);
	pCode = pHead->m_pCode;
   const uint8_t FF = 0xFF;

	// Let's scale it, baby! (pre-clipping)
	int16_t sDenX = pimSrc->m_sWidth; 
	int16_t sDenY = pimSrc->m_sHeight; 
	RFracU16 frX = {0};
	RFracU16 frInitX = {0};
	RFracU16 frOldX = {0};
	RFracU16 frOldY = {0},frY = {0};

   RFracU16 *afrSkipX=nullptr,*afrSkipY=nullptr;
	afrSkipX = rspfrU16Strafe256(sDstW,sDenX);
	afrSkipY = rspfrU16Strafe256(sDstH,sDenY);
	// Make magnification possible:
	int16_t i;
	int32_t *alDstSkip = (int32_t*)calloc(sizeof(int32_t),afrSkipY[1].mod + 2);
	for (i=1;i<(afrSkipY[1].mod + 2);i++) 
		alDstSkip[i] = alDstSkip[i-1] + lDstP;
	uint8_t	bits[] = {128,64,32,16,8,4,2,1};
	int16_t sBit;
	frInitX.mod = (sDstX & 7);

	// ***********************************************************
	// *****************  AT LAST!   CODE!  **********************
	// ***********************************************************
	while (TRUE)
		{
		if ((*pCode) == FF) // vertical run
			{	// end of sprite?
			if ( (ucCount = *(++pCode)) == FF) break; 
			rspfrAdd(frY,afrSkipY[ucCount],sDenY);
			pDstLine += lDstP * (frY.mod - frOldY.mod);
			pCode++; // open stack
			continue; // next line
			}

		if (frOldY.mod == frY.mod) // do a quick skip of a line:
			{
			while ( (*(pCode++)) != FF) ; // skip line!
			rspfrAdd(frY,afrSkipY[1],sDenY);
			pDstLine += alDstSkip[frY.mod - frOldY.mod];
			}
		else // actually draw it!
			{
			frOldY = frY;
			pDst = pDstLine;
			frX.set = frInitX.set; // start of line!
			while ( (ucCount = *(pCode++)) != FF) // EOL
				{
				frOldX = frX;
				rspfrAdd(frX,afrSkipX[ucCount],sDenX);
				//pDst += (frX.mod - frOldX.mod); // skip
				ucCount = *(pCode++);
				frOldX = frX;
				rspfrAdd(frX,afrSkipX[ucCount],sDenX);
				ucCount = uint8_t(frX.mod - frOldX.mod);
				// Modify this to a rect for solid VMagnification.
				pDst = pDstLine + ((frOldX.mod)>>3);
				sBit = frOldX.mod & 7;

				while (ucCount--) 
					{
					(*pDst) |= bits[sBit]; // watch this!
					sBit++;
					if (sBit > 7)
						{
						sBit = 0;
						pDst++;
						}
					}
				}
			rspfrAdd(frY,afrSkipY[1],sDenY);
			pDstLine += alDstSkip[frY.mod - frOldY.mod];
			}
		}

	free(alDstSkip);
	free(afrSkipX);
	free(afrSkipY);

	//======================= for debugging only:
	/*
	CImage* pimScreen,*pimBuffer;
	rspNameBuffers(&pimBuffer,&pimScreen);

	pimDst->Convert(BMP8);
	// copy safebuf to screen:
	rspBlit(pimDst,pimScreen,0,0,0,0,(short)pimDst->lWidth,
					(short)pimDst->lHeight);
	pimDst->Convert(BMP1);
	rspWaitForClick();
	*/

	return SUCCESS;
	}

// mono rect ....
//
int16_t rspRectToMono(uint32_t ulColor,RImage* pimDst,int16_t sX,int16_t sY,
						int16_t sW,int16_t sH)
	{
#ifdef _DEBUG

	if (pimDst->m_type != RImage::BMP1)
		{
		TRACE("rspRectMono: Only BMP1 images supported.\n");
      return FAILURE;
		}

	if ( (sW < 1) || (sH < 1) )
		{
		TRACE("rspRectMono: Zero or negative area passed.\n");
      return FAILURE;
		}
#endif

	if ( (sX < 0) || (sY < 0) || ( (sX + sW) > pimDst->m_sWidth) ||
		( (sY + sH) > pimDst->m_sHeight) )
		{
		TRACE("rspRectMono:Clipping not yet supported.\n");
      return FAILURE;
		}

	int32_t lP = pimDst->m_lPitch;

	uint8_t	ucStart = 0,ucEnd = 0;
	uint8_t *pDst,*pDstLine;
	int16_t sMidCount,sStart,sEnd,i,j;

   uint8_t ucBits[]      = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
   uint8_t ucStartBits[] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
   uint8_t ucEndBits[]   = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

	sStart = (sX >> 3);
	sEnd = (sX + sW - 1);
	int16_t sEndB = (sEnd >> 3);
	sMidCount = sEndB - sStart - 1;
	if (sMidCount < 1) sMidCount = 0;

	if (sStart == sEndB) // very thin:
		for (i= (sX&7);i<=(sEnd&7);i++) ucStart += ucBits[i];
	else // more normal run:
		{
		ucStart = ucStartBits[sX&7];
		ucEnd = ucEndBits[ sEnd & 7 ];
		}
	
	pDstLine = pimDst->m_pData + lP * sY + sStart;

	if (ulColor) // copy a rect of 1 bits:
		{
		for (j=sH;j!=0;j--)
			{
			pDst = pDstLine;
			(*pDst++) |= ucStart;
         for (i=sMidCount;i!=0;i--)
           *(pDst++) = 0xFF;
			(*pDst++) |= ucEnd;
			pDstLine += lP;
			}
		}
	else // copy color 0 rect of bits:
		{
		ucStart = ~ucStart;
		ucEnd = ~ucEnd;
		for (j=sH;j!=0;j--)
			{
 			pDst = pDstLine;
			(*pDst++) &= ucStart;
			for (i=sMidCount;i!=0;i--) *(pDst++) = 0;
			(*pDst++) &= ucEnd;
			pDstLine += lP;
			}
		}
	return SUCCESS;
	}
