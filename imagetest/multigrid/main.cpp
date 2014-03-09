/**
  Demo of multigrid-style renderer: do one coarse-to-fine pass.
  
  Orion Sky Lawlor, olawlor@acm.org, 2014-03-08 (Public Domain)
*/
#include <iostream>
#include <cmath>
#include <GL/glew.h> /* 
OpenGL Extensions Wrangler:  for gl...ARB extentions.  Must call glewInit after glutCreateWindow! */
#include <GL/glut.h> /* OpenGL Utilities Toolkit, for GUI tools */
#include "ogl/glsl.h"
#include "ogl/glsl.cpp"
#include "ogl/minicam.h"
#include "osl/mat4.h"
#include "osl/mat4_inverse.h"
#include "ogl/dumpscreen.h"

const float km=1.0/6371.0; // convert kilometers to render units (planet radii)
int benchmode=0;

/** SOIL **/
#include "soil/SOIL.h" /* Simple OpenGL Image Library, www.lonesock.net/soil.html (plus Dr. Lawlor
's modifications) */
#include "soil/SOIL.c" /* just slap in implementation files here, for easier linking */
#include "soil/stb_image_aug.c"
void read_soil_jpeg(const char *filename, GLenum mode=GL_RGBA8)
{
	printf("Reading texture '%s'",filename); fflush(stdout);
	GLuint srcTex = SOIL_load_OGL_texture
	(
		filename,
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y,
		mode
	);
	printf(".\n");
	glBindTexture(GL_TEXTURE_2D,srcTex);
}


const char *source_image="../real/marble.jpg";


#include <vector>

void display(void) 
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE); 
	
	glClearColor(0.4,0.5,0.7,0.0); // background color
	glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
	
	glViewport(0,0, glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // flush any ancient matrices
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // flush any ancient matrices
	
	/* Build the pixel shader */
	static GLhandleARB prog=makeProgramObjectFromFiles(
		"interpolate_vtx.txt","interpolate.txt");
	glUseProgramObjectARB(prog);
	float pix=10.0;
	glFastUniform2fv(prog,"texdel",1,vec3(pix/1024.0,pix/768.0,0.0));
	
	static bool read_imgs=true; /* only read textures on the first frame */
/* Upload planet texture, to texture unit 1 */
	glActiveTexture(GL_TEXTURE1);
	if (read_imgs) read_soil_jpeg(source_image,GL_RGBA8);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,4);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glFastUniform1i(prog,"srctex",1); // texture unit number
	GLenum target=GL_TEXTURE_2D;
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	GLenum texWrap=GL_CLAMP_TO_EDGE;
	glTexParameteri(target, GL_TEXTURE_WRAP_S, texWrap); 
	glTexParameteri(target, GL_TEXTURE_WRAP_T, texWrap);
	read_imgs=false;
	glActiveTexture(GL_TEXTURE0);
	
	// Only draw one set of faces, to avoid drawing raytraced geometry twice.
	//   (front side may get clipped if you're inside the object, so draw back)
	
	// Draw proxy quad 
	glBegin (GL_QUAD_STRIP);
	glTexCoord2f(0,0); glVertex3d(-1.0,-1.0,0.0); 
	glTexCoord2f(1,0); glVertex3d(+1.0,-1.0,0.0); 
	glTexCoord2f(0,1); glVertex3d(-1.0,+1.0,0.0); 
	glTexCoord2f(1,1); glVertex3d(+1.0,+1.0,0.0); 
	glEnd();
	
	
	glUseProgramObjectARB(0);
	
	if (key_down['p']) {
		oglDumpScreen();
		key_down['p']=false; /* p is momentary print-screen; m is for movie */
	} 
	glutPostRedisplay(); // continual animation
	
	glutSwapBuffers();

/* Estimate framerate */
	static double last_time=0.0;
	static int framecount=0, last_framecount=0;
	framecount++;
	double cur_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	if (cur_time>0.5+last_time) 
	{ // every half a second, estimate fps
		double time=cur_time-last_time;
		double time_per_frame=time/(framecount-last_framecount);
		if (benchmode==1) {
			static FILE *f=fopen("bench.txt","a");
			static int bcount=0;
			int w=glutGet(GLUT_WINDOW_WIDTH),h=glutGet(GLUT_WINDOW_HEIGHT);
			/* count	resolution	frames/sec	milliseconds/frame        nanoseconds/pixel */
			fprintf(f,"%d	%dx%d		%.2f	%.5f	%.3f\n",
				bcount,w,h, 1.0/time_per_frame,
				time_per_frame*1.0e3, time_per_frame/(w*h)*1.0e9);
			if (bcount++>3) {
				fclose(f);
				int err=system("cat bench.txt");
				oglDumpScreen();
				exit(err);
			}
		}
		else { /* not a benchmark, just an ordinary run */
			char str[100];
			sprintf(str,"Interpolator: %.1f fps, %.1f ms/frame",
				1.0/time_per_frame,1.0e3*time_per_frame);
		}
		last_framecount=framecount;
		last_time=cur_time;
	}
}

int main(int argc,char *argv[]) 
{
	glutInit(&argc,argv);
	
	// This is high definition 720p
	int w=1024, h=768;
	//int w=1280/4, h=720/4;
#ifdef MPIGLUT_H
	w=8000; h=4000;
#endif
	for (int argi=1;argi<argc;argi++) {
		if (0==strcmp(argv[argi],"-bench")) benchmode=1;
		else if (0==strcmp(argv[argi],"-pixelbench")) benchmode=2;
		else if (2==sscanf(argv[argi],"%dx%d",&w,&h)) {}
		else printf("Unrecognized argument '%s'!\n",argv[argi]);
	}
	
	glutInitDisplayMode(GLUT_RGBA + GLUT_DOUBLE);
	glutInitWindowSize(w,h);
	glutCreateWindow("Raytracer Image Reconstruction");
	
	glewInit();
	
	glutDisplayFunc(display);
	oglCameraInit();
	
	glutMainLoop();
	return 0;
}
