/*
Broilerplate GLUT/OpenGL main routine.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ogl/main.h"
#include "ogl/readback.h"
#include "osl/vector4d.h" /* for lighting */

int oglMouseX,oglMouseY,oglMouseButtons;
double oglLastFrameTime;


oglDrawable::~oglDrawable() {}

// Default implementation of all oglEventConsumer events: just ignore it
oglEventConsumer::~oglEventConsumer() {} 
void oglEventConsumer::reshape(int wid,int ht) {}
void oglEventConsumer::mouseHover(int x,int y) {}
void oglEventConsumer::mouseDown(int button,int x,int y) {}
void oglEventConsumer::mouseDrag(int buttonMask,int x,int y) {}
void oglEventConsumer::mouseUp(int button,int x,int y) {}
void oglEventConsumer::handleEvent(int uiCode) {
	switch(uiCode) {
	case 27: /*  Escape key  */
	case 'q': case 'Q': 
		oglExit();
		break;
	case 'P': 
		oglScreenshot(4096,4096,2,"snapshot.ppm");
		break;
	}
	/* Repaint on every keypress */
	oglRepaint(10);
}


// oglTrackballController:
oglTrackballController::oglTrackballController(
			double eyeDist_,double fov_,oglVector3d rotcen_)
	:rotcen(rotcen_), eyeDist(eyeDist_), fov(fov_)
{
	state=stateNone;
}
const oglViewpoint &oglTrackballController::getViewpoint(int width,int height)
{
	double zoomrate=0.0; /* Power of two distance change per second */
	if (oglKeyDown('+',"Zoom in")||oglKeyDown('=',"Zoom in")||oglButton(5,"Zoom in"))
		zoomrate=-2.0;
	if (oglKeyDown('-',"Zoom out")||oglKeyDown('_',"Zoom out")||oglButton(7,"Zoom out"))
		zoomrate=2.0;
	
	double rotrate=2.0; /* radians rotation per second at full throttle */
	double metarate=0.5;
	if (oglButton(6,"Slow down rotation and zoom by 10x")) metarate=0.05;
	rotrate*=metarate; zoomrate*=metarate;
	
	rotrate*=oglLastFrameTime; /* in radians rotation */
	axes.nudge(rotrate*oglAxis(1,"X camera rotation"),-rotrate*oglAxis(2,"Y camera rotation"));
	//printf("Zoom at rate %f over %.3f s\n",zoom,oglLastFrameTime);
	eyeDist*=pow(2.0,zoomrate*oglLastFrameTime);
	
	viewpoint=oglViewpoint(getEye(),rotcen,axes.getY());
	viewpoint.discretize(width,height,fov);

	return viewpoint;
}

void oglTrackballController::mouseDown(int button,int x,int y) {
	lastX=x; lastY=y;
	if (button==0) { /* left mouse: rotate or twirl */
		state=stateRotate;
		oglVector3d screen(viewpoint.getWidth(),viewpoint.getHeight(),0);
		double rotateR=screen.mag()*0.5*0.7;
		double r=(oglVector3d(x,y,0)-0.5*screen).mag();
		if (r>rotateR) /* outer click: twirl */
			state=stateTwirl;
	}
	else /* button != 0 */ { /* zoom */
		state=stateZoom;
	}
}
void oglTrackballController::mouseDrag(int buttonMask,int x,int y) {
	/* Rotation rate, in radians rotation per pixel mouse movement */
	double rate=oglKey('`',"10x slower mouse rotation.")?0.0002:0.002;
	double dx=rate*(x-lastX);
	double dy=rate*(y-lastY);
	switch (state) {
	case stateRotate:
		axes.nudge(dx,dy);
		break;
	case stateZoom:
		eyeDist*=(1.0+dy);
		break;
	case stateTwirl: 
		{
		oglVector3d cen(0.5*viewpoint.getWidth(), 0.5*viewpoint.getHeight(), 0);
		oglVector3d rel(oglVector3d(x,y,0)-cen);
		oglVector3d tan(-rel.y,rel.x,0);
		double rotAng=-oglVector3d(x-lastX,y-lastY,0).dot(tan.dir())/tan.mag();
		axes.rotate(rotAng);
		} break;
	case stateNone:  /* for whining compilers */
		break;
	};
	oglRepaint(10);
	lastX=x; lastY=y;
}

