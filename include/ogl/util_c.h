/*
Local openGL utility routines written in C,
not in C++.  Also includes the appropriate
platform-specific invocation of the OpenGL headers.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002
*/
#ifndef __OSL_GLUTIL_C_H
#define __OSL_GLUTIL_C_H

#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else

#ifdef USE_STDARG /*<- from mpiCC, meaning we should use MPIglut */
//#error MPIglut compile enabled
# include <GL/mpiglut.h>
#else
# include <GL/glut.h>
#endif

# include <GL/gl.h>
# include <GL/glu.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quickly build mipmaps for a 2D, 4-byte-per-pixel image.
 * Works with any channel order: ARGB, _RGB, RGB_, RGBA, BGRA, BARG, etc.
 * WARNING: only works if (wid==ht)
 */
void oglBuildFast2DMipmaps(int target, int destFormat,int srcWid,int srcHt, 
	int srcFormat, int srcType, const void *pixelData);

#ifdef __cplusplus
};
#endif

#endif
