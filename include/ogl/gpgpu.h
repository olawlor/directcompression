/**
  General-Purpose Graphics Processing Unit (GPGPU)
  
  Simple OpenGL functions and classes for computing stuff on the graphics card.
  
  This code is entirely contained in this header (and sub-headers); 
  no libraries or other source files are needed except osl/glsl.cpp.
  
  Orion Sky Lawlor, olawlor@acm.org, 2008-04-21 (Public Domain)
*/
#ifndef __OGL_GPGPU_H
#define __OGL_GPGPU_H


#include <GL/glew.h> /*<- for gl...ARB extentions. */
#include <GL/glut.h> /*<- needed to connect to the graphics card. */
#include <stdlib.h> /* for exit() */
#include <stdio.h> /* for printf */
#include <string> /* we pass std::string below */
#include <map> /* used to keep track of uniform variables */
#include <algorithm> /* for std::swap */

#include "ogl/glsl.h" /* Orion's tiny GLSL library */
#include "osl/vec4.h" /*<- handy little vector class */

/* Handy timer function--waits for GPU, and returns seconds of wall-clock time */
double gpgpu_timer(void) {
	glFinish();
	return 0.001*glutGet(GLUT_ELAPSED_TIME);
}

/* Simple, handy GLSL vertex shader for GPGPU work: */
#define GPGPU_V "varying vec2 location;/* onscreen location */\n void main(void) { location=vec2(gl_MultiTexCoord0); gl_Position=gl_Vertex; }\n"

/**
 Draw one screen-filling quadrilateral using the current style.
*/
inline void gpgpu_fillscreen(void) {
	glBegin (GL_TRIANGLE_FAN);
	vec3 lo(-1.0f,-1.0f,0.0f), hi(+1.0f,+1.0f,0.0f);
        glTexCoord2f(0,0); glVertex3fv(vec3(lo.x,lo.y,lo.z)); 
        glTexCoord2f(1,0); glVertex3fv(vec3(hi.x,lo.y,lo.z)); 
        glTexCoord2f(1,1); glVertex3fv(vec3(hi.x,hi.y,lo.z)); 
        glTexCoord2f(0,1); glVertex3fv(vec3(lo.x,hi.y,lo.z)); 
        glEnd();
}

/**
  A gpgpu_texture2D is a 2D rectangular array of pixels that can be read.
  This is the fundamental unit of data storage in OpenGL.
  
  OpenGL's texture coordinates run from:
	0.0 (left edge) to 1.0 (right edge) in X,
	0.0 (bottom edge) to 1.0 (top edge) in Y.

  Note that creating and destroying textures is fairly slow, especially
  when passing in CPU data, so you want upload once and keep this object
  around as long as possible.
*/
class gpgpu_texture2D {
public:
	/** This is OpenGL's integer reference to our texture. */
	GLuint handle;
	
	/** These are the dimensions of our texture (in pixels). 
	  width is the size along X; height is the size along Y.
	*/
	int width,height;
	
	/** 1: greyscale.  3: RGB.  4: RGBA.  0: other (non-float) */
	int floats_per_pixel;
	
	/** GL_LUMINANCE, GL_RGB, GL_RGBA, etc. */
	int gl_format;
	
	/** Create a new texture of this size on the graphics card. 
	  If you pass a NULL data pointer, the image will be uninitialized.
	  The graphics card's internal data storage precision will be 8-bit.
	*/
	gpgpu_texture2D(int w,int h,const float *rgba_data=0,
		GLenum internalFormat=GL_RGBA32F_ARB) 
		:width(w), height(h)
	{
		glGenTextures(1,&handle);
		update_data(rgba_data,internalFormat);
		wrap(GL_CLAMP_TO_EDGE); /* clamp out-of-bounds accesses */
		filter(GL_NEAREST); /* don't do linear filtering */
		/* Build mipmaps.  This is needed to render 
		  into a framebuffer object, and is also just handy. */
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}
	
	/** Destroy this texture */
	~gpgpu_texture2D() {glDeleteTextures(1,&handle); handle=0;}

/********* Texture Use and Rendering ************/
	