void oglTrackballController::handleEvent(int uiCode) {
	switch(uiCode) {
	default:
		oglEventConsumer::handleEvent(uiCode);
	}
}


/******************* GLUT interface *******************/

static oglOptions myCfg;
static oglController *myCtl=NULL;
static oglDrawable *myScene=NULL;

// oglController:
void oglController::getZRange(double &z_near,double &z_far) {
	z_near=1.0/myCfg.depthRange; z_far=myCfg.depthRange;
}
void oglTrackballController::getZRange(double &z_near,double &z_far) {
	z_near=eyeDist/myCfg.depthRange; z_far=eyeDist*myCfg.depthRange;
}

static double avgTime=0; // Recent time per frame (s)

#define MYCALLBACK(ret) extern "C" ret

MYCALLBACK(void) ogl_main_motionFn(int x, int y)
{
	y=myCfg.height-y; //Flip y around so it matches OpenGL coords.
	oglMouseX=x; oglMouseY=y;
	if (oglMouseButtons==0) myCtl->mouseHover(x,y);
	else /* oglMouseButtons!=0 */ myCtl->mouseDrag(oglMouseButtons,x,y);
}

MYCALLBACK(void) ogl_main_mouseFn(int button, int state, int x, int y)
{
	y=myCfg.height-y; //Flip y around so it matches OpenGL coords.
	oglMouseX=x; oglMouseY=y;
	if (state==GLUT_DOWN) {
		oglMouseButtons|=(1<<button); /* Set the bit for this button */
		myCtl->mouseDown(button,x,y);
	}
	if (state==GLUT_UP) {
		myCtl->mouseDrag(oglMouseButtons,x,y);
		oglMouseButtons&=~(1<<button); /* Clear the bit for this button */
		myCtl->mouseUp(button,x,y);
	}
}

/** Default state for a new frame */
void oglSetFrameState(void) 
{

	/* lights and materials */
	oglDirectionLight(GL_LIGHT0, oglVector3d(0.0, 0.0, 1.0), 
		oglVector3d(1.0, 1.0, 0.7), 0.1,0.8,0.0);
	oglDirectionLight(GL_LIGHT1, oglVector3d(0.0, 0.0, -1.0), 
		oglVector3d(0.05, 0.1, 0.2), 0.4,0.3,0.0);

	GLfloat ones[] = {1.0f, 1.0f, 1.0f, 0.0f};
	GLfloat specularity[] = {50.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ones);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   ones);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  ones);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS,  specularity);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	/* glLightModel(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR); */
	oglEnable(myCfg.lighting, GL_LIGHTING);
	if (myCfg.lighting) {
		glEnable(GL_NORMALIZE);
	}

	/* Depth state */
	glDepthFunc(GL_LEQUAL);
	oglEnable(myCfg.depth, GL_DEPTH_TEST);
	
	glShadeModel (GL_SMOOTH);
	
	/* Alpha blending */
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (myCfg.alpha) {
		glEnable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0.01f);
	}
	
	/* Textures */
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	// oglWrapType(GL_REPEAT); //? 
	// oglTextureType(oglTexture_mipmap); // Screws up currently bound texture-- bad idea...
}

/**
 Perform all OpenGL-related start-frame work:
   -Clear the back buffer.
   -Initialize lighting and matrices.
*/
void oglStartFrame(const oglViewpoint &vp) {
/* Clear buffers */
	int clearMode=0;
	if (myCfg.clear) clearMode|=GL_COLOR_BUFFER_BIT;
	if (myCfg.depth) clearMode|=GL_DEPTH_BUFFER_BIT;
	if (myCfg.stencil) clearMode|=GL_STENCIL_BUFFER_BIT;
	glClearColor(myCfg.clearColor.x,myCfg.clearColor.y,myCfg.clearColor.z,0);
	if (clearMode!=0) glClear(clearMode);

/* Set up the projection and model matrices */
	oglSetViewpoint(vp);
	glLoadIdentity();

/* Set up OpenGL state */
	oglSetFrameState();

#ifdef GL_MULTISAMPLE_ARB
	glEnable(GL_MULTISAMPLE_ARB);
#endif
	oglCheck("after frame setup");
}

