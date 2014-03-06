/*
Utility routine to agglomerate a series of different-sized textures
into one giant texture.

This header and .cpp file depend on no other files, not even OpenGL.

Orion Sky Lawlor, olawlor@acm.org, 2008-12-01 (Public Domain)
*/
#ifndef __OSL_GL_TEXATLAS_H
#define __OSL_GL_TEXATLAS_H

#include <vector>

/** Trivial 2D pixel location class */
class point2i {
public:
	int x,y;
	point2i(int x_,int y_) :x(x_), y(y_) {}
};
/** Trivial 2D float location class */
class point2f {
public:
	float x,y;
	point2f(float x_,float y_) :x(x_), y(y_) {}
	operator const float * (void) const {return &x;}
};

/**
  An atlas holds an entire set of smaller images.
  
  Typical usage pattern:
  	oglTexAtlas atl;
	for (int myimg=0;...) atl.add(imgs[myimg].wid,imgs[myimg].ht);
	point2i dim=atl.size();
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,
		dim.x,dim.y,
		0,GL_RGBA,GL_UNSIGNED_BYTE,0);
	for (int myimg=0;...) {
		point2i start=atl.lookup(myimg);
		glTexSubImage2D(GL_TEXTURE_2D,0,
			start.x,start.y,
			imgs[myimg].wid,imgs[myimg].ht,
			GL_RGBA,GL_UNSIGNED_BYTE,imgs[myimg].data);
	}
	
	... translate texture coordinates ...
		
*/
class oglTexAtlas {
public:
	/* Make a new empty atlas using this tile size (allocation granularity, in pixels) */
	oglTexAtlas(int tileSize_=64);
	
	/* Make room for this image in the atlas's bookkeeping.  
	   Image size is given in pixels.
	   Returns the image's atlas number, for lookup and translate;
	   image numbers start at zero and increase with each add. */
	int add(int wid,int ht);
	
	/* Return the size, in pixels, of the entire texture atlas.
	  This is useful so you can allocate a texture for the atlas.
	*/
	point2i size(void) const {return P;}
	
	/* Get this image's location, for example to upload new data
	   using glTexSubImage2D or glCopyTexSubImage2D. */
	point2i lookup(int imageNo) const { return cornerP[imageNo]; }
	
	/* Translate texture coordinates from image (0-1) tex coords
	  to atlas (0.125-0.25) texture coordinates.
	  WARNING: if you want GL_WRAP, you'll have to call "fract" from a pixel shader...
	*/
	void translate(int imageNo,float *x,float *y) const {
		point2i c=cornerP[imageNo], p=sizeP[imageNo];
		float atlPx=c.x+(*x)*p.x, atlPy=c.y+(*y)*p.y; /* atlas pixels */
		*x=Pinv.x*atlPx;  *y=Pinv.y*atlPy;
	}
	
private:
	/* Pixels per tile */
	int tileSizeX,tileSizeY;
	/* Tile dimensions of entire texture atlas */
	point2i T;
	/* Pixel dimensions of entire texture atlas */
	point2i P;
	/* 1.0/pixel dimensions of entire texture atlas */
	point2f Pinv;
	
	/* topleft corner (in tiles) of each allocated image */
	std::vector<point2i> cornerT;
	/* topleft corner (in pixels) of each allocated image */
	std::vector<point2i> cornerP;
	/* dimensions (in tiles) each allocated image */
	std::vector<point2i> sizeT;
	/* dimensions (in pixels) each allocated image */
	std::vector<point2i> sizeP;
	
	/* T.x*T.y 2D table indicating which tiles are in use.
	  The flag for tile (x,y) is at inuse[x+T.x*y] 
	*/
	std::vector<bool> inuse;
	
	/* Resize the inuse vector to be this big */
	void resize(int newTx,int newTy);
};


#endif /*def(thisHeader)*/
