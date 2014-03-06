/*
OpenGL extension routines: allows easy access to,
e.g., ARB_multitexture or ARB_fragment_program.

This is a thin wrapper around the "GLEW" (OpenGL
Extension Wrangler) library available at
	http://glew.sourceforge.net/

Orion Sky Lawlor, olawlor@acm.org, 2003/10/30  (Public Domain)
*/
#ifndef __OSL_GLEXT_H
#define __OSL_GLEXT_H

#include "GL/glew.h"
#include "ogl/util.h"

/* Extensions setup */
void oglSetupExt(void);

/* Multitexture support */

/** Set up one texture combiner source from these fields. */
#define oglTextureCombineOne(n, src_rgb,op_rgb, src_alpha,op_alpha) \
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE##n##_RGB, src_rgb);       \
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND##n##_RGB, op_rgb);       \
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE##n##_ALPHA, src_alpha);   \
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND##n##_ALPHA, op_alpha);   

/** Set up texture combiners using these color and alpha modes. */
void oglTextureCombine(
	GLenum Csrc0,GLenum Cop0, GLenum Cmode, GLenum Csrc1,GLenum Cop1,
	GLenum Asrc0,GLenum Aop0, GLenum Amode, GLenum Asrc1,GLenum Aop1);

/** Set up texture combiners to (linearly) blend this texture in by this factor */
void oglTextureCombineLinear(float v);


#endif /*def(thisHeader)*/
