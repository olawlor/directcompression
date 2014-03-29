/*
Assembly and GLSL graphics card programming support.
Supports GLSL high-level shading language.
Supports GL_ARB_fragment_program/GL_ARB_vertex_program assembly language
per-pixel or per-vertex program bindings.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002 (Public Domain)
*/
#ifndef __OGL_PROGRAM_H
#define __OGL_PROGRAM_H

#include "ogl/ext.h" /* MUST call oglSetupExt if you use this header! */
#include "ogl/util.h"

/****************** High-level GLSL programs ***********************/

/**
  Shader to run on each vertex or pixel fragment.
*/
class oglShaderObject {
	int target;// Program target, like GL_FRAGMENT_SHADER_ARB
	GLhandleARB progName; // OpenGL handle to program
public:
	oglShaderObject(int target_) :target(target_) 
		{progName=glCreateShaderObjectARB(target);}
	
	/// Get our handle (e.g., for glAttachObjectARB)
	GLhandleARB get(void) const {return progName;}
	
	// Replace the current program by the one in this file.
	//  Prints out an error message if the program can't be parsed.
	void read(const char *fName);
	
	// Replace the current program by this one.
	//  Prints out an error message if the program can't be parsed.
	void set(const char *programText);
	
	~oglShaderObject() {glDeleteObjectARB(progName);}
};

/**
  Finished GLSL vertex/shader program.
*/
class oglProgramObject {
	oglShaderObject *vert,*frag;
	GLhandleARB prog;
public:
	/** Make a GLSL program from this vertex and fragment shader. 
	  This object keeps the shaders, and will delete them when done. */
	oglProgramObject(oglShaderObject *v,oglShaderObject *f);

	/// Get our handle (e.g., for glUniform call)
	GLhandleARB get(void) const {return prog;}
	
	/** Set the uniform u to this value v.  Works for ints,floats, vectors, and matrices */
	inline void set(const char *u_name,int v) 
		{set(glGetUniformLocationARB(prog,u_name),v);}
	inline void set(int u,int v) 
		{glUniform1iARB(u,v);}

	inline void set(const char *u_name,float v) 
		{set(glGetUniformLocationARB(prog,u_name),v);}
	inline void set(int u,float v) 
		{glUniform1fARB(u,v);}

	inline void set(const char *u_name,double v) 
		{set(glGetUniformLocationARB(prog,u_name),(float)v);}

/* By default, "set" on a "float *" means to set a 4-vector */
	inline void set(const char *u_name,const float *v,int n=1) 
		{set4(glGetUniformLocationARB(prog,u_name),v);}

/* 4-entry float vectors */
	inline void set4(const char *u_name,const float *v,int n=1) 
		{set4(glGetUniformLocationARB(prog,u_name),v);}
	inline void set4(int u,const float *v,int n=1) 
		{glUniform4fvARB(u,n,v);}

/* 3-entry float vectors */
	inline void set3(const char *u_name,const float *v,int n=1) 
		{set3(glGetUniformLocationARB(prog,u_name),v);}
	inline void set3(int u,const float *v,int n=1) 
		{glUniform3fvARB(u,n,v);}

/* 2-entry float vectors */
	inline void set2(const char *u_name,const float *v,int n=1) 
		{set2(glGetUniformLocationARB(prog,u_name),v);}
	inline void set2(int u,const float *v,int n=1) 
		{glUniform2fvARB(u,n,v);}

#ifdef __OSL_VEC4_H
	inline void set(const char *u_name,const vec4 &v) 
		{set4(glGetUniformLocationARB(prog,u_name),v);}
	inline void set(const char *u_name,const vec3 &v) 
		{set3(glGetUniformLocationARB(prog,u_name),v);}
#endif
#ifdef __OSL_MAT4_H
	inline void set(const char *u_name,const osl::mat4 &v) 
		{set(glGetUniformLocationARB(prog,u_name),v);}
	inline void set(int u,const osl::mat4 &v) 
		{glUniformMatrix4fvARB(u,1,0,&v[0][0]);}
#endif

	/** Start using this program */
	void begin(void) { glUseProgramObjectARB(prog); }
	void end(void) { glUseProgramObjectARB(0); }
	
	~oglProgramObject();
};

