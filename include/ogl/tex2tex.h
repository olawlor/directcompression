/**
 Lets you write a pixel program that converts an input set of textures
 to an output set of textures on the graphics card.
 
 Orion Sky Lawlor, olawlor@acm.org, 2005/12/06 (Public Domain)
*/
#ifndef __OGL_TEX2TEX_H
#define __OGL_TEX2TEX_H

#include <ogl/program.h>
#include <ogl/framebuffer.h>

/**
 Run this fragment program on these input textures
 to generate (via MRT) these output textures.
 
 Internally uses OpenGL Framebuffer Objects (FBO).
*/
class oglTex2tex {
public:
	/// Framebuffer object used to hold output textures
	oglFramebuffer fbo;
	
	/**
	  Make a render-to-texture buffer with wid x ht pixels.
	  If nOutTex is more than one (the default), you'll use
	  Multiple Render Targets (result.color[i]).  outFormat
	  can be floating-point for floating-point render-to-texture.
	*/
	oglTex2tex(int wid,int ht,
		int nOutTex=1,GLenum outFormat=GL_RGBA8);
	~oglTex2tex();
	
	/// Add this input texture object to this input ARB channel.
	///   You can cheaply repeatedly overwrite texture objects.
	void add(int channel,GLuint texObj,GLenum target=GL_TEXTURE_2D)
	{
		in[channel]=texObj;
		in_targ[channel]=target;
	}
	
	/// Get this output texture.
	GLuint getOut(int outNo=0) {return out[outNo];}
	/// Set the i'th output texture.  Deletes the old texture.
	void setOut(GLuint tex,int i=0);
	int getOutCount(void) const {return n_out;}
	
	
	/// Begin using this framebuffer object
	void begin(void);
	
	/// Stop using this framebuffer object
	void end(void);
	
	/// Bind the input textures and output framebuffer,
	///  run this fragment program, and reset.  
	void run(oglProgram *fp,oglProgram *vp=0);
	
private:
	enum {maxIO=8};
	/// Input texture objects (initially all 0--unused)
	GLuint in[maxIO];
	/// Input texture targets (normally GL_TEXTURE_2D)
	GLenum in_targ[maxIO];
	/// Output texture objects, bound into the FBO
	GLuint out[maxIO];
	/// Output texture destinations (GL_COLOR_ATTACHMENT0_EXT...)
	GLenum out_targ[maxIO];
	int n_out;
	oglProgram *vp_default;
};

#endif
