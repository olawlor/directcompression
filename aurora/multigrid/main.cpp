/**
  Aurora Rendering demo application, with rendering time compression
  
  Orion Sky Lawlor, olawlor@acm.org, 2014-03-27 (Public Domain)
*/
#include <iostream>
#include <cmath>
#include <GL/glew.h> /* 
OpenGL Extensions Wrangler:  for gl...ARB extentions.  Must call glewInit after glutCreateWindow! */
#include <GL/glut.h> /* OpenGL Utilities Toolkit, for GUI tools */
#include "ogl/glsl.h"
#include "ogl/glsl.cpp"
#define USE_OGL_JOYSTICK 1 /* joystick makes for very smooth camera motion */
#include "ogl/minicam.h"
#include "osl/mat4.h"
#include "osl/mat4_inverse.h"
#include "ogl/dumpscreen.h"
#include "ogl/pixelbench.h"

const float km=1.0/6371.0; // convert kilometers to render units (planet radii)
int benchmode=0;
double threshold=0.2; // total color error to allow before subdividing

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

/* Load an OpenGL Cubemap texture from this directory (using SOIL) */
unsigned int loadCubemap(const std::string &dir) {
	return SOIL_load_OGL_cubemap(
		(dir+"/Xp.jpg").c_str(),
		(dir+"/Xm.jpg").c_str(),
		(dir+"/Yp.jpg").c_str(),
		(dir+"/Ym.jpg").c_str(),
		(dir+"/Zp.jpg").c_str(),
		(dir+"/Zm.jpg").c_str(),
		0, // force channels
		0, // make new texture ID
		SOIL_FLAG_MIPMAPS
	);
}


#include <vector>

#include "multigrid.h" /* multigrid renderer */
multigrid_renderer *renderer=0;


class sphereProxy : public multigrid_proxy {
public:
	void draw() {
		// Draw one *HUGE* sphere; this one proxy geometry covers all our objects.
		//   Keep in mind the far clipping plane in determining the sphere radius
		glutSolidSphere(200.0,3,3); 
	}
};


void display(void) 
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	
	glClearColor(0.4,0.5,0.7,0.0); // background color
	glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
	
	glFinish();
	double start_time=glutGet(GLUT_ELAPSED_TIME)*0.001;
	
	glViewport(0,0, glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // flush any ancient matrices
	gluPerspective(70.0, // fov
		glutGet(GLUT_WINDOW_WIDTH)/(float)glutGet(GLUT_WINDOW_HEIGHT),
		0.1,
		1000.0); // z clipping planes
	float altitude=length(camera);
	float cameraVelocity=0.5*(altitude-0.99);
	oglCameraLookAt(cameraVelocity);
	float min_alt=1.0+1.0e-4; /* minimum allowed altitude */
	if (length(camera)<min_alt) camera=normalize(camera)*min_alt; // boot out of planet
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // flush any ancient matrices
	
	/* Build the pixel shader */
	static GLhandleARB prog=makeProgramObjectFromFiles(
		"raytrace_vertex.txt","raytrace.txt");
	glUseProgramObjectARB(prog);
	glFastUniform3fv(prog,"C",1,camera);
	
	static bool read_imgs=true; /* only read textures on the first frame */
/* Upload planet texture, to texture unit 1 */
	glActiveTexture(GL_TEXTURE1);
	if (read_imgs) read_soil_jpeg("tex/nightearth.png",GL_LUMINANCE8);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,4);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glFastUniform1i(prog,"nightearthtex",1); // texture unit number
	
/* Upload aurora source texture, to texture unit 2 */
	glActiveTexture(GL_TEXTURE2);
	if (read_imgs) read_soil_jpeg("tex/aurora.jpg",GL_RGBA8);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,4);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glFastUniform1i(prog,"auroratex",2); // texture unit number
	
/* Upload aurora distance texture, to texture unit 3 */
	glActiveTexture(GL_TEXTURE3);
	if (read_imgs) read_soil_jpeg("tex/aurora_distance.jpg",GL_LUMINANCE8);
	/* mipmaps screw up distance estimates at bottom of curtains. */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glFastUniform1i(prog,"auroradistance",3); // texture unit number
	
/* Upload deposition lookup texture, to texture unit 4 */
	glActiveTexture(GL_TEXTURE4);
	if (read_imgs) read_soil_jpeg("tex/deposition.bmp",GL_RGBA8);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glFastUniform1i(prog,"auroradeposition",4); // texture unit number
	
/* Upload star cubemap, to texture unit 5 */
	glActiveTexture(GL_TEXTURE5);
	static unsigned int cubemap=0;
	if (read_imgs) {cubemap=loadCubemap("tex/stars");}
	glBindTexture(GL_TEXTURE_CUBE_MAP,cubemap);
	
	glEnable(GL_TEXTURE_CUBE_MAP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,GL_LINEAR);//_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glFastUniform1i(prog,"stars",5); // texture unit number
	
