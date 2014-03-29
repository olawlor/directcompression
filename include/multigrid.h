/**
  Simple demo multigrid rendering class.
  
  Dr. Orion Lawlor, lawlor@alaska.edu, 2014-03-28 (Public Domain)
*/
#include "ogl/framebuffer.h"  // we use textures and framebuffer objects from here
#include "ogl/framebuffer.cpp"
#include "ogl/util.cpp" // for check and exit functions
#include "ogl/fast_mipmaps.c"

#include "ogl/glsl.h" // glFastUniform GLSL utilities

/**
 This proxy geometry renderer must draw all the pixels in the viewport.
*/
class multigrid_proxy {
public:
	virtual void draw() {
		glBegin (GL_QUAD_STRIP);
		glTexCoord2f(0,0); glVertex3d(-1.0,-1.0,0.0); 
		glTexCoord2f(1,0); glVertex3d(+1.0,-1.0,0.0); 
		glTexCoord2f(0,1); glVertex3d(-1.0,+1.0,0.0); 
		glTexCoord2f(1,1); glVertex3d(+1.0,+1.0,0.0); 
		glEnd();
	}
};


/**
 Renders an image in steps, from coarse to fine resolution.
 
 FIXME: need to decouple and parameterize several things here
 	- Multigrid level count
 	- Estimation scheme
 	- Interpolation scheme
 	- Sampling backend (passed in from user)
*/
class multigrid_renderer {
public:
	int wid,ht; // size of full resolution image
	enum {msaa=0}; // levels of multisample antialiasing: 4^msaa samples per pixel.
	enum {levels=
#if multigrid_levels
	multigrid_levels
#else
	3
#endif
		+msaa}; // multigrid levels are from 0..levels-1.  level==msaa is the full resolution image
	oglFramebuffer *fb[levels];
	
	multigrid_renderer(int wid_,int ht_) 
	{
		wid=wid_; ht=ht_;
		for (int l=0;l<levels;l++) fb[l]=new oglFramebuffer(
			(wid<<msaa)>>l,(ht<<msaa)>>l,GL_RGBA8);
	}
	
	// Convert a framebuffer (size) to a vec4 giving x,y pixel size, z,w 1.0/pixel size
	vec4 framebuffer2vec4(const oglFramebuffer *fbo) {
		return vec4(fbo->w,fbo->h,1.0/fbo->w,1.0/fbo->h);
	}
	
	/**
	 Loop over multigrid levels and do rendering.
	 FIXME: inputs & sampling part of shader should be parameterized
	*/
	void render(GLhandleARB prog,float threshold,multigrid_proxy &pixels) {
		glFastUniform1f(prog,"threshold",threshold);
		
		// Start at coarsest level
		glFastUniform1f(prog,"multigridCoarsest",1.0f);
		fb[levels-1]->bind();
		pixels.draw();
		
		// Loop over finer and finer levels
		for (int l=levels-2;l>=0;l--) {
			float multigridCoarsest=l*1.0/(levels-1.0);
			glFastUniform1f(prog,"multigridCoarsest",multigridCoarsest);
			if (l==0) fb[levels-1]->unbind(); // last step: render to screen
			else fb[l]->bind(); // intermediate step: render to framebuffer
			
			// Bind coarser level to texture:
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D,fb[l+1]->get_color());
			glFastUniform1i(prog,"multigridCoarserTex",7);
			glFastUniform4fv(prog,"multigridCoarser", 1, framebuffer2vec4(fb[l+1]) );
			glFastUniform4fv(prog,"multigridFiner", 1, framebuffer2vec4(fb[l]) );
			
			// Render finer level
			pixels.draw();
		}
		
		glBindTexture(GL_TEXTURE_2D,0); // clear texture state
		glActiveTexture(GL_TEXTURE0);
	}
};

/**
 Allocate a static multigrid_renderer for the current GLUT window.
 Resize the renderer when the window size changes.
*/
#define make_multigrid_renderer \
	static multigrid_renderer *renderer=NULL; \
	int mg_wid=glutGet(GLUT_WINDOW_WIDTH), mg_ht=glutGet(GLUT_WINDOW_HEIGHT); \
	if (!renderer || renderer->wid!=mg_wid || renderer->ht!=mg_ht) { \
		delete renderer; \
		renderer=new multigrid_renderer(mg_wid,mg_ht); \
	}


