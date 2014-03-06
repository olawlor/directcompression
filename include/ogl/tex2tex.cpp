/**
 Write a pixel program that converts an input set of textures
 to an output set of textures on the graphics card.
 
 Orion Sky Lawlor, olawlor@acm.org, 2005/12/06 (Public Domain)
*/
#include <ogl/tex2tex.h>

oglTex2tex::oglTex2tex(int wid,int ht,int nOutTex,GLenum outFormat)
	:fbo(wid,ht,0),n_out(nOutTex),vp_default(0)
{
	int i;
	for (i=0;i<maxIO;i++) {
		in[i]=0; in_targ[i]=0; out[i]=0; out_targ[i]=0;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo.get_handle());
	if (outFormat!=0)
		for (int i=0;i<n_out;i++) {
			setOut(fbo.make_tex(outFormat));
		}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
void oglTex2tex::setOut(GLuint tex,int i)
{
	if (out[i]!=0) glDeleteTextures(1,&out[i]);
	out[i]=tex;
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_TEXTURE_2D, tex, 0);
	out_targ[i]=GL_COLOR_ATTACHMENT0_EXT+i;
}

oglTex2tex::~oglTex2tex() {
	int i;
	for (i=0;i<maxIO;i++) {
		if (out[i]!=0) 
			glDeleteTextures(1,&out[i]);
	}
}

const static char *vp_default_str=oglProgram_vertexStandard
"MOV result.texcoord[0], vertex.texcoord[0];\n" /* just copy over tex coordinates */
"END"
;

/// Bind the input textures and output framebuffer,
///  run this fragment program, and reset.  
void oglTex2tex::run(oglProgram *fp,oglProgram *vp) {
	if (vp==0) {
		if (vp_default==0) {
			vp_default=new oglProgram(
				vp_default_str,
				GL_VERTEX_PROGRAM_ARB);
		}
		vp=vp_default;
	}
	begin();
	fp->begin();
	vp->begin();
	
	/* Draw an axis-aligned quad covering our screen */
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(-1,-1,0);
	glTexCoord2f(1,0); glVertex3f(+1,-1,0);
	glTexCoord2f(1,1); glVertex3f(+1,+1,0);
	glTexCoord2f(0,1); glVertex3f(-1,+1,0);
	glEnd();
	
	vp->end();
	fp->end();
	end();
	
}

void oglTex2tex::begin(void)
{  /* Bind in input textures */
	for (int i=0;i<maxIO;i++) 
		if (in[i]!=0) {
			glActiveTextureARB(GL_TEXTURE0_ARB+i);
			glBindTexture(in_targ[i],in[i]);
		}
	glActiveTextureARB(GL_TEXTURE0_ARB+0);
	fbo.begin();
	oglPixelMatrixSentry sentry;
	if (glDrawBuffersATI!=0)
		glDrawBuffersATI(n_out, out_targ);
}

void oglTex2tex::end(void)
{
	fbo.end();
}
