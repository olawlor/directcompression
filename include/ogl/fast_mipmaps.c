/**
 * Very fast OpenGL Mipmap-building routines.
 * These are typically a good deal faster than 
 * the builtin gluBuild2DMipmaps.
 * Orion Sky Lawlor, olawlor@acm.org, 2003/2/19 (Public Domain)
 */
#include <stdlib.h>
#include "ogl/util_c.h"

/* This has to be a 4-byte unsigned integral type. */
typedef unsigned int pixel_t;

/**
 * Average four source pixels into one destination pixel across an entire row.
 * A fast version of:
 *    for (int x=0;x<destWid;x++)
 *       dest[x]=(srcLo[2*x]+srcLo[2*x+1]+srcHi[2*x]+srcHi[2*x+1])/4;
 */
void oglBuildFastRow(int destWid,
	const pixel_t *srcLo,
	const pixel_t *srcHi,
	pixel_t *dest)
{
#if 0 /*Slow, obvious version: 
 16 byte loads, 4 byte stores
 20 address calculations (induction-optimizable)
 16 adds
 4 shifts
   -> 9.9/21.7 ns/pixel (total across all MIP levels: alone/with glTexImage2D)
 */
	typedef unsigned char byte;
	const byte *sl=(const byte *)srcLo;
	const byte *sh=(const byte *)srcHi;
	byte *d=(byte *)dest;
	int x,c;
	int channels=4+some_func_evaluates_to_zero();
	for (x=0;x<destWid;x++) { 
		for (c=0;c<channels;c++) {
			unsigned int val=
				sh[4*(2*x+0)+c]+sh[4*(2*x+1)+c]+
				sl[4*(2*x+0)+c]+sl[4*(2*x+1)+c];
			d[4*x+c]=(byte)((val+2)/4u);
		}
	}
#elif 1 /* Clean SIMD-in-a-register version (gives identical results to above):
 4 int loads, 1 int store
 3 address calculations (induction-optimized)
 10 bitwise-ANDs
 9 adds
 5 shifts
   ->  5.9/14.7 ns/pixel (total across all MIP levels: alone/with glTexImage2D)
 */
 	int x;
	for (x=destWid-1;x>=0;x--) {
		const pixel_t evenMask=(pixel_t)0x00ff00ff; /*Even-numbered bytes*/
		const pixel_t oddMask=(pixel_t)0xff00ff00; /*Odd-numbered bytes*/
		const pixel_t oddSMask=oddMask>>2; /* shifted version */
		const pixel_t evenRound=(pixel_t)(0x00020002); /* Roundoff-prevention add */
		const pixel_t oddRound=(pixel_t)(0x02000200u>>2); /* shifted version */
		
		/* Even bytes can be masked out and added, overflowing to odd bytes;
		   then shifted back after the add.
		 */
		pixel_t even=((srcHi[0]&evenMask)+(srcHi[1]&evenMask)
		             +(srcLo[0]&evenMask)+(srcLo[1]&evenMask)+evenRound) >> 2;
		/* Odd bytes have to be pre-shifted, because otherwise the high byte will
		   overflow.  Luckily, this means we don't have to shift later! */
		pixel_t odd =((srcHi[0]>>2)&oddSMask)+((srcHi[1]>>2)&oddSMask)
		            +((srcLo[0]>>2)&oddSMask)+((srcLo[1]>>2)&oddSMask)+oddRound;
		/* Now we can just knit the even and odd bytes back together: */
		*dest++=(even&evenMask)+(odd&oddMask);
		srcLo+=2; srcHi+=2;
	}
#elif 1 /* Quick and dirty SIMD version (drops low two bits of each color--to 6-bit color) 
 4 int loads, 1 int store
 4 ANDs
 3 adds
 4 shifts
   -> 2.2/12.7 ns/pixel (total across all MIP levels: alone/with glTexImage2D)
 */
 	int x;
	for (x=destWid-1;x>=0;x--) {
		const pixel_t loMask=(pixel_t)0x3f3f3f3f; /*All but highest two bits of each byte */
		
		/* We drop off the low two bits of each color */
		*dest++ =    ((srcHi[0]>>2)&loMask)+((srcHi[1]>>2)&loMask)
		            +((srcLo[0]>>2)&loMask)+((srcLo[1]>>2)&loMask);
		srcLo+=2; srcHi+=2;
	}
#endif
}


/**
 * Quickly build mipmaps for a 2D, 4-byte-per-pixel image.
 * Works with any channel order: ARGB, _RGB, RGB_, RGBA, BGRA, BARG, etc.
 * Assumes (wid==ht)
 */
#ifdef __cplusplus
extern "C" /* Allows C++ people to blindly compile and link this file. */
#endif
void oglBuildFast2DMipmaps(int target, int destFormat,int srcWid,int srcHt, 
	int srcFormat, int srcType, const void *pixelData)
{
	pixel_t *tmpBuf1=(pixel_t *)malloc(sizeof(pixel_t)*srcWid*srcHt); /* FIXME: too big... */
	pixel_t *tmpBuf2=(pixel_t *)malloc(sizeof(pixel_t)*srcWid*srcHt); /* FIXME: too big... */
	const pixel_t *src=(const pixel_t *)pixelData;
	int level=0;
	while (1) {
		pixel_t *dest;
		int destWid,destHt, destY;
		
		/* Upload this level to OpenGL */
		glTexImage2D(target, level++, destFormat, 
			srcWid,srcHt, /* border? */ 0, 
			srcFormat, srcType, src);
		
		/* Shrink source by a factor of two into the *other* buffer
		   (starting with tmpBuf1) 
		 */
		dest=(src!=tmpBuf1)?tmpBuf1:tmpBuf2;
		destWid=srcWid>>1, destHt=srcHt>>1;
		if (destWid==0) break;
		for (destY=0;destY<destHt;destY++) {
			int srcY=2*destY;
			oglBuildFastRow(destWid, 
				&src[srcWid*(srcY+0)],&src[srcWid*(srcY+1)],
				&dest[destWid*destY]);
		}
		
		/* Begin the next mip level */
		srcWid=destWid; srcHt=destHt;
		src=dest; 
	}
	free(tmpBuf1); 
	free(tmpBuf2);
}