	/** Draw this texture onscreen to the currently-bound framebuffer.*/
	void draw(void) {
		bind_tex();
		glEnable(GL_TEXTURE_2D);
		gpgpu_fillscreen();
	}

	/** Bind our texture onto this texture unit of the graphics card. 
	    Textures must be bound before they can be used. */
	void bind_tex(int texture_unit=0) {
		glActiveTexture(GL_TEXTURE0+texture_unit);
		glBindTexture(GL_TEXTURE_2D,handle);
	}
	
	/** Change the data stored in this texture.  rgba_data must contain
	   4*width*height floats. */
	void update_data(const float *rgba_data,GLenum internalFormat=GL_RGBA32F_ARB) {
		bind_tex();
		if (internalFormat==GL_LUMINANCE32F_ARB) {
			floats_per_pixel=1; gl_format=GL_LUMINANCE;
		} else if (internalFormat==GL_RGB32F_ARB)  {
			floats_per_pixel=3; gl_format=GL_RGB;
		} else if (internalFormat==GL_RGBA32F_ARB) {
			floats_per_pixel=4; gl_format=GL_RGBA;
		} else {
			floats_per_pixel=0; gl_format=GL_RGBA;
		} 
		glTexImage2D(GL_TEXTURE_2D,0,internalFormat,
			width,height,0,
			gl_format,GL_FLOAT,rgba_data);
	}
	
/******** Texture State ************/
	/** Set this texture to do this sort of wrapping.
	  Wrapping refers to out-of-bounds texture coordinates. 
	  GL_REPEAT causes the texture's pixels to appear again and again.
	  GL_CLAMP_TO_EDGE clamps out-of-bounds lookups to the closest edge pixel.
	*/
	void wrap(GLenum mode=GL_REPEAT) {
		bind_tex();
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,mode);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,mode);
	}
	/** Set this texture to perform this sort of filtering on lookups.
	  GL_NEAREST filtering returns the nearest pixel value.
	  GL_LINEAR filtering interpolates between nearby pixels, for smoother output.
	*/
	void filter(GLenum mode=GL_LINEAR) {
		bind_tex();
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mode);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,mode);
	}

private:
	/* Sorry, you can't copy or assign GPU textures--the hardware doesn't do it! */
	gpgpu_texture2D(const gpgpu_texture2D &src);
	void operator=(const gpgpu_texture2D &src);
};

/** Set up GLSL Scale variables described below */
inline void gpgpu_tex_scale(GLhandleARB program,gpgpu_texture2D *tex,const std::string &glsl_name)
{
	int scale=glGetUniformLocation(program,(glsl_name+"Scale").c_str());
	if (scale>-1) {
		vec4 s(1.0/tex->width,1.0/tex->height,0.0,0.0);
		glUniform2fvARB(scale,1,s); /* try the vec2 version first */
	}
}

/** Add this texture to this program under this name, using this texture unit.
   In GLSL, you must have declared a "uniform sampler2D <>;" with this name,
   and optionally a "uniform vec2 <>Scale;" texture scale factor,
   which converts from pixel centers to texture coordinates.
   Both the name and unit must be unique within each program.  
 */
inline void gpgpu_add(GLhandleARB program,gpgpu_texture2D *tex,const std::string &glsl_name,int texture_unit=0)
{
	glUseProgramObjectARB(program);
	tex->bind_tex(texture_unit);
	glUniform1iARB(glGetUniformLocation(program,glsl_name.c_str()),texture_unit);
	gpgpu_tex_scale(program,tex,glsl_name);
}

/** Turn off all the crazy stuff we've set up, and go back to plain OpenGL */
inline void gpgpu_reset(void) {
	glUseProgramObjectARB(0); /* no program */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0); /* window framebuffer */
	glViewport(0,0,glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	glActiveTexture(GL_TEXTURE0);
}


/**
  A gpgpu_framebuffer is a way to render new pixel values into a texture.
  It also allows you to 
  
  Note that creating and destroying framebuffer objects is fairly slow,
  so you want to re-use objects whenever possible.
*/
class gpgpu_framebuffer {
public:
	/** OpenGL handle to our framebuffer object. */
	GLuint handle;
	
