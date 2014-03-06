/*
GLSL or GL_ARB_fragment_program (or GL_ARB_vertex_program) per-pixel
or per-vertex program bindings.

Orion Sky Lawlor, olawlor@acm.org, 2005/2/17 (Public Domain)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ogl/program.h"


/******************* GLSL ******************/

void oglShaderObject::read(const char *fName) {
	
	// Read program from file
	int len=65536;
	char *s=new char[len];
	FILE *f=fopen(fName,"r");
	if (!f) oglExit("could open GLSL shader file",fName);
	len=fread(s,1,len,f); // reset len to file length
	s[len]=0; // null terminate string
	fclose(f);
	
	// Upload program to graphics card
	set(s);
	
	delete[] s;
}

static void checkOp(GLhandleARB obj,int what,const char *where) {
	GLint compiled;
	glGetObjectParameterivARB(obj,what,&compiled);
	if (!compiled) {
		printf("Compile error on program: %s\n",where);
		enum {logSize=1000};
		char log[logSize]; int len=0;
		glGetInfoLogARB(obj, logSize,&len,log);
		//printf("Log: \n%s\n",log); exit(1);
		oglExit("compilation of GLSL code:",log);
	}
}
	
void oglShaderObject::set(const char *programText) {
	glShaderSourceARB(progName,1, &programText,NULL);
	glCompileShaderARB(progName);
	checkOp(progName,GL_OBJECT_COMPILE_STATUS_ARB,programText);
	oglCheck(programText);
}


oglProgramObject::oglProgramObject(oglShaderObject *v,oglShaderObject *f)
	:vert(v), frag(f), prog(glCreateProgramObjectARB())
{
	oglCheck("before attach");
	glAttachObjectARB(prog,vert->get());
	glAttachObjectARB(prog,frag->get());

	glLinkProgramARB(prog);
	checkOp(prog,GL_OBJECT_LINK_STATUS_ARB,"link");
	oglCheck("link");
	
	if (0) { /* Linker log can show what runs on hardware */
		char log[65536];int loglen=0;
		glGetInfoLogARB(prog,sizeof(log)-1,&loglen,log);
		log[loglen]=0;
		printf("Log: %s\n",log);
	}
}

oglProgramObject::~oglProgramObject()
{
	glDeleteObjectARB(prog);
	delete vert;
	delete frag;
}

/* Abort if GLSL isn't installed and ready to run. */
void checkGLSL(void) 
{
	/* function pointers to OpenGL routines are NULL if not supported. */
	if (!glCreateShaderObjectARB)
		oglExit("oglProgramObject","Sorry--your OpenGL driver does not support GLSL.  This could be because of either hardware (graphics card too old) or software (old or incomplete drivers).");
}

/* Make an OpenGL program, containing a compiled GLSL shader, from these files,
 which are expected to contain GLSL shader code.   Aborts on errors.
*/
oglProgramObject *oglProgramFromFiles(const char *vertex_file,const char *frag_file) 
{
	checkGLSL();
	oglShaderObject *v=new oglShaderObject(GL_VERTEX_SHADER_ARB);   
	oglShaderObject *f=new oglShaderObject(GL_FRAGMENT_SHADER_ARB); 
	v->read(vertex_file);
	f->read(frag_file);
	return new oglProgramObject(v,f);
}

/* Make an OpenGL program, containing a compiled GLSL shader, from these strings,
 which are expected to contain GLSL shader code.   Aborts on errors.
*/
oglProgramObject *oglProgramFromStrings(const char *vertex_glsl,const char *frag_glsl) 
{
	checkGLSL();
	oglShaderObject *v=new oglShaderObject(GL_VERTEX_SHADER_ARB);   
	oglShaderObject *f=new oglShaderObject(GL_FRAGMENT_SHADER_ARB); 
	v->set(vertex_glsl);
	f->set(frag_glsl);
	return new oglProgramObject(v,f);
}




/***************** Assembly *****************/
void oglProgram::make(int target_)
{
	if (!GLEW_ARB_fragment_program) {
		printf("oglProgram> You need programmable graphics hardware (ARBfp1.0) to run this program!\n");
		exit(1);
	}
	target=target_;
	glGenProgramsARB(1, &progName);
	bind();
}

void oglProgram::read(const char *fName) {
	
	// Read program from file (FIXME: what if program is longer than 65K?)
	int len=65536;
	char *s=new char[len];
	FILE *f=fopen(fName,"r");
	len=(int)fread(s,1,len,f); // reset len to file length
	s[len]=0; // null terminate string
	fclose(f);
	
	// Upload program to graphics card
	set(s);
	
	delete[] s;
}

/**
  Search through this string, and return the start of
  line l, where l is a 1-based line number.
*/
static const char *findLine(const char *t,int l)
{
	if (l<=0) return "Unknown line";
	while (l>1) {
		while (*t!='\n') t++;
		t++; /* advance over newline */
		l--;
	}
	return t;
}
	
void oglProgram::set(const char *programText) {
	oglCheck("Before uploading oglProgram");
	bind();
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, 
		(GLsizei)strlen(programText), programText);
	if (glGetError()!=0) {
		const char *stringErr = (const char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		int badLine=-1;
		sscanf(stringErr,"line %d",&badLine);
		if (badLine!=-1) {
			printf("Error at line %d: %s\n",
				badLine,findLine(programText,badLine));
		} else {
			printf("Error while compiling program:\n"
				"%s\n",programText);
		}
		printf("Error: %s\n",stringErr);
		oglCheck(programText);
	}
}

