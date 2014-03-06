/*
  Quick and dirty header file to read and write PPM images.
  
  Orion Lawlor, lawlor@alaska.edu, 2013-03-18 (Public Domain)
*/
#ifndef __OSL_PPM_H
#define __OSL_PPM_H
#include <iostream> /* for cerr */
#include <stdlib.h> /* for exit */
#include <vector>
#include <fstream>
#include <sstream>

class Image {
public:
	class pixel {
	public:
		typedef unsigned char channel; // data type is one byte per pixel color
		channel v[3]; // R, G, B brightness values
		
		pixel(channel gray=0) {v[0]=v[1]=v[2]=gray;}
		pixel(channel r,channel g,channel b) {v[0]=r; v[1]=g; v[2]=b;}
	};
	int w,h; // pixel dimensions: width is X extent, height is Y extent
	
	std::vector<pixel> data; // row-major pixel storage
	pixel &at(int x,int y) {return data[y*w+x];}
	const pixel &at(int x,int y) const {return data[y*w+x];}
	
	Image() {w=h=0;}
	Image(const char *fName) {read(fName);}
	Image(int nw,int nh) {resize(nw,nh);}
	
	void resize(int nw,int nh) {
		w=nw; h=nh;
		data.resize(w*h);
	}
	
	void read(const char *fName) {
		std::ifstream f(fName,std::ios_base::binary);
		std::string line;
		int mpix=0;
		int state=0;
		while (true) {
			std::getline(f,line);
			if (!f) break; // early EOF?
			std::stringstream ss(line);
			if (line[0]=='#') continue; // comment
			
			if (state==0) {
				if (line!="P6") { std::cerr<<"Sorry, only PPM binary files supported.\n"; exit(1); } 
				else state++;
			}
			else if (state==1) {
				ss>>w>>h; 
				state++;
			} else if (state==2) {
				ss>>mpix; 
				break;
			}
		}
		// FIXME: check mpix==255 and header=="P6"
		resize(w,h);
		f.read((char *)&data[0],w*h*3);
	}
	void write(const char *fName) {
		std::ofstream f(fName,std::ios_base::binary);
		f<<"P6\n"<<w<<" "<<h<<"\n255\n";
		f.write((char *)&data[0],w*h*3);
	}
};

#endif