	/** Texture object that we will render into. */
	gpgpu_texture2D *tex;
	
	/** Create a framebuffer object that will render into this texture. */
	gpgpu_framebuffer(gpgpu_texture2D *tex_) {
		glGenFramebuffersEXT(1,&handle);
		attach(tex_);
	}
	~gpgpu_framebuffer() {glDeleteFramebuffersEXT(1,&handle); handle=0;}
	
	/** Run this OpenGL program to generate all our pixels */
	void run(GLhandleARB prog) {
		bind_fbo();
		glUseProgramObjectARB(prog);
		/* The "uniform vec2 locationScale;" converts location into 
		  onscreen integer pixel coordinate centers.
		  FIXME: don't re-do this uniform lookup every time (somehow) */
		int scale_index=glGetUniformLocationARB(prog,"locationScale");
		if (scale_index>-1) { /* scale back to pixels */
			glUniform4fvARB(scale_index,1,
				vec4(tex->width,tex->height,0.0,0.0));
		}
		gpgpu_fillscreen();
	}
	/** As above, but carefully measures and prints out timing statistics */
	void run_time(GLhandleARB prog) {
		double t1=gpgpu_timer(), elapsed;
		int count=0;
		do {
			count++;
			run(prog);
			elapsed=gpgpu_timer()-t1;
		} while (elapsed<0.01); /* spend a little while doing timing */
		elapsed/=count;
		double per_ns=elapsed/(tex->width*1.0e-9*tex->height);
		printf("%.3f ms to draw %dx%d pixels: %.2f ns/pixel; %.2f Gpixels/second\n",
			1.0e3*elapsed, tex->width,tex->height,
			per_ns, 1.0/per_ns);
	}
	
	/** Extract this rectangle of RGBA pixels out of the GPU onto the CPU. 
	  Warning: this function takes 10us to start up, 
	  plus several ns per float!  Leave things on the graphics card for
	  highest performance.
	*/
	void read(float *destination,
		int copy_width,int copy_height, /* 4-float pixels to copy out */
		int source_x=0,int source_y=0,
		GLenum dest_type=GL_FLOAT) 
	{
		bind_fbo();
		glReadPixels(source_x,source_y,copy_width,copy_height,
			tex->gl_format,dest_type,destination);
	}
	
	/** Bind this framebuffer object as the rendering destination.
	    All subsequent rendering will then write to us! */
	void bind_fbo(void) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,handle);
		if (tex) glViewport(0,0,tex->width,tex->height);
	}
	
	/** Attach this texture to our framebuffer object, as a place to render. 
	  Switching attached textures is slightly faster than switching framebuffer objects.
	*/
	void attach(gpgpu_texture2D *tex_) {
		tex=tex_;
		if (tex) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,handle);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
				tex->handle, 0);
		}
	}
private: /* Sorry, you can't copy or assign framebuffer objects */
	gpgpu_framebuffer(const gpgpu_framebuffer &src);
	void operator=(const gpgpu_framebuffer &src);
};

/************ This is a *very* primitive set of texture output functions *********/
/* This is the current texture to display */
static gpgpu_texture2D *volatile my_glut_draw_tex=0;
static int volatile my_glut_draw_without_pause=0;

#include <setjmp.h> /* for the funky setjmp/longjmp stuff, below */
static jmp_buf my_glut_jmp;
static void my_glut_keydown(unsigned char key,int mouse_x,int mouse_y) {
	longjmp(my_glut_jmp,1); /* exit draw_and_pause function on all keypresses */
}
/* Run glut until above function is called */
static void my_glut_run(void) {
	glutPostRedisplay(); /* makes sure we get a render call */
	if (setjmp(my_glut_jmp)==0)
		glutMainLoop();
	/* else return */
}

/* Simple function to render the window */
inline void my_glut_draw(void) {
	gpgpu_reset();
	if (my_glut_draw_tex) my_glut_draw_tex->draw();
	glutSwapBuffers(); /* show what we've rendered onscreen */
	if (my_glut_draw_without_pause) my_glut_keydown('x',0,0);
}

