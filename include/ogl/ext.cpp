/*
Local openGL extension routines.

Orion Sky Lawlor, olawlor@acm.org, 2003/10/30
*/
#include "ogl/ext.h"

/* Extensions setup */
void oglSetupExt(void) {
	glewExperimental=1;  /* Try even not-fully-supported extensions */
	glewInit();
}

/** Set up texture combiners using these color and alpha modes. */
void oglTextureCombine(
	GLenum Csrc0,GLenum Cop0, GLenum Cmode, GLenum Csrc1,GLenum Cop1,
	GLenum Asrc0,GLenum Aop0, GLenum Amode, GLenum Asrc1,GLenum Aop1)
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, Cmode);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, Amode);
	oglTextureCombineOne(0,Csrc0,Cop0, Asrc0,Aop0);
	oglTextureCombineOne(1,Csrc1,Cop1, Asrc1,Aop1);
}

/** Set up texture combiners to (linearly) blend this texture in by this factor */
void oglTextureCombineLinear(float v) {
	oglTextureCombine(
		GL_TEXTURE,GL_SRC_COLOR, GL_INTERPOLATE, GL_PREVIOUS,GL_SRC_COLOR,
		GL_TEXTURE,GL_SRC_ALPHA, GL_INTERPOLATE, GL_PREVIOUS,GL_SRC_ALPHA);
	float f[4]={0,0,0,v};
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	oglTextureCombineOne(2,GL_CONSTANT,GL_SRC_ALPHA, GL_CONSTANT,GL_SRC_ALPHA);
}



