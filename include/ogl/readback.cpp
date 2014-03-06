/*
Depth and stencil buffer readback and display.

Orion Sky Lawlor, olawlor@acm.org, 2005/02/21 (Public Domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include "ogl/readback.h"
#include "ogl/main.h" /* for toggles */

// Perform a readback.
void oglReadback::read(int x_,int y_,int wid_,int ht_,buftype t)
{
	oglCheck("before oglReadback read");
	type=t;
	x=x_; y=y_; wid=wid_; ht=ht_;
	memFresh=false;
	switch (type) {
	case depth:
		datatype=GL_UNSIGNED_SHORT;
		format=GL_DEPTH_COMPONENT;
		intlFormat=GL_DEPTH_COMPONENT;
		break;
	case stencil:
		datatype=GL_UNSIGNED_BYTE;
		format=GL_LUMINANCE;
		intlFormat=GL_LUMINANCE8; /* OpenGL doesn't allow GL_STENCIL_INDEX here... */
		forceMem=true;
		break;
	case color:
		datatype=GL_UNSIGNED_BYTE;
		format=GL_RGBA;
		intlFormat=GL_RGBA8;
		break;
	default: abort();
	};
}

/// Round up to nearest power of two.
int roundUp(int sz) {
	int s=1;
	while (s<sz) s*=2;
	return s;
}

/** Prepare a texture suitable for the latest readback.	*/
oglTexture *oglReadback::makeTex(void) {
	int nt_wid=roundUp(wid), nt_ht=roundUp(ht);
	char *garbage=new char[2*nt_wid*nt_ht]; /* texture setup only */
	oglTexture *t=new oglTexture((unsigned char *)garbage,nt_wid,nt_ht,
		oglTexture_linear, format,datatype, intlFormat);
	delete[] garbage;
	return t;
}
/** Read the most recent data into this texture.	*/
void oglReadback::readTex(oglTexture *t) {
	t->bind();
	if (forceMem) { /* Copy from data buffer */
		glTexSubImage2D(GL_TEXTURE_2D,0,
			0,0, wid,ht,
			format,datatype,
			getData());
		oglCheck("oglReadback glTexSubImage2D");
	} else { /* Copy directly from framebuffer */
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,
			x,y, wid,ht);
		oglCheck("oglReadback glCopyTexSubImage2D");
	}
}

/** After a readback, return a texture to display the data. */
oglTexture *oglReadback::getTex(void) {
	int nt_wid=roundUp(wid), nt_ht=roundUp(ht);
	if (tex==NULL || tex_wid!=nt_wid || tex_ht!=nt_ht || tex_type!=type) {
		if (tex) delete tex;
		tex_wid=nt_wid; tex_ht=nt_ht;
		tex_type=type;
		tex=makeTex();
	}
	readTex(tex);
	return tex;
}

const void *oglReadback::getData(void)
{
	int len=((type==depth)?2:(type==stencil)?1:4)*wid*ht;
	if (buflen<len) { /* old buffer won't do-- allocate a new one. */
		if (buf) delete []buf;
		buf=new char[len];
		buflen=len;
	} else { /* Same size as last time */
		if (memFresh) return buf; /* heck-- it IS last time! */
	}
	memFresh=true;
	
	glReadPixels(x,y,wid,ht, 
		(type==depth)?GL_DEPTH_COMPONENT:
		(type==stencil)?GL_STENCIL_INDEX:
		GL_RGBA,
		datatype,buf);
	oglCheck("oglReadback glReadPixels");
	return buf;
}

template <class T>
void getMinMax(const T *src,int len,double *minV,double *maxV) {
	*minV=1.0e30; *maxV=-1.0e30;
	for (int i=0;i<len;i++) {
		if (*minV>src[i]) *minV=src[i];
		if (*maxV<src[i]) *maxV=src[i];
	}
}
template <class T>
void scaleToMinMax(T *val,int len,
	double oldMin,double oldMax,double newMin,double newMax) 
{
	double scale=(newMax-newMin)/(oldMax-oldMin);
	double offset=-scale*oldMin+newMin;
	for (int i=0;i<len;i++) {
		val[i]=(T)(scale*val[i]+offset);
	}
}

template <class T>
void normalizeColor(T *val,int wid,int ht, int maxPossible)
{
	/**
	  Decide how to normalize the displayed color:
	*/
	double minV,maxV;
	getMinMax(val,wid*ht,&minV,&maxV);
	double range=maxV-minV;
	/*
	if (oglKey('m',"Mouse-driven scaling")) {
		double m=(double)val[wid*oglMouseY+oglMouseX];
		printf("Value at %d,%d: %lf (scaled) or %lf\n",oglMouseX,oglMouseY,m,m/(double)maxPossible);
		range/=16;
		minV=m-0.5*range; maxV=m+0.5*range;
	}
	if (oglKey('z',"Zebra-like striped scaling")) {
		maxV=minV+range/16;
	}
	*/
	scaleToMinMax(val,wid*ht,minV,maxV,0,maxPossible);
	// printf("Min/max: %lf - %lf\n",minV,maxV);
}

/** Display read-back data onscreen with a normalized color mapping. */
void oglReadback::show(void) {
/*
	if (!oglKey('n',"Disable depth normalization")) {
		if (type==depth) {
			normalizeColor((unsigned short *)getData(),wid,ht,65535);
		} else {
			normalizeColor((unsigned char *)getData(),wid,ht,255);
		}
	}
*/
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-1,-1,0);
	glScaled(2.0/wid,2.0/ht,1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	
	forceMem=true;
	if (0) {
		oglTextureQuad(*getTex(),
			osl::Vector3d(0,0,0),osl::Vector3d(wid,0,0),
			osl::Vector3d(0,ht,0),osl::Vector3d(wid,ht,0));
	} else {
		getTex()->bind();
		oglTextureType(oglTexture_linear);
		double tx=wid/(float)tex_wid, ty=ht/(float)tex_ht;
		glBegin (GL_QUAD_STRIP);
		glTexCoord2f(0,0); glVertex3d(0,0, 0); 
		glTexCoord2f(tx,0); glVertex3d(wid,0, 0); 
		glTexCoord2f(0,ty); glVertex3d(0,ht, 0); 
		glTexCoord2f(tx,ty); glVertex3d(wid,ht, 0); 
		glEnd();
	}
	forceMem=false;
	glPopAttrib();
}

