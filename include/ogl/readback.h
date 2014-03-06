/*
Depth and stencil buffer readback and display.

Orion Sky Lawlor, olawlor@acm.org, 2005/02/21 (Public Domain)
*/
#ifndef __OSL_READBACK_H
#define __OSL_READBACK_H

#include "ogl/util.h"

/**
  Provides operations to support reading color, depth or
  stencil from the screen, and putting it into a texture,
  host memory, or onscreen.
*/
class oglReadback {
	int x,y,wid,ht;
	typedef enum {depth,stencil,color} buftype;
	buftype type;
	GLenum datatype; ///< Texture datatype (e.g., GL_UNSIGNED_CHAR)
	GLenum format; ///< e.g., GL_DEPTH_COMPONENT
	GLenum intlFormat; ///< e.g., GL_RGBA8
	
	// In-memory buffer to hold the read-back data.
	char *buf; int buflen; 
	bool forceMem; /* Read oglTexture from above memory buffer */
	bool memFresh; /* above memory buffer is up-to-date */
	
	// Texture used to display the read-back data.
	oglTexture *tex;
	int tex_wid,tex_ht; buftype tex_type;
	
	// Perform a readback.
	void read(int x,int y,int wid,int ht,buftype t);
public:
	oglReadback() {tex=0; buf=0; buflen=0; forceMem=memFresh=false; wid=ht=0; tex_wid=tex_ht=0;}
	~oglReadback() {delete tex; delete[] buf; buflen=-1;}
	
	/** Read back this portion of the stencil buffer */
	void readStencil(int x,int y,int wid,int ht)
		{read(x,y,wid,ht,stencil);}
	/** Read back this portion of the depth buffer */
	void readDepth(int x,int y,int wid,int ht)
		{read(x,y,wid,ht,depth);}
	/** Read back this portion of the color buffer */
	void readColor(int x,int y,int wid,int ht)
		{read(x,y,wid,ht,color);}
	
	/** After a readback, return the read data. 
	  Stencil data is 8-bit per pixel; depth is 16-bit per pixel.
	  WARNING: host readback is really slow (up to hundreds of ns/pixel).
	*/
	const void *getData(void);
	
	/** After a readback, return a texture to display the data. 
	   Uses glCopyTexSubImage2D if possible.
	*/
	oglTexture *getTex(void);
	
	/** Prepare a texture suitable for the latest readback.	*/
	oglTexture *makeTex(void);
	/** Read the most recent data into this texture.	*/
	void readTex(oglTexture *t);
	
	/** Display read-back data onscreen with the default mapping. */
	void show(void);
};


#endif