/** Draw this texture to the screen, and wait for a keypress. */
inline void gpgpu_show_and_pause(gpgpu_texture2D *tex,const char *title_text=0) {
	printf("Showing texture onscreen: %s...\n",title_text);
	if (title_text) glutSetWindowTitle(title_text);
	my_glut_draw_tex=tex;
	my_glut_draw_without_pause=0;
	my_glut_run();
}

/** Draw this texture to the screen, and return as soon as it's visible. */
inline void gpgpu_show(gpgpu_texture2D *tex) {
	my_glut_draw_tex=tex;
	my_glut_draw_without_pause=1;
	my_glut_run();
}

/**
 Set up a *minimal* OpenGL rendering context for showing GPGPU stuff.
 You've got to call this (usually from main) before even *creating* 
 any other gpgpu classes!
*/
inline int gpgpu_init(int window_width=800,int window_height=600,const char *window_title="GPGPU Program") 
{
	static bool gpgpu_initted=false; /* only do the setup code *once* */
	if (gpgpu_initted) return -1; /* already initialized */
	gpgpu_initted=true;
	
	glutInitWindowSize(window_width,window_height);
	int argc=1; char *argv[2]; argv[0]=(char *)"foo"; argv[1]=0; /* fake arguments */
	glutInit(&argc,argv);
	int mode=GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH;
        glutInitDisplayMode (mode);
	int win=glutCreateWindow(window_title);
	glewInit(); /*<- needed for gl...ARB functions to work! */
	
	/* These functions are used from gpgpu_draw, above */
	glutDisplayFunc(my_glut_draw);
	glutKeyboardFunc(my_glut_keydown);
	
	/* We don't usually want these features for GPGPU rendering: */
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	
	return win;
}


/************** Second-layer implementation *****************
Idea is to encapsulate the problems of:
	- naming textures and picking texture units
	- declaring "uniform sampler.."
*/
#include <vector>
#include <string>

class gpu_array; /* forward declaration; real declaration below */

/* Sets a uniform variable inside gpu_env */
class gpgpu_uniform_setter {
protected:
	void *data; // points to user data to set uniform variables to
public:
	virtual void set(const char *name,GLhandleARB prog) =0;
	virtual ~gpgpu_uniform_setter() {}
};

/* Sets a uniform variable to a 3-vector */
class gpgpu_uniform_3fv : public gpgpu_uniform_setter {
public:
	gpgpu_uniform_3fv(float *some_vector) {data=some_vector;}
	virtual void set(const char *name,GLhandleARB prog) { 
		glUniform3fvARB(glGetUniformLocationARB(prog,name),1,(float *)data);
	}
};


/** "Environment" for running GPU code: 
 accumulates a list of textures and utility functions. 
 An environment cannot be deleted until all its gpu_arrays are destroyed.
*/
class gpu_env { 
public:
	std::vector<gpu_array *> tex; /* textures to add to our GLSL codes */
	std::string utils; /* utility functions in GLSL */
	bool time; /* if true, print out timings for each kernel */
	
	/* Add this (new) array to our list of textures. */
	void add_array(gpu_array *t) {
		tex.push_back(t);
	}
	
	/* Add this 3D vector uniform to our list of uniforms */
	void add_uniform3fv(const std::string &name,float *value) {
		if (uniforms.find(name)==uniforms.end()) 
			utils+="uniform vec3 "+name+";\n"; // tell GLSL
		uniforms[name]=new gpgpu_uniform_3fv(value); // upload value when needed
	}
	
	
	/** Creating a default GPU environment automatically initializes OpenGL
	  (if it hasn't already been initialized above). */
	gpu_env(bool already_initted=false) {
		if (!already_initted) gpgpu_init(); 
		time=false;
	}
	
	/* Used by GPU_RUN macros, below */
	void runprep(gpu_array &dest,GLhandleARB code);

private:
	typedef std::map<std::string, gpgpu_uniform_setter *> uniform_list;
	uniform_list uniforms;
};