/* Upload cloud texture, to texture unit 6 
	glActiveTexture(GL_TEXTURE6);
	if (read_imgs) read_soil_jpeg("tex/clouds.jpg",GL_LUMINANCE8);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,8);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glFastUniform1i(prog,"clouds",6); // texture unit number
*/
	
	
	read_imgs=false;
	glActiveTexture(GL_TEXTURE0);
	
	glFastUniform1f(prog,"time_uniform",start_time);
	
	// Only draw one set of faces, to avoid drawing raytraced geometry twice.
	//   (front side may get clipped if you're inside the object, so draw back)
	glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA); // premultiplied alpha
	
	
	
	int wid=glutGet(GLUT_WINDOW_WIDTH), ht=glutGet(GLUT_WINDOW_HEIGHT);
	if (!renderer || renderer->wid!=wid || renderer->ht!=ht) {
		delete renderer;
		renderer=new multigrid_renderer(wid,ht);
	}
	sphereProxy proxy;
	renderer->render(prog,threshold,proxy);
	
	glUseProgramObjectARB(0);
	
	if (benchmode==2) {oglPixelBench(display,64,4); return;}
	
	static int movie_mode=0;
	static double last_movieframe=0.0;
	static double frame_interval=1.0/24; /* time between movie frames, seconds */
	if (key_down['p'] || movie_mode || key_down['m']) {
		if (movie_mode) {
			if (last_movieframe+frame_interval<start_time) {
				oglDumpScreen();
				last_movieframe+=frame_interval;
			}
		} else {
			oglDumpScreen();
			last_movieframe=start_time;
		}
		key_down['p']=false; /* p is momentary print-screen; m is for movie */
		if (key_down['m']) movie_mode=1;
		if (key_down['p']) movie_mode=0;
	} 

/* Estimate framerate */
	glFinish();
	static double last_time=0.0;
	static int framecount=0, last_framecount=0;
	framecount++;
	double cur_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	
	
	glutPostRedisplay(); // continual animation
	glutSwapBuffers(); //<- waits for VSYNC?
	
	static double total_time=0.0;
	total_time+=cur_time-start_time;
	if (cur_time>0.1+last_time) 
	{ // every tenth of a second, estimate fps
		double time_per_frame=total_time/(framecount-last_framecount);
		total_time=0.0;
		if (benchmode==1) {
			static FILE *f=fopen("bench.txt","w");
			static int bcount=0;
			int w=glutGet(GLUT_WINDOW_WIDTH),h=glutGet(GLUT_WINDOW_HEIGHT);
			/* count	resolution	frames/sec	milliseconds/frame        nanoseconds/pixel */
			fprintf(f,"%d	%dx%d		%.2f	%.5f	%.3f\n",
				bcount,w,h, 1.0/time_per_frame,
				time_per_frame*1.0e3, time_per_frame/(w*h)*1.0e9);
			
			camera -= 0.002*camera_orient.z; /* move forward */
			
			if (bcount%10==5) {
				oglDumpScreen();
			}
			if (bcount++>60) {
				fclose(f);
				int err=system("cat bench.txt");
				exit(err);
			}
		}
		else { /* not a benchmark, just an ordinary run */
			char str[100];
			sprintf(str,"Aurora Renderer: %.1f fps, %.1f ms/frame (%.1f km)",
				1.0/time_per_frame,1.0e3*time_per_frame,
				(altitude-1.0)/km);
#ifndef MPIGLUT_H
			printf("%s\n",str);

			glutSetWindowTitle(str);

			printf("Minicam: camera=vec3(%f,%f,%f); camera_orient.z=vec3(%f,%f,%f);\n",
				VEC3_TO_XYZ(camera),
				VEC3_TO_XYZ(camera_orient.z));
#endif
		}
		last_framecount=framecount;
		last_time=cur_time;
	}
}

int main(int argc,char *argv[]) 
{
	glutInit(&argc,argv);
	
	// This is high definition 720p
	int w=1280, h=720;
	//int w=1280/4, h=720/4;
#ifdef MPIGLUT_H
	w=8000; h=4000;
#endif
	for (int argi=1;argi<argc;argi++) {
		if (0==strcmp(argv[argi],"-bench")) benchmode=1;
		else if (0==strcmp(argv[argi],"-threshold")) { threshold=atof(argv[++argi]); }
		else if (0==strcmp(argv[argi],"-pixelbench")) benchmode=2;
		else if (2==sscanf(argv[argi],"%dx%d",&w,&h)) {}
		else printf("Unrecognized argument '%s'!\n",argv[argi]);
	}
	
	glutInitDisplayMode(GLUT_RGBA + GLUT_DEPTH + GLUT_DOUBLE);
	glutInitWindowSize(w,h);
	glutCreateWindow("Raytraced Planet (Multigrid)");
	
	glewInit();
	
	glutDisplayFunc(display);
	oglCameraInit();
	//camera=vec3(0,-1.5,0.6); // over continental US
	camera=vec3(-0.3,-0.8,1.1); // over Alaska
	
	// Benchmark: v31, 60km altitude, over Finland, looking toward Iceland:
	camera=vec3(-0.467287,-0.223999,0.868115); 
	camera_orient.z=vec3(0.177043,-0.919557,-0.350815);
	camera_orient.y=camera; /* point up from planet */
	
	correct_head_tilt=false; // allow free flight...
	correct_head_tilt_sphere=true; // ...around sphere
	
	glutMainLoop();
	return 0;
}
