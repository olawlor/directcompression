/* Capture onscreen frames, for later encoding to a movie.
*/
#include <vector>
#include <fstream>

inline void oglDumpScreen(void) {
	int dims[4]; glGetIntegerv(GL_VIEWPORT,dims);
	int w=dims[2], h=dims[3];
	static std::vector<unsigned char> rgb; /* static avoids repeated allocation */
	rgb.resize(w*h*3);
	glPixelStorei(GL_PACK_ALIGNMENT,1); /* no particular alignment */
	char name[100]; 
	glReadPixels(dims[0],dims[1],w,h, GL_RGB,GL_UNSIGNED_BYTE,&rgb[0]);
	static int imageCount=0;
	sprintf(name,"image%05d.ppm",imageCount++);
	std::ofstream of(name,std::ios_base::binary);
	of<<"P6\n"<<w<<" "<<h<<"\n255\n";
	of.write((char *)&rgb[0],w*h*3);
}
