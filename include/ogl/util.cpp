/*
Local openGL utility routines.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002
*/
#include "ogl/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> /* for oglPrintf */
#include "osl/vector4d.h" /* for oglDirectionLight */

#ifdef _WIN32 /* For 'MessageBox', down in oglExit */
#   include <windows.h>
#endif

// Check GL for errors after the last command
void oglCheck(const char *where) {
	GLenum e=glGetError();
	if (e==GL_NO_ERROR) return;
	const GLubyte *errString = gluErrorString(e);
	oglExit(where, (const char *)errString);
}

void oglExit(const char *where,const char *why) {
	fprintf (stderr, "FATAL OpenGL Error in %s: %s\n", where, why);
#ifdef _WIN32
    char buf[10240];
    sprintf(buf,"FATAL OpenGL Error in %s: %s\n",where,why);
    MessageBox(0,buf,"Fatal OpenGL Error!",MB_OK|MB_APPLMODAL);
#endif
	abort();	
}


void oglTextureType(oglTexture_t type,GLenum target) {
	switch(type) {
	case oglTexture_nearest:
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		break;
	case oglTexture_linear:
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		break;
	case oglTexture_mipmap:
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		break;
	};
}

/// Pixel conversion routines:
void convertBGRAtoRGBA(GLubyte *dest,const GLubyte *src,int len) 
{
	for (int x=0;x<len;x++) {
		dest[4*x+0]=src[4*x+2]; /* R */
		dest[4*x+1]=src[4*x+1]; /* G */
		dest[4*x+2]=src[4*x+0]; /* B */
		dest[4*x+3]=src[4*x+3]; /* A */
	}
}
void convertARGBtoRGBA(GLubyte *dest,const GLubyte *src,int len) 
{
	for (int x=0;x<len;x++) {
		dest[4*x+0]=src[4*x+1]; /* R */
		dest[4*x+1]=src[4*x+2]; /* G */
		dest[4*x+2]=src[4*x+3]; /* B */
		dest[4*x+3]=src[4*x+0]; /* A */
	}
}

void oglUploadTexture2D(const GLubyte *imagePixels, int wid,int ht,
		oglTexture_t type,
		int srcFormat, int srcType,
		int destFormat,
		int texWrap,
		int target)
{
	oglCheck("before texture data upload (oglUploadTexture2D)");
	const GLubyte *uploadPixels=imagePixels;
	GLubyte *allocPixels=0;
	if (imagePixels==0) {
		allocPixels=new GLubyte[wid*ht*4];
		uploadPixels=allocPixels;
	}
#if ALWAYS_UPLOAD_RGBA
	/* Avoid disasterously slow non-RGBA upload performance on, 
	   e.g., ATI Mobility Radeon 9000, by swapping to RGBA order first. */
	bool fromBGRA=(srcFormat==GL_BGRA && srcType==GL_UNSIGNED_BYTE);
	bool fromARGB=(srcFormat==GL_BGRA && srcType==GL_UNSIGNED_INT_8_8_8_8_REV);
	if ((!allocPixels) && (fromBGRA || fromARGB)) {
		allocPixels=new GLubyte[wid*ht*4];
		uploadPixels=allocPixels;
		for (int y=0;y<ht;y++) {
			if (fromBGRA)
				convertBGRAtoRGBA(&allocPixels[y*wid*4],&imagePixels[y*wid*4],wid);
			if (fromARGB)
				convertARGBtoRGBA(&allocPixels[y*wid*4],&imagePixels[y*wid*4],wid);
		}
		srcFormat=GL_RGBA;
		srcType=GL_UNSIGNED_BYTE;
	}
#endif
	
	if (type==oglTexture_mipmap) { /* build mipmaps */
		if (wid==ht && (srcFormat==GL_RGBA || srcFormat==GL_BGRA)) 
			oglBuildFast2DMipmaps(target, destFormat, wid,ht, 
				srcFormat, srcType, uploadPixels);
		else /* wid!=ht, use slow glu mipmaps */
			gluBuild2DMipmaps(target, destFormat, wid,ht, 
				srcFormat, srcType, uploadPixels);
	} 
	else { /* no mipmaps */
		glTexImage2D(target, 0, destFormat,
			wid,ht, 0, 
			srcFormat, srcType,uploadPixels);
	}
	if (allocPixels) delete[] allocPixels;
	oglCheck("after texture data upload (oglUploadTexture2D)");
	oglTextureType(type);
	oglTexWrap(texWrap);
	/* HACK: old Windows boxes' OpenGL implementations don't support (my favorite)
	  GL_CLAMP_TO_EDGE wrap mode, and so error out with "Invalid Enumerant"
	  after this call.  So we ignore OpenGL errors from the above two calls. */
	glGetError(); /* and ignore that error... */
}