void oglSetViewpoint(const oglViewpoint &vp) {
	glViewport(0, 0, (GLsizei) vp.getWidth(), (GLsizei) vp.getHeight());
	glMatrixMode(GL_PROJECTION);
	double mat[16],z_near=1.0e-3,z_far=1.0e5;
	if (myCtl) myCtl->getZRange(z_near,z_far);
	vp.makeOpenGL(mat,z_near,z_far);
	glLoadMatrixd(mat);
	
	glMatrixMode(GL_MODELVIEW);
}

/**
 Perform all OpenGL-related end-frame work:
   -Swap the buffers
*/
void oglEndFrame(void) {
	
	
	glutSwapBuffers();
}

/* Object to read back depth and stencil buffers */
static oglReadback rb;

MYCALLBACK(void) ogl_main_display(void)
{
	glFinish();
	double startFrame=oglTimer();
	const oglViewpoint &vp=myCtl->getViewpoint(myCfg.width,myCfg.height);
	oglStartFrame(vp);
	myScene->draw(vp);
	oglCheck("after scene display");

	glFinish();
	double newTime=oglTimer()-startFrame;
	double compareTime=(newTime>avgTime)?newTime:avgTime;
	/* Set it up so we average the times of previous frames with
	  a decay half-life of about this long: */
	double decayTime=0.01; /* seconds */
	double oldWeight;
	double maxWeight=0.99;
	if (compareTime<=0) 
	{ /* Current time reading is bogus--stick with old value */
		oldWeight=maxWeight;
	} else { /* Normal case: current time reading is reasonable */
		oldWeight=0.5*(decayTime/compareTime); /* t==decayTime -> 0.5 weight */
		if (!(oldWeight>0)) oldWeight=0;
		if (!(oldWeight<maxWeight)) oldWeight=maxWeight;
	}
	// printf (" old= %f   new= %f   wgt= %f\n",avgTime,newTime,oldWeight);
	avgTime=oldWeight*avgTime + (1.0-oldWeight)*newTime;
	oglLastFrameTime=newTime;
	double frameTimeCap=0.1; /* to prevent freakouts at low framerate */
	if (oglLastFrameTime>frameTimeCap) oglLastFrameTime=frameTimeCap;
	
	if (myCfg.framerate) {
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glLoadIdentity();
		oglPixelMatrixSentry pixelMode;
		glColor3f(1.0f,1.0f,1.0f);
		oglPrintf(oglVector3d(-0.98,-0.98,0),"%.2f ms/frame; %.2f fps",1.0e3*avgTime,1.0/avgTime);
	}
	
	if (myCfg.depth && oglKey('D',"Show Z-buffer onscreen.  Dark is close; white is far")) {
		rb.readDepth(0,0,vp.getWidth(),vp.getHeight());
		rb.show();
	}
	if (myCfg.stencil && oglKey('S',"Show stencil buffer onscreen.  Black is zero")) {
		rb.readStencil(0,0,vp.getWidth(),vp.getHeight());
		rb.show();
	}
	
	oglEndFrame();
}

	template <class T>
	inline T my_max(T a,T b) { if (a>b) return a; else return b; }
	template <class T>
	inline T my_min(T a,T b) { if (a<b) return a; else return b; }
	
