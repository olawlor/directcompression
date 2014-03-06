/*
Local openGL utility routines.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002
*/
#include "ogl/util.h"
#include "ogl/liltex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

oglLilTexs::oglLilTexs(int lilW_,int lilH_)
	:lilW(lilW_), lilH(lilH_) 
{
	/// Allocate image to hold tiles.
	int pixSize=4;
	int len=pixSize*texSize*texSize;
	unsigned char *imgData=new unsigned char[len];
	memset(imgData,0,len);
	tex=new oglTexture(imgData,texSize,texSize,
		oglTexture_linear,GL_RGBA,GL_UNSIGNED_BYTE,
		GL_RGBA8,GL_REPEAT);
	
	/// Number of image tiles along each axis
	int tW=texSize/(lilW+pad), tH=texSize/(lilH+pad);
	
	/// Texture coordinate shifts
	double p2f=1.0/texSize; // pixels to texture fraction
	lilWr=p2f*(lilW+pad); lilHr=p2f*(lilH+pad); 
	lilWv=p2f*lilW; lilHv=p2f*lilH;
	
	/// Set up freelist
	freeHead.x=freeHead.y=-1; /* end of freelist */
	int x,y;
	for (x=0;x<tW;x++) {
		freeImages.push_back(std::vector<subImageRec>());
		for (y=0;y<tH;y++) {
			subImageRec r; r.x=-2; r.y=-2;
			freeImages[x].push_back(r);
			subImageRec cur; cur.x=x; cur.y=y;
			deallocate(cur); // links new tile into freelist
		}
	}
}
	
/// Allocate a new image slot.
///   Sets r and returns true if the next slot is available;
///   returns false if there's no room left.
bool oglLilTexs::allocate(subImageRec &r) {
	if (freeHead.y==-1) return false;
	r=freeHead;
	freeHead=freeImages[freeHead.x][freeHead.y];
	freeImages[r.x][r.y].y=-2;
	return true;
}
void oglLilTexs::upload(const subImageRec &dest,
	const GLubyte *imagePixels, int srcFormat, int srcType)
{
	tex->bind();
	glTexSubImage2D(GL_TEXTURE_2D,0,
		dest.x*(lilW+pad),dest.y*(lilH+pad),
		lilW,lilH,
		srcFormat,srcType,imagePixels);
}

/// Free this old image slot.
void oglLilTexs::deallocate(const subImageRec &r) {
	if (freeImages[r.x][r.y].y!=-2) {
		printf("Freed image slot twice!\n");
		abort();
	}
	freeImages[r.x][r.y]=freeHead;
	freeHead=r;
}


/**
  Stores and looks up a set of oglLilTexs.
*/
class oglLilTexGenerator {
public:
	/// SILLY: linear list of textures.
	///   Should be indexed by image size using a map.
	std::vector<oglLilTexs *> src;
	
	/// Return the parent for a new child of this size.
	oglLilTexs *alloc(int w,int h,oglLilTexs::subImageRec &r)
	{
		for (unsigned int i=0;i<src.size();i++) 
		if (src[i]->lilW==w && src[i]->lilH==h) 
		{ /* here's an image of the right size: */
			if (src[i]->allocate(r))
				return src[i];
		}
		oglLilTexs *ret=new oglLilTexs(w,h);
		src.push_back(ret);
		ret->allocate(r);
		return ret;
	}
};
static oglLilTexGenerator *generator=new oglLilTexGenerator;

oglLilTex::oglLilTex(const GLubyte *imagePixels, int w,int h,
	int srcFormat, int srcType)
{
	enum {doSeparate=128};
	if (w>doSeparate || h>doSeparate)
	{ /* upload a single image */
		tex=new oglTexture(imagePixels,w,h,oglTexture_linear,srcFormat,srcType);
		texs=NULL;
	} else { /* upload a batched image */
		tex=NULL;
		texs=generator->alloc(w,h,r);
		texs->upload(r,imagePixels,srcFormat,srcType);
	}
}
