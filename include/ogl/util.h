/*
Small self-contained openGL utility routines.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002 (Public Domain)
*/
#ifndef __OSL_GLUTIL_H
#define __OSL_GLUTIL_H

#include <stdio.h>
#include "ogl/ext.h"
#include "ogl/util_c.h"
#include "osl/vector3d.h"
typedef osl::Vector3d oglVector3d;

// OpenGL #defines missing on Windows
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

/// Check GL for errors after the last command.
///  Exit if errors were encountered.
void oglCheck(const char *where);

///  Exit with this error message
void oglExit(const char *where,const char *why);


/** Defines the different ways of drawing textures: */
enum oglTexture_t {
  oglTexture_nearest=0, ///< Nearest-neighbor interpolation
  oglTexture_linear=1, ///< Linear interpolation
  oglTexture_mipmap=2 ///< Mipmapping
};

/// Set the OpenGL texture mode to this type.
void oglTextureType(oglTexture_t type,GLenum target=GL_TEXTURE_2D);

/// Windows OpenGL headers don't have GL1.1 CLAMP_TO_EDGE
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV /* missing OpenGL 1.2 symbol */
#define GL_UNSIGNED_INT_8_8_8_8_REV             0x8367
#endif

/// Bind this texture to this OpenGL texture unit
inline void oglBindTexUnit(GLuint texHandle,int unit,GLenum target=GL_TEXTURE_2D) {
	glActiveTextureARB(GL_TEXTURE0_ARB+unit);
	glBindTexture(target,texHandle);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

/// Set the OpenGL wrap mode to this type.
///   Usually either GL_CLAMP_TO_EDGE or GL_REPEAT.
inline void oglTexWrap(GLenum texWrap=GL_CLAMP_TO_EDGE,GLenum target=GL_TEXTURE_2D) {
	glTexParameteri(target, GL_TEXTURE_WRAP_S, texWrap); 
	glTexParameteri(target, GL_TEXTURE_WRAP_T, texWrap);
}

/// Set up 2D OpenGL texture coordinate generation.
///   origin is the texture topright; x and y are the size of the texture.
///  FIXME: assumes x and y are orthogonal.
inline void oglTexGen2d(const oglVector3d &origin,const oglVector3d &x,const oglVector3d &y) 
{
	// Texture coordinate generation: locations are in heightmap samples.
	for (int axis=0;axis<2;axis++) {
		// (texGenParams dot location) gives normalized texture coords
		//   along each texture axis.
		// We want 
		//    texGenParams dot origin == 0
		//    texGenParams dot origin+x == 1
		oglVector3d dir; if (axis==0) dir=x; else dir=y;
		dir/=dir.magSqr(); /* flip image length to be a scale factor */
		double texGenParams[4]={dir.x,dir.y,dir.z,-origin.dot(dir)};
		glTexGeni(GL_S+axis,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
		glTexGendv(GL_S+axis,GL_OBJECT_PLANE,texGenParams);
		glEnable(GL_TEXTURE_GEN_S+axis);
	}
}
inline void oglTexGenDisable(void) {
	glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); 
}

/// Set the maximum mipmap level used to this.  0 means no mipping.
///   By default there is no maximum -- it'll go all the way to 1x1.
inline void oglTexMaxMip(double val) {
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, val);
}

/**
 Bind a (wid x ht) RGBA image to the current texture object.
 This should normally be called only by oglTexture, below.
*/
void oglUploadTexture2D(const GLubyte *imagePixels, int wid,int ht,
		oglTexture_t type=oglTexture_mipmap,
		int srcFormat=GL_RGBA, int srcType=GL_UNSIGNED_BYTE,
		int destFormat=GL_RGBA8,
		int texWrap=GL_CLAMP_TO_EDGE,
		int target=GL_TEXTURE_2D);

/**
  A handle to an OpenGL texture.  Includes *no*
  data-- use oglTexture for a 2D texture, or build
  your own.
*/
class oglTextureName {
	GLuint texName; /* OpenGL texture handle */
	oglTextureName(const oglTextureName &src); /*do not use*/
	void operator=(const oglTextureName &src); /*do not use*/	
public:
	/// Allocate OpenGL handle for this texture.
	///  Does not make the texture active-- you must call bind.
	oglTextureName() {
		texName=0;
		glGenTextures(1,&texName);
	}
	/// Return our OpenGL texture handle
	GLuint getHandle(void) const {return texName;}
	/// Reclaim the texture memory used by this object
	~oglTextureName() {
		glDeleteTextures(1,&texName);
	}
	