/**
 Render a screenshot with w x h output pixels, antialized by a factor
 of aaZoom (1 for no antialiasing), and write it to the PPM
 image file named outPPM.
 
 Coordinate systems used: 
    output pixels: w x h pixels, factor of aaZoom bigger than fullres pixels
    fullres pixels: fw x fh pixels, before antialiasing compression
    grab pixels: gw x gh pixels, in small windows of fullres pixels
*/
void oglScreenshot(int w,int h,int aaZoom,const char *outPPM)
{
	int fw=w*aaZoom, fh=h*aaZoom; /* full-res (pre-AA shrink) size */
	/* Full-resolution viewport we'll be rendering chunks of. */
	oglViewpoint fullres=myCtl->getViewpoint(fw,fh);
	
	int gw=1024, gh=1024; /* size of chunks of viewport to grab */
	while (gw>myCfg.width) gw/=2;
	while (gh>myCfg.height) gh/=2;
	printf("Rendering %d x %d (pre-AA) snapshot in pieces of size %d x %d \n",
		fw,fh, gw,gh);
	
	int tw=gw/aaZoom, th=gh/aaZoom; /* output rendering tile */
	typedef unsigned char byte;
	byte *out=new byte[3*w*h]; /* antialiased output pixels */
	/* Loop over output tiles */
	for (int oy=0;oy<h;oy+=th)
	for (int ox=0;ox<w;ox+=tw) 
	{ /* Render the output section starting at (ox,oy) */
		printf("Rendering output section starting at (%d,%d) \n",ox,oy);
		oglViewpoint window(fullres.getEye(),
			fullres.viewplane(osl::Vector2d(ox*aaZoom,oy*aaZoom)),
			fullres.getX(),fullres.getY(),gw,gh);
		oglStartFrame(window);
		myScene->draw(window);
		rb.readColor(0,0,gw,gh);
		const byte *pix=(const byte *)rb.getData();
		int endy=my_min(oy+th,h);
		int endx=my_min(ox+tw,w);
		/* Loop over output antialiased pixels for this tile */
		for (int y=oy;y<endy;y++) 
		for (int x=ox;x<endx;x++) {
			int px=aaZoom*(x-ox),py=aaZoom*(y-oy);
			unsigned int r=0,g=0,b=0,a=0; /* pixel value accumulators */
			for (int dy=0;dy<aaZoom;dy++)
			for (int dx=0;dx<aaZoom;dx++) {
				int i=((py+dy)*gw+(px+dx))*4;
				r+=pix[i+0];
				g+=pix[i+1];
				b+=pix[i+2];
				a+=pix[i+3];
			}
			if (aaZoom!=1) {
				r/=(aaZoom*aaZoom);
				g/=(aaZoom*aaZoom);
				b/=(aaZoom*aaZoom);
				a/=(aaZoom*aaZoom);
			}
			int i=(y*w+x)*3;
			out[i+0]=r;
			out[i+1]=g;
			out[i+2]=b;
		}
		oglEndFrame(); /* swap buffers, so we can see what's goin' on */
	}
	
	/* Create output image */
	FILE *f=fopen(outPPM,"wb");
	if (f==NULL) {printf("Can't create file '%s'!\n",outPPM);return;}
	fprintf(f,"P6\n%d %d\n255\n",w,h);
	for (int y=0;y<h;y++) { /* flip image bottom-to-top on output */
		fwrite(&out[3*(h-1-y)*w],1,3*w,f);
	}
	fclose(f);
	delete[] out;
	printf("Wrote %d x %d pixel %d-way antialiased image to '%s'\n",w,h,aaZoom,outPPM);
}

MYCALLBACK(void) ogl_main_reshape(int w, int h)
{
	myCfg.width=w; myCfg.height=h;
	
	myCtl->reshape(w,h);
}

#ifdef __unix__
#include <unistd.h> /* for select */
void give_up_timeslice(void) {
	/* note: sleep(0) doesn't work here, because GLUT uses SIGALRM */
	struct timeval tv;
	tv.tv_sec=0; tv.tv_usec=1000; /* wait up to 1ms */
	select(0,0,0,0,&tv);
}
#else /* Windows */
#include <windows.h>
void give_up_timeslice(void) {
	Sleep(0);
}
#endif