/** An on-GPU 2D array */
class gpu_array : public gpgpu_texture2D, public gpgpu_framebuffer {
public:
	gpu_env &env; // environment I live in
	std::string name; // my texture's name inside GLSL
	
	gpu_array(gpu_env &env_,std::string glsl_name,
		int w,int h,const float *rgba_data=0,
		GLenum internalFormat=GL_RGBA32F_ARB) 
		:gpgpu_texture2D(w,h,rgba_data,internalFormat),
		 gpgpu_framebuffer(this),
		 env(env_), name(glsl_name)  { env.add_array(this); }
		
	/**
	 Return our texture declarations: uniform sampler2D texA, texB, ...;
	*/
	std::string get_tex_decls(void) {
		std::string s;
		for (unsigned int i=0;i<env.tex.size();i++)
		{
			s+="uniform sampler2D "+env.tex[i]->name+";\n";
			s+="uniform vec2 "+env.tex[i]->name+"Scale;\n";
		/*
			s+="float "+env.tex[i]->name+"_float(float count) {\n"
			"  float f=texture2D("+env.tex[i]->name+",vec2(count,0));\n"
			"  return f;\n"
			"}\n";
		*/
		}
		return s+env.utils;
	}
	
	/**
	  Swap our OpenGL texture and framebuffer with this object.
	  This is useful for "pingpong" type computations:
	     for (...) {
	        GPU_RUN(dest,"gl_FragColor=texture2D(src,...);");
		dest.swapwith(src); // prepare for next pass
	     }
	*/
	void swapwith(gpu_array &arr) {
		std::swap(gpgpu_framebuffer::handle,arr.gpgpu_framebuffer::handle);
		std::swap(gpgpu_texture2D::handle,arr.gpgpu_texture2D::handle);
	}
};

/**
 Prepare the texture environment to run this code
*/
inline void gpu_env::runprep(gpu_array &dest,GLhandleARB code)
{
	// Add all textures
	unsigned int texunit=0;
	for (unsigned int i=0;i<tex.size();i++)
	{
	  if (tex[i]!=&dest) { /* don't add destination as a texture! */
		gpgpu_add(code,tex[i],tex[i]->name,texunit++);
	  } else { /* for destination, set up scale names */
		gpgpu_tex_scale(code,tex[i],tex[i]->name);
	  }
	}
	
	// Add all pre-registered uniforms
	for (uniform_list::iterator it=uniforms.begin();it!=uniforms.end();++it) 
	{
		const std::string &name=(*it).first;
		(*it).second->set(name.c_str(),code);
	}
}

/**
  Run this whole code to generate a new value for our array.  Example:
  	GPU_RUNMAIN(a,void main(void) { float x=texture2D(b,location).r;
		gl_FragColor = vec4(x,0,x,1);   } 
	);
*/
#define GPU_RUNMAIN(gpu_arraydest,someGLSL_maincode) \
{	static std::string gpu_string="varying vec2 location;/* onscreen location */\n"+(gpu_arraydest).get_tex_decls()+ std::string(#someGLSL_maincode ); \
	static GLhandleARB gpu_code=makeProgramObject(GPGPU_V,gpu_string.c_str()); \
	(gpu_arraydest).env.runprep(gpu_arraydest,gpu_code); \
	if ((gpu_arraydest).env.time) (gpu_arraydest).run_time(gpu_code);\
	else (gpu_arraydest).run(gpu_code);\
}

/**
  Run this extended code to generate a new value for our array.  Example:
  	GPU_RUN(a,float x=texture2D(b,location).r; 
		gl_FragColor = vec4(x,0,x,1);
	);
*/
#define GPU_RUN(gpu_arraydest,someGLSL_code) \
	GPU_RUNMAIN(gpu_arraydest, void main(void) { someGLSL_code ; } )

/**
  Run this one-liner code to generate a new value for an array.  Example:
  	GPU_ASSIGN(a,texture2D(b,location));
*/
#define GPU_ASSIGN(gpu_arraydest,someGLSL_code) \
	GPU_RUN(gpu_arraydest,	gl_FragColor = someGLSL_code; )


#endif

