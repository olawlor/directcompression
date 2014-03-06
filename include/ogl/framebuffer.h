/*
Wrapper for EXT_framebuffer_object

Orion Sky Lawlor, olawlor@acm.org, 2005/12/06 (Public Domain)
*/
#ifndef __OGL_FRAMEBUFFER_H
#define __OGL_FRAMEBUFFER_H

#include "ogl/ext.h"
#include "ogl/util.h"

/** EXT_framebuffer_object wrapper class. 
  Has skeleton support for machines without EXT_framebuffer_object support,
  by doing buffer readback afterwards.
*/
class oglFramebuffer {
public:
	int w,h; ///< Size of our framebuffer
	int old_sz[4]; ///< Saved dimensions of old viewport
	GLuint fb;///< Framebuffer object
	GLuint depth,stencil; ///< Renderbuffer for depth, stencil (or 0 if none)
	GLuint tex; ///< Texture object to render for color (or 0 if none)
public:
	/**
	 Make a new framebuffer object.
	   @param wid Width of new framebuffer object, in pixels.
	   @param colorFormat Format of framebuffer color, like GL_RGBA8 or GL_RGBA16.  Pass 0 to disable color rendering.
	*/
	oglFramebuffer(int wid,int ht,
		GLenum colorFormat=GL_RGBA8,
		GLenum depthFormat=0, /* e.g., GL_DEPTH_COMPONENT24 */
		GLenum stencilFormat=0); /* e.g., GL_STENCIL_INDEX */
	~oglFramebuffer();
	
	/// Make a new empty OpenGL texture with this internal format.
	///  This texture can be used for, e.g., multiple render targets (MRT).
	GLuint make_tex(GLenum texFormat=GL_RGBA8);
	/// Make a new empty OpenGL renderbuffer with this format.
	GLuint make_rb(GLenum rbFormat=GL_DEPTH_COMPONENT24);
	
	/// Set the color buffer to this texture, like from make_tex.  Pass 0 to disable color.
	inline void set_color(GLuint new_tex) {tex=new_tex;}
	/// Set the depth buffer to this renderbuffer, like from make_rb.  Pass 0 to disable depth.
	inline void set_depth(GLuint rb) {depth=rb;}
	/// Set the stencil buffer to this renderbuffer, like from make_rb.  Pass 0 to disable stencil.
	inline void set_stencil(GLuint rb) {stencil=rb;}
	
	/// Return the color buffer texture handle, or 0 if none.
	inline GLuint get_color(void) const {return tex;}
	/// Return the depth renderbuffer, or 0 if none.
	inline GLuint get_depth(void) const {return depth;}
	/// Return the stencil renderbuffer, or 0 if none.
	inline GLuint get_stencil(void) const {return stencil;}
	
	/// Return our OpenGL Framebuffer Object handle
	inline GLuint get_handle(void) const {return fb;}
	
	/// Begin rendering to this framebuffer.  Does both bind and viewport.
	void bind(void);
	inline void begin(void) {bind();}
	
	/// Finished rendering to this framebuffer.  Does unbind and restores viewpoint
	void unbind(void);
	inline void end(void) {unbind();}
	
private:
	// Slower copy-framebuffer-to-texture version.
	//  Currently only works with color buffer, not depth or stencil.
	void readback(void);
};


#endif
