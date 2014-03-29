/*
Very simple GLUT program using bare OpenGL libraries to
demonstrate use of GL_ARB_shading_language_100.

Orion Sky Lawlor, olawlor@acm.org, 2006/09/15 (Public Domain)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h> /* GL Extensions Wrangler, http://glew.sourceforge.net/ */

#ifdef USE_MPIGLUT
#  include <GL/mpiglut.h> /* MPI-capable GLUT */
#else
#  include <GL/glut.h> /* WAS: GL Utilities Toolkit, http://freeglut.sourceforge.net/ */
#endif
#include "osl/vec4.h" /* GLSL-style C++ types, by Orion Lawlor */
#include "osl/mat4.h"
using osl::mat4;

//#include "ogl/perflib.cpp"

#include "ogl/util.h"
#include "ogl/program.h"
#include "ogl/program.cpp"
#include "ogl/dumpscreen.h"

#define multigrid_levels 3
#include "multigrid.h"

/* Variables updated by the GUI */
//float center_x=0.0, center_y=1.0; /* center of mandel zooming */
double center_x=-0.7451580638016240, center_y=0.1125749162054177; /* spiral near neck */
double zoom=1.0;
double threshold=0.1;
float aspect=1.0;
int benchmode=0; static int bench_count=0;

void Exit(const char *where,const char *why) {
	fprintf (stderr, "FATAL OpenGL Error in %s: %s\n", where, why);
	abort();	
}

// Check GL for errors after the last command
void Check(const char *where) {
	GLenum e=glGetError();
	if (e==GL_NO_ERROR) return;
	const GLubyte *errString = gluErrorString(e);
	Exit(where, (const char *)errString);
}

/************* Object we'll be drawing ***********/
class mainObject_t {
public:
	mainObject_t();
	void draw(void);
};

mainObject_t::mainObject_t()
{
	/* nothing to set up here... */
}

/** Main drawing routine, called by main_display below */
void mainObject_t::draw(void)
{
	/* Set up programmable shader. */
	static oglProgramObject *p=oglProgramFromFiles("vertex.txt","fragment.txt");

	// Everything we draw from here will use our programmable shader...
	p->begin();
	Check("use");
	
	/* Copy center_x and center_y into program uniforms */
	float fc_x=float(center_x);
	float fl_x=float(center_x-fc_x); // multiprecision low word
	float fc_y=float(center_y);
	float fl_y=float(center_y-fc_y);
	
	p->set("center_x",fc_x);
	p->set("center_y",fc_y);
	p->set("center_low_x",fl_x);
	p->set("center_low_y",fl_y);
	p->set("zoom",zoom);
	
	p->set("aspect",glutGet(GLUT_WINDOW_WIDTH)/float(glutGet(GLUT_WINDOW_HEIGHT)));
	if (benchmode)
		p->set("time",0.125*bench_count);
	else
		p->set("time",0.001*glutGet(GLUT_ELAPSED_TIME));

	/* multiply incoming matrix by subwindow matrix (happens automatically with glLoadMatrix!) */
	mat4 subwindow=mat4(1.0);
#ifdef MPIGLUT_H
	mpiglutGetSubwindowMatrixf(&subwindow[0][0]);
#endif
	p->set("subwindow",subwindow);

	make_multigrid_renderer;
	multigrid_proxy proxy;
	renderer->render(p->get(),threshold,proxy);
		
	/* Stop using programmable shader */
	glUseProgramObjectARB(0);
	
	/* This line causes the world to repeatedly redraw itself, using 100% of the CPU. */
	glutPostRedisplay();
}


/******** Program Initialization *********/
mainObject_t *mainObject=0; /**< Big object we're trying to draw */
int win_w, win_h; /**< Width and height (in pixels) of our window at last resize */

/* GLUT calls this routine every time our window needs to be redrawn */
void main_display(void) {
	//perf_startframe();
	Check("Entering display routine");
	// Clear the color and depth buffers
	glClearColor(0.2,0.3,0.4,1.0); /* set clear color */
	glClearDepth(1.0); /* set clear depth */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); /* actually clear color and depth */
	
	// Set up a little error-prone OpenGL state:
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	
	glFinish();
	double start_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	
	// Draw stuff
	Check("About to draw");
	mainObject->draw();
	Check("Done drawing");
	
	glFinish();
	double end_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	static double run_time=0.0;
	run_time+=end_time-start_time;
	static int frame_count=0;
	frame_count++;
	if (benchmode==1 && run_time>0.2 && frame_count>=2) {
		double spf=run_time/frame_count;
		printf("%d %.2f fps (bench)\n",bench_count,1.0/spf);
		static FILE *log=fopen("bench.txt","w");
		fprintf(log,"%d %.2f fps (bench)\n",bench_count,1.0/spf);
		if ((bench_count%10)==5) oglDumpScreen();
		if (bench_count>60) exit(0);
		zoom*=0.8;
		frame_count=0; run_time=0.0; bench_count++;
	}
	
	// Display the stuff drawn so far to the screen:
	glutSwapBuffers(); //perf_swapframe();
}

/* GLUT calls this routine every time our window changes size */
void main_reshape(int w, int h) {
	win_w=w; win_h=h;
	aspect=win_w/(float)win_h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h); 

	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void main_key(unsigned char key,int x,int y) {
	if (key=='x')
		exit(0);
	
	double speed=0.1*zoom;
	if (key=='w') center_y+=speed;
	if (key=='s') center_y-=speed;
	if (key=='a') center_x-=speed;
	if (key=='d') center_x+=speed;
	if (key=='q') zoom*=0.9;
	if (key=='z') zoom*=1.0/0.9;
	
	const double limit=1.0e-14; // roundoff atrocious even with double-single here
	if (zoom<limit) zoom=limit;
	
	char where[1000];
	snprintf(where,sizeof(where),"Mandelbrot: %.4g@%.16f,%.16f \n",zoom,center_x,center_y);
	printf("%s\n",where);
	glutSetWindowTitle(where);
}

void main_motion(int x,int y) {
	zoom=pow(0.1,x*(5.0/win_w));
	//printf("Zoom = %.6f\n",zoom);
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc,argv);
	
	for (int argi=1;argi<argc;argi++) {
		if (0==strcmp(argv[argi],"-bench")) { benchmode=1; }
		if (0==strcmp(argv[argi],"-threshold") && argi+1<argc) { threshold=atof(argv[++argi]); }
	}
	
	//perf_init();
	glutInitWindowSize(1000,700);

	// Ask for a back buffer, color buffer, and Z buffer in a window.
	int mode=GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH;
	glutInitDisplayMode (mode);
	glutCreateWindow("GLSL Mandelbrot Demo");
	
	// Turn on OpenGL extensions, like programmable shaders
	//   WARNING: you MUST call glutCreateWindow before doing this!
	//   If you don't call glewInit, or call it wrong, you'll segfault!
	glewInit();

	// Ask GLUT to call us when we need to redraw
	glutDisplayFunc (main_display);
	glutReshapeFunc (main_reshape);
	glutMotionFunc (main_motion);
	glutKeyboardFunc (main_key);
	
	// Make an object to draw
	mainObject=new mainObject_t;

	glutMainLoop();
	
	return 0;
}
