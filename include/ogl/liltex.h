/*
Utility routines to manage a set of little textures.
These little textures are agglomerated into bigger
textures.  This is needed on many OpenGL drivers, where
drivers get really slow (ATI) or crash (nVidia)
when loading tens of thousands of tiny textures.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002
*/
#ifndef __OSL_GL_LILTEX_H
#define __OSL_GL_LILTEX_H

#include "ogl/util.h" 
#include <vector>

/**
  A 2D array of "tiles", or sub-images,
  each of which represent a separate texture.
  Also keeps free store, which is trivial because
  all the tiles are the same size.
*/
class oglLilTexs {
public:
	/// Describes one tiles in our array.
	///   This is the 0-based tile offset.
	struct subImageRec {
		short x,y;
	};
	/// Size, in pixels, of tiles/sub-images.
	int lilW, lilH;
private:
	/// Padding, in pixels, to add to bottomleft of each image.
	enum {pad=1};
	
	/// Size, in image fraction, *between* tiles/sub-images.
	///  This is used to calculate texture coordinates.
	double lilWr, lilHr;
	
	/// Size, in image fraction, *of* tiles/sub-images.
	///  This is used to calculate texture coordinates.
	double lilWv, lilHv;
	
	oglTexture *tex; ///< Texture image with actual tile array.
	enum {texSize=512}; ///< Size, in pixels, of overall texture image.
	
	/// Singly-linked list of free sub-image slots.
	///   The rec for one slot gives the next free slot;
	///    y==-2 if it's not free, or y==-1 if no more free slots.
	std::vector<std::vector<subImageRec> > freeImages;
	
	/// Next free image slot, or y==-1 if no free slots left.
	subImageRec freeHead;
	
	/// Don't copy or assign oglLilTexHolders.
	oglLilTexs(const oglLilTexs &h);
	void operator=(const oglLilTexs &h);
public:
	/// Allocate a new empty image array to hold tiles of this size.
	oglLilTexs(int lilW_,int lilH_);
	
	/// Allocate a new image slot.
	///   Sets r and returns true if the next slot is available;
	///   returns false if there's no room left.
	bool allocate(subImageRec &r);
	
	/// Upload this data to this tile.
	void upload(const subImageRec &dest,
		const GLubyte *imagePixels, int srcFormat, int srcType);
	
	inline oglTexture *getTexture(void) const {
		return tex;
	}
	
	/// Return the value of this texture coordinate in this tile.
	oglVector3d texCoord(const subImageRec &r,const oglVector3d &v) const {
		return oglVector3d(lilWr*r.x+lilWv*v.x, lilHr*r.y+lilHv*v.y, 0);
	}
	
	/// Free this tile's slot.
	void deallocate(const subImageRec &r);
};

/**
  Keeps a small texture fragment which is just a piece of 
  a larger texture.
*/
class oglLilTex {
	oglTexture *tex; /// Used for large images
	oglLilTexs *texs;
	oglLilTexs::subImageRec r;
public:
	oglLilTex(const GLubyte *imagePixels, int w,int h,
		int srcFormat, int srcType);
	~oglLilTex() {
		if (tex) delete tex;
		else texs->deallocate(r);
	}
	
	inline void bind(void) const {
		getTexture()->bind();
	}
	
	inline oglTexture *getTexture(void) const {
		if (tex) return tex;
		else return texs->getTexture();
	}
	
	/// Return the value of this texture coordinate in this tile.
	oglVector3d texCoord(const oglVector3d &v) const {
		if (tex) return v;
		else return texs->texCoord(r,v);
	}
};


#endif /*def(thisHeader)*/
