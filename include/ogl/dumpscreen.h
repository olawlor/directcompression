/* Capture onscreen frames, for later encoding to a movie.
*/
#include <vector>
#include <fstream>

inline std::vector<unsigned char> oglDumpScreen(int &w,int &h,int numComponents=3) {
	GLenum glComponents=GL_RGB;
	switch (numComponents) {
	case 1: glComponents=GL_ALPHA; break;
	case 3: default: numComponents=3; glComponents=GL_RGB; break;
	case 4: glComponents=GL_RGBA; break;
	}
	
	int dims[4]; glGetIntegerv(GL_VIEWPORT,dims);
	w=dims[2], h=dims[3];
	std::vector<unsigned char> data;
	data.resize(w*h*numComponents);
	glPixelStorei(GL_PACK_ALIGNMENT,1); /* no particular alignment */
	glReadPixels(dims[0],dims[1],w,h, 
		glComponents,
		GL_UNSIGNED_BYTE,&data[0]);
	return data;
}

inline void oglDumpScreen(void) {
	int w,h;
	static std::vector<unsigned char> data; /* static avoids repeated allocation */
	data=oglDumpScreen(w,h,3);
	static int imageCount=0;
	char name[100]; 
	sprintf(name,"image%05d.ppm",imageCount++);
	std::ofstream of(name,std::ios_base::binary);
	of<<"P6\n"<<w<<" "<<h<<"\n255\n";
	of.write((char *)&data[0],w*h*3);
}

inline void oglDumpAlpha(void) {
	int w,h;
	static std::vector<unsigned char> data; /* static avoids repeated allocation */
	data=oglDumpScreen(w,h,1);
	char name[100]; 
	static int imageCount=0;
	sprintf(name,"image%05d_alpha.ppm",imageCount++);
	std::ofstream of(name,std::ios_base::binary);
	of<<"P5\n"<<w<<" "<<h<<"\n255\n";
	of.write((char *)&data[0],w*h);
}