/* Make an OpenGL program, containing a compiled GLSL shader, from these files,
 which are expected to contain GLSL shader code.   Aborts on errors.
*/
oglProgramObject *oglProgramFromFiles(const char *vertex_file,const char *frag_file);

/* Make an OpenGL program, containing a compiled GLSL shader, from these strings,
 which are expected to contain GLSL shader code.   Aborts on errors.
*/
oglProgramObject *oglProgramFromStrings(const char *vertex_glsl,const char *frag_glsl);

/* String to define a "typical" vertex shader interface */
#define oglProgramObject_vertexPrologue "\n"\
"/* gl_ModelViewMatrix gives world coordinates from model coordinates */\n"\
"uniform mat4 projViewMat; // Projection and view transform (clip from world)\n"\
"uniform vec4 cameraLoc; // Location of camera\n"\
"\n"\
"varying vec4 color; // Material diffuse color\n"\
"varying vec4 position; // World-space coordinates of object\n"\
"varying vec3 normal; // Surface normal\n"\
"varying vec3 cameraDir; // Points toward camera\n"\
"varying vec4 texCoords; // Texture coordinate zero\n"\

/* String to define a "typical" vertex shader main routine.
   Sets all varying parameters and gl_Position. */
#define oglProgramObject_vertexMain "\n"\
"color=gl_Color;\n"\
"texCoords=gl_MultiTexCoord0;\n"\
"position=gl_ModelViewMatrix*gl_Vertex;\n"\
"normal=normalize(gl_NormalMatrix*gl_Normal);\n"\
"cameraDir=normalize(vec3(cameraLoc)-vec3(position));\n"\
"gl_Position=projViewMat*(gl_ModelViewMatrix*gl_Vertex);\n"\

/* String to define a "typical" fragment shader interface */
#define oglProgramObject_fragmentPrologue "\n"\
"uniform vec4 lightDir; // Points toward light\n"\
"\n"\
"varying vec4 color; // Material diffuse color\n"\
"varying vec4 position; // World-space coordinates of object\n"\
"varying vec3 normal; // Surface normal\n"\
"varying vec3 cameraDir; // Points toward camera\n"\
"varying vec4 texCoords; // Texture coordinate zero\n"\

/* Typical fragment shader main routine.*/
#define oglProgramObject_fragmentMain "\n"\
"	vec3 N=normalize(normal);\n"\
"	vec3 H=normalize(cameraDir+vec3(lightDir));\n"\
"	float dotNL=clamp(dot(N,vec3(lightDir)),0.0,1.0);\n"\
"	float dotNH=clamp(dot(N,H),0.0,1.0);\n"\


/****************** Assembly language GL_ARB_fragment_program *************/
/**
  Program to run on each vertex or pixel fragment.
*/
class oglProgram {
	int target;// Program target, like GL_FRAGMENT_PROGRAM_ARB
	GLuint progName; // OpenGL handle to program
	void make(int target_);
public:
	oglProgram(int target_=GL_FRAGMENT_PROGRAM_ARB) {make(target_);}
	oglProgram(const char *str,int target_=GL_FRAGMENT_PROGRAM_ARB)
		{make(target_); set(str);}
	
	/// Make this the current OpenGL program.
	void bind(void) {
		glBindProgramARB(target, progName);
	}
	/// Enable this program-- bind it and enable rendering
	void begin(void) {
		bind();
		glEnable(target);
	}
	/// Stop rendering using this program.
	void end(void) {
		glDisable(target);
	}
	
	// Replace the current program by the one in this file.
	//  Prints out an error message if the program can't be parsed.
	void read(const char *fName);
	
	// Replace the current program by this one.
	//  Prints out an error message if the program can't be parsed.
	void set(const char *programText);
	
	~oglProgram() {
		glDeleteProgramsARB(1,&progName);
	}
};