MYCALLBACK(void) ogl_main_idleFn(void)
{
	if (oglAsyncRepaintRequest) 
	{ /* Do a repaint */
		if (oglAsyncRepaintRequest==1)
			oglAsyncRepaintRequest=0;
		oglRepaint(0);
		return;
	}
	
	/* Give up rest of timeslice.  This is important!
	  Without this call, the program uses 100% CPU! */
	give_up_timeslice();
}

MYCALLBACK(void) ogl_main_keyboard(unsigned char key, int x, int y)
{
	// flip toggles on keypress
	oglToggles[key]=!oglToggles[key];
	oglKeyMap[key]=1;
	
	myCtl->handleEvent((int)key);
}
MYCALLBACK(void) ogl_main_keyboard_up(unsigned char key, int x, int y)
{
	oglKeyMap[key]=0;
}
MYCALLBACK(void) ogl_main_special(int key, int x, int y)
{
	if (key<0x80)
		oglKeyMap[0x80+key]=1;
	myCtl->handleEvent(0x80+key);
}
MYCALLBACK(void) ogl_main_special_up(int key, int x, int y)
{
	if (key<0x80)
		oglKeyMap[0x80+key]=0;
}

static int main_window=0;
int oglGetMainWindow(void) {
	if (main_window==0) printf("WARNING: Calling oglGetMainWindow before main window created!\n");
	return main_window;
}
void oglSetup(const oglOptions &init, int *argc,char **argv) {
	glutInitWindowSize (init.width,init.height);

/* Parse command line */
	int fake_argc=1;
	char *fake_argv[2]={(char *)"foo",0};
	if (argc==0) {argc=&fake_argc;argv=fake_argv;}
	glutInit(argc, argv);
	myCfg=init;

/* Create main window */
	int mode=GLUT_DOUBLE | GLUT_RGBA;
	if (myCfg.stencil) mode|=GLUT_STENCIL;
	if (myCfg.depth) mode|=GLUT_DEPTH;
	mode|=GLUT_MULTISAMPLE;
	glutInitDisplayMode (mode);
	bool createWindow=true;
	const char *gameMode=getenv("GLUT_GAME_MODE");
	if (myCfg.fullscreen || gameMode) {
		char modeStr[100];
		sprintf(modeStr,"%dx%d:%d",myCfg.width,myCfg.height,32);
		glutGameModeString(gameMode?gameMode:modeStr);
		if (glutEnterGameMode()!=0) /* Game mode worked-- don't open window */
			createWindow=false;
	} 
	if (createWindow) {
		main_window=glutCreateWindow (myCfg.name);
	}
	glutMouseFunc(ogl_main_mouseFn);
	glutMotionFunc(ogl_main_motionFn);
	glutReshapeFunc (ogl_main_reshape);
	glutIdleFunc (ogl_main_idleFn);
	glutKeyboardFunc (ogl_main_keyboard);
	glutKeyboardUpFunc (ogl_main_keyboard_up); /* "up" version for KeyMap */
	glutSpecialFunc (ogl_main_special); /* for arrow keys */
	glutSpecialUpFunc (ogl_main_special_up);
	glutDisplayFunc (ogl_main_display);

	oglCheck("after init");
}

static int exit_is_happy=0;
#ifdef _WIN32 /* Prevent the DOS window from vanishing along with error message on exit... */
#include <conio.h>
void windows_exit_getch(void) {
	if (exit_is_happy) return;
	fflush(stdout); fflush(stderr);
	fprintf(stdout,"Press any key to exit\n");
	_getch();
}
#endif

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int oglEventloop(oglController *ctl,oglDrawable *scene)
{
#ifdef _WIN32
	atexit(windows_exit_getch);
#endif

	myCtl=ctl;
	myScene=scene;

	myCtl->reshape(myCfg.width,myCfg.height);
	glutMainLoop();
	return 0;
}

/* Normal happy exit routine */
void oglExit(void) {
	exit_is_happy=1;
	/* delete myCtl; // Breaks if myScene and myCtl are same object... */
	delete myScene;
	myCtl=0;
	myScene=0;
	exit(0);
}