// Bind an OpenGL texture for these pixels
//  Pixels are copied out, and can be deleted after the call.
oglTexture::oglTexture(const GLubyte *imagePixels, int wid,int ht,
	oglTexture_t type,
	int srcFormat, int srcType,
	int destFormat,
	int texWrap,
	int target)
{
	bind(target);
	oglUploadTexture2D(imagePixels,wid,ht,
		type,srcFormat,srcType,destFormat,texWrap,target);
}

void oglTextureQuad(
	const oglVector3d &tl,const oglVector3d &tr,
	const oglVector3d &bl,const oglVector3d &br)
{
	glBegin (GL_QUAD_STRIP);
	glTexCoord2f(0,0); glVertex3d(bl.x, bl.y, bl.z); 
	glTexCoord2f(1,0); glVertex3d(br.x, br.y, br.z); 
	glTexCoord2f(0,1); glVertex3d(tl.x, tl.y, tl.z); 
	glTexCoord2f(1,1); glVertex3d(tr.x, tr.y, tr.z); 
	glEnd();
}

/*Draw an OpenGL texture in a quadrilateral*/
void oglTextureQuad(const oglTexture &tex,
	const oglVector3d &tl,const oglVector3d &tr,
	const oglVector3d &bl,const oglVector3d &br)
{
	tex.bind();
	glEnable(GL_TEXTURE_2D);
	oglTextureQuad(tl,tr,bl,br);
}

/*Draw a line between these two points*/
void oglLine(const oglVector3d &s,const oglVector3d &e) {
	glDisable(GL_TEXTURE_2D);
	glBegin (GL_LINES);
	// glColor3f(1,1,1);
	glVertex3d(s.x, s.y, s.z); 
	glVertex3d(e.x, e.y, e.z); 
	glEnd();
}

/* Render this string */
void oglPrint(const oglVector3d &v,const char *str)
{
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	void *font=GLUT_BITMAP_HELVETICA_12;
	glRasterPos3f(v.x,v.y,v.z);
	while (*str!=0) glutBitmapCharacter(font,*str++);
	glPopAttrib();
}

/** Render this string at this 3d location.
*/
void oglPrintf(const oglVector3d &v,const char *fmt,...) {
	va_list p; va_start(p,fmt);
	char dest[1000];
	vsprintf(dest,fmt,p);
	oglPrint(v,dest);
	va_end(p);
}

/**
  Set up a directional light source shining from the world
  direction "dir", with this color, and this distribution
  of ambient, diffuse, and specular light.
    \param lightNo E.g., GL_LIGHT0 or GL_LIGHT3
    \param dir     Points to light source (world coordinates).
    \param color   Emissive color of light source.
    \param ambient Fraction of color to add as ambient light.
    \param diffuse Fraction of color to add as diffuse light.
    \param specular Fraction of Color(1.0) to add as specular light.
*/
void oglDirectionLight(GLenum lightNo,const oglVector3d &dir, const oglVector3d &color,
			double ambient,double diffuse,double specular)
{
	glEnable(lightNo);
	glLightfv(lightNo, GL_POSITION, osl::Vector4f(dir.dir(),0));
	glLightfv(lightNo, GL_AMBIENT, osl::Vector4f(color*ambient,0.0));
	glLightfv(lightNo, GL_DIFFUSE, osl::Vector4f(color*diffuse,0.0));
	glLightfv(lightNo, GL_SPECULAR, osl::Vector4f(osl::Vector3d(specular),1.0));
}