/**
 Defines a vertex program that computes the points' 
 world coordinates, normal, albedo, and viewing direction.
 You must finish this with "END", like
 oglProgram vp(GL_VERTEX_PROGRAM_ARB); vp.set(oglProgram_vertexStandard "END");
*/
#define oglProgram_vertexStandard \
"!!ARBvp1.0\n"\
"#\n"\
"# Demonstrates a simple vertex shader to compute world coordinates.\n"\
"#\n"\
"# Orion Sky Lawlor, olawlor@acm.org, 2005/2/7 (Public Domain)\n"\
"#\n"\
"\n"\
"# Program/vertex communication protocol\n"\
"PARAM  camera =program.env[0]; # World-coordinates eye location\n"\
"\n"\
"# Vertex/fragment communication protocol\n"\
"OUTPUT world  =result.texcoord[4]; # World-coordinates position\n"\
"OUTPUT worldN =result.texcoord[5]; # World-coordinates normal\n"\
"OUTPUT albedo =result.texcoord[6]; # Material \"diffuse color\"\n"\
"OUTPUT viewdir=result.texcoord[7]; # Viewing direction (points toward eye)\n"\
"\n"\
"# Compute clip coordinates by the multiplying by modelview & projection matrix.\n"\
"#  To draw geometry, OpenGL requires vertex programs to do this. \n"\
"PARAM  mvp[4]	    = { state.matrix.mvp };\n"\
"# Map vertex to clip coordinates, by multiplying by mvp matrix:\n"\
"\n"\
"TEMP P;\n"\
"DP4   P.x, mvp[0], vertex.position; \n"\
"DP4   P.y, mvp[1], vertex.position;\n"\
"DP4   P.z, mvp[2], vertex.position;\n"\
"DP4   P.w, mvp[3], vertex.position;\n"\
"# MUL P.x,P.x,0.3;\n"\
"MOV result.position, P;\n"\
"\n"\
"\n"\
"# Compute world position of vertex by multiplying by modelview\n"\
"PARAM  mv[4]	    = { state.matrix.modelview };\n"\
"TEMP W;\n"\
"DP4   W.x, mv[0], vertex.position; \n"\
"DP4   W.y, mv[1], vertex.position;\n"\
"DP4   W.z, mv[2], vertex.position;\n"\
"MOV   W.w, 1;\n"\
"MOV world,W;\n"\
"\n"\
"# Compute world normal of vertex by multiplying by modelview inverse transpose\n"\
"PARAM  mvi[4]	    = { state.matrix.modelview.invtrans};\n"\
"TEMP N;\n"\
"DP4   N.x, mvi[0], vertex.normal; \n"\
"DP4   N.y, mvi[1], vertex.normal;\n"\
"DP4   N.z, mvi[2], vertex.normal;\n"\
"MOV   N.w, 0;\n"\
"TEMP L;\n"\
"DP4   L.w,N,N; # Normalize N into worldN\n"\
"RSQ   L,L.w;\n"\
"MUL   worldN,N,L;\n"\
"\n"\
"# Copy over diffuse color\n"\
"MOV  albedo, vertex.color;\n"\
"\n"\
"# Compute view direction=(camera-world).normalize();\n"\
"TEMP view;\n"\
"SUB view,camera,W;\n"\
"MOV view.w,0;\n"\
"DP4   L.w,view,view; # Normalize view into viewdir\n"\
"RSQ   L,L.w;\n"\
"MUL   viewdir,view,L;\n"\
"\n"


/**
 Defines a fragment program that has access to the world coordinates,
 normal, color, and viewing direction computed by the vertex program above.
 You must at least write to result.color and add "END", like this:
 oglProgram vp(GL_FRAGMENT_PROGRAM_ARB); 
 vp.set(oglProgram_fragmentStandard 
   "FRC result.color,world\n"
   "END");
*/
#define oglProgram_fragmentStandard \
"!!ARBfp1.0\n"\
"# Demonstrates basic vertex/fragment shader communication.\n"\
"# Orion Sky Lawlor, olawlor@acm.org, 2005/2/7 (Public Domain)\n"\
"#\n"\
"\n"\
"# Vertex/fragment communication protocol\n"\
"ATTRIB world  =fragment.texcoord[4]; # World-coordinates position\n"\
"ATTRIB worldN =fragment.texcoord[5]; # World-coordinates normal\n"\
"ATTRIB albedo =fragment.texcoord[6]; # Material \"diffuse color\"\n"\
"ATTRIB viewdir=fragment.texcoord[7]; # Viewing direction (points toward eye)\n"\
"\n"

/************** GL Shading Lanugage (GLSL) ***************/



#endif /*def(thisHeader)*/