	/// Bind this texture into the current environment.
	///   Makes this the current texture -- 
	///   Do this before drawing.
	///   Be sure to do this before uploading,
	///     or messing with the minification or wrap type!
	inline void bind(GLenum target=GL_TEXTURE_2D) const {
		glBindTexture(target, texName);
	}
};

/**
Bind a (wid x ht) RGBA image to an OpenGL texture object.

GL state that only applies to the currently ``bound'' texture, 
and switches when loading a new texture object:
   oglTextureType (linear or mipmap)
   oglWrapType (repeat or clamp)
   texgen coordinates
   
GL state that does not switch:
   GL_TEXTURE_2D enable, texgen modes, TEXTURE_ENV mode.
*/
class oglTexture : public oglTextureName {
public:
	/// Bind a mipmapped OpenGL texture for these pixels
	///  Pixels are copied out, and can be deleted after the call.
	oglTexture(const GLubyte *imagePixels, int wid,int ht,
		oglTexture_t type=oglTexture_mipmap,
		int srcFormat=GL_RGBA, int srcType=GL_UNSIGNED_BYTE,
		int destFormat=GL_RGBA8,
		int texWrap=GL_CLAMP_TO_EDGE,
		int target=GL_TEXTURE_2D);
};

/// Describes how an OpenGL texture is stored in memory.
class oglTextureFormat_t {
public:
	GLenum format; ///< e.g., GL_RGBA or GL_BGRA
	GLenum type; ///< e.g., GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_8_8_8_8_REV
};

/**** osl::graphics2d integration: only enabled if osl/raster.h included before this file */
#ifdef __OSL_RASTER_H

inline oglTextureFormat_t oglTextureFormat(osl::graphics2d::RgbaPixel *p)
{
	oglTextureFormat_t fmt;
#if OSL_LIL_ENDIAN
	fmt.format=GL_BGRA; fmt.type=GL_UNSIGNED_BYTE; /* RgbaPixel in OpenGL on little-endian: BGRA */
#elif OSL_BIG_ENDIAN
	fmt.format=GL_BGRA; fmt.type=GL_UNSIGNED_INT_8_8_8_8_REV; /* RgbaPixel in OpenGL on big-endian: ARGB */
#else
#  error "You must set either OSL_LIL_ENDIAN or OSL_BIG_ENDIAN"
#endif
	return fmt;
}

inline oglTextureFormat_t oglTextureFormat(osl::graphics2d::RGBA_Pixel16 *p)
{
	oglTextureFormat_t fmt;
	fmt.format=GL_RGBA; fmt.type=GL_UNSIGNED_SHORT;
	return fmt;
}


/**
 Create an oglTexture from this RgbaRaster.  
 WARNING: RgbaRaster *cannot* be a sub-image, it must be 
   a complete image (due to direct pixel access).
*/
template <class T>
inline oglTexture *makeOglTexture(const osl::graphics2d::FlatRasterPixT<T> &src,
	oglTexture_t type=oglTexture_mipmap,
	int w=0,int h=0)
{
	const osl::graphics2d::FlatRasterPixT<T> *s=&src;
	osl::graphics2d::FlatRasterPixT<T> expand;
	if (w!=0 || h!=0) { /* Scale image to exactly this size */
		expand.reallocate(w,h);
		osl::graphics2d::Rasterizer r(expand);
		osl::graphics2d::GraphicsState gs;
		gs.scale(w/float(src.wid),h/float(src.ht));
		r.copy(gs,src);
		s=&expand;
	}
	oglTextureFormat_t fmt=oglTextureFormat((T *)0);
	return new oglTexture((const GLubyte *)s->getRawData(),
		s->wid,s->ht,type,
		fmt.format, fmt.type
	);
}

