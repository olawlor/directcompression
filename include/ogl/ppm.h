#ifndef __OGL_PPM_H
#define __OGL_PPM_H

#include <stdio.h>

/* Save a PPM image file of the current framebuffer to this file */
void oglSavePPM(int sx,int sy,int w,int h,const char *fname)
{
	FILE *f=fopen(fname,"wb");
	if (!f) return;
	char *out=new char[3*w*h];
	glPixelStorei(GL_PACK_ALIGNMENT,1); /* byte aligned output */
	glReadPixels(sx,sy,w,h, GL_RGB,GL_UNSIGNED_BYTE,out);
	fprintf(f,"P6\n%d %d\n255\n",w,h);
	for (int y=0;y<h;y++) { /* flip image bottom-to-top on output */
		fwrite(&out[3*(h-1-y)*w],1,3*w,f);
	}
	fclose(f);
	delete[] out;
	printf("Saved PPM image to '%s'...\n",fname);
}

#endif
