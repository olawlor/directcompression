/**
Rather than make users futz with a funny Makefile,
you can just #inlcude all the source code of the 
ogl and osl libraries right here.  This only needs
to be done in one place (e.g., from a "main.cpp" file).

Orion Sky Lawlor, olawlor@acm.org, 2005/2/23
*/
#ifndef __OGL_EASYLINK_H
#define __OGL_EASYLINK_H

/* Include all needed library source code files here */
#include "ogl/util.cpp"
#include "ogl/fast_mipmaps.c"
#include "ogl/ext.cpp"
#include "ogl/glew.c"
#include "ogl/main.cpp"
#include "ogl/readback.cpp"
#include "osl/viewpoint.cpp"
#include "ogl/program.cpp"

#endif
