/**
Keep a scrolling list of text messages onscreen.

Messages are formatted with ostreams.
*/
#ifndef __OSL_MESSAGES_H

#include <string> /* for std::string */
#include <sstream> /* for ostringstream */

class oglMessages {
public:
	oglMessages() {cur=0;}
	
	/* Erase all previous messages and reset */
	void clear(void) { 
		for (int i=0;i<cur;i++) streams[i].str("");
		cur=0;
	}
	
	/* Add a new line of text */
	std::ostream &add(void) {
		std::ostream &ret=streams[cur]; 
		cur++; if (cur>=n) cur=0;
		return ret;
	}
	
	/* Draw all existing lines of text.  Draws at 3D origin--shift as needed! */
	void draw(const void *font=GLUT_BITMAP_HELVETICA_12) {
		glDisable(GL_LIGHTING);
        	glDisable(GL_DEPTH_TEST);
		for (int i=0;i<cur;i++) {
			std::string s=streams[i].str();
			const char *str=s.c_str();
        		glRasterPos3f(0,i,0);
        		while (*str!=0) glutBitmapCharacter((void *)font,*str++);
		}
	}
	
private:
	enum {n=20};
	std::ostringstream streams[n];
	int cur; /* next stream to write to */
};


#endif
