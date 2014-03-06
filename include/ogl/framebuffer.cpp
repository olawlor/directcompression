/**
Wrapper for EXT_framebuffer_object

Orion Sky Lawlor, olawlor@acm.org, 2005/12/06 (Public Domain)
*/
#include <ogl/framebuffer.h>

/// Make a new empty OpenGL texture with this internal format.
GLuint oglFramebuffer::make_tex(GLenum texFormat) {
	oglCheck("before oglFramebuffer::make_tex");
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	/* turn off mipmapping */
	GLenum filt=GL_LINEAR;
	if (texFormat==GL_RGBA_FLOAT16_ATI) filt=GL_NEAREST; /* or else nVidia 6200 fails below */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt);
	glTexImage2D(GL_TEXTURE_2D, 0, texFormat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glBindTexture(GL_TEXTURE_2D,0);
	oglCheck("after oglFramebuffer::make_tex");
	return tex;
}
/// Make a new empty OpenGL renderbuffer with this format.
GLuint oglFramebuffer::make_rb(GLenum rbFormat) {
	if (fb==0) return 0; /* no support */
	GLuint rb;
	glGenRenderbuffersEXT(1, &rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, rbFormat, w, h);
	//glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	return rb;
}

oglFramebuffer::oglFramebuffer(int wid,int ht,GLenum texFormat,GLenum depthFormat,GLenum stencilFormat)
	:w(wid), h(ht)
{
	if (!GLEW_EXT_framebuffer_object) {fb=0;}
	else {
		glGenFramebuffersEXT(1, &fb);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	}
	if (texFormat==0) { tex=0; } 
	else {
		tex=make_tex(texFormat);
		if (fb) glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	}
	
	if (fb==0 || depthFormat==0) { depth=0; }
	else {
		depth=make_rb(depthFormat);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth);
	}
	
	if (fb==0 || stencilFormat==0) { stencil=0; }
	else {
		stencil=make_rb(stencilFormat);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, stencil);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
oglFramebuffer::~oglFramebuffer() {
	if (fb) glDeleteFramebuffersEXT(1,&fb);
	if (tex) glDeleteTextures(1,&tex);
	if (depth) glDeleteRenderbuffersEXT(1,&depth);
	if (stencil) glDeleteRenderbuffersEXT(1,&stencil);
}

static void bad_framebuffer(GLenum status) {
        switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
                oglExit("oglFramebuffer::bind","framebuffer is actually OK (?!)");
                break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                oglExit("oglFramebuffer::bind","combination of formats is UNSUPPORTED by your card");
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                oglExit("oglFramebuffer::bind","ATTACHMENT_EXT");
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                oglExit("oglFramebuffer::bind","MISSING_ATTACHMENT_EXT");
       // case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
       //         oglExit("oglFramebuffer::bind","DUPLICATE_ATTACHMENT_EXT");
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                oglExit("oglFramebuffer::bind","DIMENSIONS_EXT");
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                oglExit("oglFramebuffer::bind","FORMATS_EXT");
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                oglExit("oglFramebuffer::bind","DRAW_BUFFER_EXT");
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                oglExit("oglFramebuffer::bind","READ_BUFFER_EXT");
        default:
                oglExit("oglFramebuffer::bind","GL_FRAMEBUFFER  logic error");
        };
}

void oglFramebuffer::bind(void) {
	if (fb) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status!=GL_FRAMEBUFFER_COMPLETE_EXT) bad_framebuffer(status);
	}
	
	glGetIntegerv(GL_VIEWPORT,old_sz);
	glViewport(0,0,w,h);
	oglCheck("oglFramebuffer::bind()");
}

void oglFramebuffer::unbind(void) 
{
	if (fb) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	else readback();
	glViewport(old_sz[0],old_sz[1],old_sz[2],old_sz[3]);
}

// FIXME: read back depth too (if it's bound)
void oglFramebuffer::readback(void) {
	glBindTexture(GL_TEXTURE_2D,tex);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,w,h);
}