/// Read texture from file.
/// Return 0 if file does not exist
inline oglTexture *makeOglTextureNULL(const char *fName,
	oglTexture_t type=oglTexture_mipmap,
	int w=0,int h=0)
{
	try {
		osl::graphics2d::RgbaRaster r;
		r.read(fName); /* may throw */
		return makeOglTexture(r,type,w,h);
	} catch (osl::Exception *e) {
		delete e;
	}
	return 0;
}
/// Read texture from file or abort
inline oglTexture *makeOglTexture(const char *fName,
	oglTexture_t type=oglTexture_mipmap,
	int w=0,int h=0)
{
	osl::graphics2d::RgbaRaster r(fName);
	return makeOglTexture(r,type,w,h);
}

#endif

#ifdef __OSL_MATRIX2D_H
/** Upload this 2d translation matrix into OpenGL */
inline void oglSetMatrix(const osl::Matrix2d &m)
{
	double v[16];
	v[0]=m(0,0);  v[4]=m(0,1);  v[ 8]=0;       v[12]=m(0,2);
	v[1]=m(1,0);  v[5]=m(1,1);  v[ 9]=0;       v[13]=m(1,2);
	v[2]=0;       v[6]=0;       v[10]=1;       v[14]=0;       
	v[3]=0;       v[7]=0;       v[11]=0;       v[15]=1;       
	glLoadMatrixd(v);
}
#endif

/** Draw an OpenGL texture in a quadrilateral */
void oglTextureQuad(const oglTexture &tex,
	const oglVector3d &tl,const oglVector3d &tr,
	const oglVector3d &bl,const oglVector3d &br);

/** Draw the current texture as a quadrilateral. (b and l are 0 tex coords) */
void oglTextureQuad(
	const oglVector3d &tl,const oglVector3d &tr,
	const oglVector3d &bl,const oglVector3d &br);

/** Draw a line between these two points */
void oglLine(const oglVector3d &s,const oglVector3d &e);

/** Repaint the screen within at least nMilliseconds */
inline void oglRepaint(int nMilliseconds) {
	glutPostRedisplay();
}

/** Call glEnable(mode) if v is true; glDisable(mode) if v is false */
inline void oglEnable(bool v,GLenum mode) {
	if (v) glEnable(mode); else glDisable(mode);
}

/** Return the number of seconds so far */
inline double oglTimer(void) {
	return 0.001*glutGet(GLUT_ELAPSED_TIME);
}

/** Render this string at this 3d location.
  Uses the current color; ignores lighting and depth test.
*/
void oglPrint(const oglVector3d &v,const char *str);

/** Render this string at this 3d location.
*/
void oglPrintf(const oglVector3d &v,const char *fmt,...);

/**
  Set up a directional light source shining from the world
  direction "dir", with this color, and this distribution
  of ambient, diffuse, and specular light.
*/
void oglDirectionLight(GLenum lightNo,const oglVector3d &dir, const oglVector3d &color,
			double ambient,double diffuse,double specular);


/** Switch to 2D, pixel-based rendering--change projection matrix
 so (0,0,0) is screen center, (-1,-1,0) is bottomleft, (1,1) is topright.
 Change remains in effect until object is destroyed.
 */
class oglPixelMatrixSentry {
public:
	oglPixelMatrixSentry() {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
	}
	~oglPixelMatrixSentry() {
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
};

/** Save all matrices and reset to identity */
inline void oglMatrixSave(void) {
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}
/** Restore all matrices and reset to identity */
inline void oglMatrixRestore(void) {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glPopAttrib();
}


/// Draw an untextured screen-filling quad.
inline void oglDrawScreenQuad(void) {
	oglMatrixSave();
	glDisable(GL_LIGHTING);
	glBegin(GL_QUAD_STRIP); 
	glTexCoord2i(1,0); glVertex3i(+1,-1,-1);  glTexCoord2i(1,1); glVertex3i(+1,+1,-1);
	glTexCoord2i(0,0); glVertex3i(-1,-1,-1);  glTexCoord2i(0,1); glVertex3i(-1,+1,-1);
	glEnd();
	glEnable(GL_LIGHTING);
	oglMatrixRestore();
}

/// Stretch this texture across the screen.
inline void oglTexScreenQuad(oglTexture *tex) {
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	oglMatrixSave();
	glDisable(GL_DEPTH_TEST);
	oglTextureQuad(*tex, 
		oglVector3d(-1,+1,0), oglVector3d(+1,+1,0),
		oglVector3d(-1,-1,0), oglVector3d(+1,-1,0));
	oglMatrixRestore();
	glPopAttrib();
}

#endif /*def(thisHeader)*/
