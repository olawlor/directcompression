/* Experimentally determine the performance of each region of the screen,
by benchmarking your display function again and again.

Output is a heat map of the time per pixel:
 DN   Time
 0 -> 0.01 ns/pixel
 50 -> 0.1 ns/pixel
 100 -> 1.0 ns/pixel
 150 -> 10.0 ns/pixel
 200 -> 100.0 ns/pixel
*/
#include <vector>
#include <fstream>
#include <algorithm>
#include "osl/time_function.h"

typedef void (*your_display_fn)(void);

your_display_fn display_store=0;
int display_wrapper(void) {
	glFinish(); //<- avoid GPU batching, makes timings much more regular
	display_store();
	return 0;
}

double oglTimeDisplay(your_display_fn display,int x,int y,int w,int h)
{
	//printf("Benchmarking at %d,%d,%d,%d\n",x,y,w,h);
	glScissor(x,y,w,h);
	display_store=display;
	enum {n=5};
	float t[n];
	for (int i=0;i<n;i++) t[i]=time_function(display_wrapper);
	std::sort(&t[0],&t[n]);
	return t[n/2];
}

unsigned char oglTimeToByte(float seconds_per_pixel) {
	float ns=seconds_per_pixel*1.0e9;
	if (ns<0.0) return 0; /* <- can happen due to errors in base time */
	float l=log10(ns);
	float scaled=l*50+100;
	if (scaled<0) return 0;
	if (scaled>255) return 255;
	else return (unsigned char)scaled;
}

void oglPixelBench(your_display_fn display,int size,int spacing) {
	static int reentered=0;
	if (reentered++>0) return; /* ignore duplicate recursive calls */

	int dims[4]; glGetIntegerv(GL_VIEWPORT,dims);
	int w=dims[2], h=dims[3];
	
	glEnable(GL_SCISSOR_TEST); /* we use scissor to restrict view area */
	double base=0.99*oglTimeDisplay(display,0,0,0,0);
	printf("Base time per display call is %.3f ms\n",base*1.0e3);
	
	std::ofstream of("pixelbench.ppm",std::ios_base::binary);
	of<<"P5\n"<<w/spacing<<" "<<h/spacing<<"\n255\n";
	std::ofstream of2("pixelbench_raw.ppm",std::ios_base::binary);
	of2<<"P5\n"<<w/spacing<<" "<<h/spacing<<"\n255\n";
	for (int y=0;y<h;y+=spacing) {
		for (int x=0;x<w;x+=spacing) {
			double t=oglTimeDisplay(display,x,y,size,size);
			of<<oglTimeToByte((t-base)/(size*size));
			of2<<oglTimeToByte(t/(size*size));
		}
		of.flush();
		of2.flush();
		printf("Done pixelbenching row y=%d\n",y);
	}
	of.close();
	of2.close();
	exit(0);
}
