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

#include "ogl/framebuffer.h"
#include "ogl/framebuffer.cpp"
#include "ogl/util.cpp"
#include "ogl/fast_mipmaps.c"


const float km=1.0/6371.0; // convert kilometers to render units (planet radii)
int benchmode=0, dumpmode=0, errormetric=23;
float bench_target=0.0;
double interval_time=1.0; // seconds to show each image

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
	if (srcTex==0) { printf(" Failed to load image."); exit(1); }
	printf(".\n");
	glBindTexture(GL_TEXTURE_2D,srcTex);
}


const char *source_image="../real/ocean.jpg";


#include <vector>

	static int framecount=0, last_framecount=0;
	static float last_render=0.0, last_error1=0.0, last_error2=0.0;


/**
 Renders an image in steps, from coarse to fine resolution.
 
 FIXME: need to decouple and parameterize several things here
 	- Multigrid level count
 	- Estimation scheme
 	- Interpolation scheme
 	- Sampling backend (passed in from user)
*/
class multigrid_renderer {
public:
	int wid,ht; // size of full resolution image
	enum {msaa=0}; // levels of multisample antialiasing: 4^msaa samples per pixel.
	enum {levels=
#include "sweep_levels.h"
+msaa}; // multigrid levels are from 0..levels-1.  level==msaa is the full resolution image
	oglFramebuffer *fb[levels];
	
	multigrid_renderer(int wid_,int ht_) 
	{
		wid=wid_; ht=ht_;
		for (int l=0;l<levels;l++) fb[l]=new oglFramebuffer(
			(wid<<msaa)>>l,(ht<<msaa)>>l,GL_RGBA8);
	}
	
	/** Draw a fullscreen quad (proxy geometry) */
	void screen_quad(float alphaCheck) { 
		glBegin (GL_QUAD_STRIP);
		glTexCoord2f(0,0); glVertex3d(-1.0,-1.0,0.0); 
		glTexCoord2f(1,0); glVertex3d(+1.0,-1.0,0.0); 
		glTexCoord2f(0,1); glVertex3d(-1.0,+1.0,0.0); 
		glTexCoord2f(1,1); glVertex3d(+1.0,+1.0,0.0); 
		glEnd();
		
		if (dumpmode==2 && framecount==0) {
			oglDumpScreen(); // debug
			oglDumpAlpha();
		}
		
		int w,h;
		int alphaTest=(int)ceil(alphaCheck*255.0)+1;
		std::vector<unsigned char> rgba=oglDumpScreen(w,h,4);
		for (int y=0;y<h;y++)
		for (int x=0;x<w;x++) 
		{
			int i=4*(x+y*w);
			int r=rgba[i+0];
			int g=rgba[i+1];
			int b=rgba[i+2];
			int a=rgba[i+3];
			if (alphaCheck==0.0) { // last render pass--compute error
				last_error1+=r+g+b;
				last_error2+=r*r+g*g+b*b;
			} else if (a<=alphaTest) { // we just rendered this pixel
				last_render++;
			}
		}
		printf("%.2f(%d,%d): %.0f	%.0f	%.0f\n", 
			alphaCheck,w,h,
			last_render,last_error1,last_error2);
	}
	
	// Convert a framebuffer (size) to a vec4 giving x,y pixel size, z,w 1.0/pixel size
	vec4 framebuffer2vec4(const oglFramebuffer *fbo) {
		return vec4(fbo->w,fbo->h,1.0/fbo->w,1.0/fbo->h);
	}
	
	/**
	 Loop over multigrid levels and do rendering.
	 FIXME: inputs & sampling part of shader should be parameterized
	*/
	void render(GLhandleARB prog) {
		glFastUniform1f(prog,"benchmode",(float)benchmode);
		glFastUniform1f(prog,"errormetric",(float)errormetric);
		
		last_render=0.0; last_error1=0.0; last_error2=0.0;
		glFastUniform1f(prog,"multigridCoarsest",1.0f);
		fb[levels-1]->bind();
		screen_quad(1.0f);
		for (int l=levels-2;l>=-1;l--) {
			float multigridCoarsest=(l+1)*1.0/(levels);
			glFastUniform1f(prog,"multigridCoarsest",multigridCoarsest);
			if (l==-1) fb[levels-1]->unbind(); // last step: render to screen
			else fb[l]->bind(); // intermediate step: render to framebuffer
			
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D,fb[l+1]->get_color());
	
			// Make sure texture filtering modes are correct
			GLenum target=GL_TEXTURE_2D;
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_LINEAR); // _MIPMAP_LINEAR);
			GLenum texWrap=GL_CLAMP_TO_EDGE;
			glTexParameteri(target, GL_TEXTURE_WRAP_S, texWrap); 
			glTexParameteri(target, GL_TEXTURE_WRAP_T, texWrap);
			
			glFastUniform1i(prog,"multigridCoarserTex",7);
			glFastUniform4fv(prog,"multigridCoarser", 1, framebuffer2vec4(fb[l+1]) );
			if (l>=0) glFastUniform4fv(prog,"multigridFiner", 1, framebuffer2vec4(fb[l]) );
			else glFastUniform4fv(prog,"multigridFiner", 1, framebuffer2vec4(fb[msaa]) );
			
			screen_quad(multigridCoarsest);
		}
		
		glBindTexture(GL_TEXTURE_2D,0); // clear texture state
		glActiveTexture(GL_TEXTURE0);
	}
};

multigrid_renderer *renderer=0;

void display(void) 
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE); 
	
	glClearColor(0.4,0.5,0.7,0.0); // background color
	glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
	
	glFinish();
	double start_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	
	
	int wid=glutGet(GLUT_WINDOW_WIDTH), ht=glutGet(GLUT_WINDOW_HEIGHT);
	if (!renderer || renderer->wid!=wid || renderer->ht!=ht) {
		delete renderer;
		renderer=new multigrid_renderer(wid,ht);
	}
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // flush any ancient matrices
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // flush any ancient matrices
	
	/* Build the pixel shader */
	static GLhandleARB prog=makeProgramObjectFromFiles(
		"interpolate_vtx.txt","interpolate.txt");
	glUseProgramObjectARB(prog);
	float pix=20.0;
	glFastUniform2fv(prog,"texdel",1,vec3(pix/wid,pix/ht,0.0));
	static float threshold=2.0;
	glFastUniform1f(prog,"threshold",threshold);
	
	
	static bool read_imgs=true; /* only read textures on the first frame */
/* Upload planet texture, to texture unit 1 */
	glActiveTexture(GL_TEXTURE1);
	if (read_imgs) read_soil_jpeg(source_image,GL_RGBA8);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,4);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glFastUniform1i(prog,"srctex",1); // texture unit number
	GLenum target=GL_TEXTURE_2D;
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER,GL_LINEAR); // _MIPMAP_LINEAR);
	GLenum texWrap=GL_CLAMP_TO_EDGE;
	glTexParameteri(target, GL_TEXTURE_WRAP_S, texWrap); 
	glTexParameteri(target, GL_TEXTURE_WRAP_T, texWrap);
	read_imgs=false;
	glActiveTexture(GL_TEXTURE0);
	
	
	renderer->render(prog);
	
	
	
	float pixelScale=1.0/(wid*ht);
	if (benchmode==2) {
		static double prev_err=-1.0, prev_thresh=-1.0;
		static double slopelimit=0.5, slopemul=1.0;
	
		// seeking a target render rate
		float render=last_render*pixelScale;
		float err=render-bench_target;
		
		static int leash=50;
		if (fabs(err)<0.001	|| leash--<=0) { // we hit it!
			printf("Target	%f	%f	%f	%f\n",
				threshold,
				last_render*pixelScale,
				last_error1*pixelScale/255.0,
				last_error2*pixelScale/(255.0*255.0)
			);
			// dump the final image
			benchmode=0; // real image, not error metric
			renderer->render(prog);
			oglDumpAlpha();
			oglDumpScreen();
			exit(0);
		}
		printf("Render=%f at threshold=%f (err %f)",render,threshold,err);
		
		float next_thresh=0.0;
		if (prev_thresh<0.0) { // first step: blind jump
			next_thresh=threshold+0.5*err;
			if (next_thresh<0) next_thresh=0.001;
			printf(" -> new threshold %f\n",next_thresh);
		} else { // subsequent step: secant method
			if (err*prev_err>0) 
			{ // stepping same direction-- go faster to get there
				slopelimit*=1.1;
				slopemul*=1.3;
			} 
			else 
			{ // stepping in opposite direction--avoid oscillations
				if (slopelimit>0.5) slopelimit=0.5;
				if (slopemul>1.0) slopemul=1.0;
				slopelimit*=0.7;
			}
		
			float slope=(threshold-prev_thresh)/(err-prev_err);
			if (slope>slopelimit) slope=slopelimit;
			if (slope<-slopelimit) slope=-slopelimit;
			next_thresh=threshold-err*slope;
			printf(" slope %f -> new threshold %f\n",
				slope,next_thresh);
		}
		prev_err=err;
		prev_thresh=threshold;
		threshold=next_thresh;
	}
	
	
	glUseProgramObjectARB(0);
	
	
	if (key_down['p']) {
		oglDumpScreen();
		key_down['p']=false; /* p is momentary print-screen; m is for movie */
	} 
	glutPostRedisplay(); // continual animation
	
	glutSwapBuffers();

/* Estimate framerate */
	static double last_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	glFinish();
	framecount++;
	double cur_time=0.001*glutGet(GLUT_ELAPSED_TIME);
	if (benchmode<2 && cur_time>interval_time+last_time) 
	{ // every half a second, estimate fps
		double time=cur_time-last_time;
		double time_per_frame=time/(framecount-last_framecount);
		time_per_frame=cur_time-start_time;
		if (benchmode==1) {
			static FILE *f=fopen("bench.txt","a");
			static int bcount=0;
			fprintf(f,"%d	%.6f	%.6f	%.6f	%.6f\n",
				bcount,threshold,
				last_render*pixelScale,
				last_error1*pixelScale/255.0,
				last_error2*pixelScale/(255.0*255.0)
				);
			fflush(f);
			if (bcount++>40) {
				fclose(f);
				int err=system("cat bench.txt");
				exit(err);
			}
			threshold=threshold*0.9;
		}
		else { /* not a benchmark, just an ordinary run */
			printf("Interpolator: %.1f fps, %.1f ms/frame %.2f ns/pixel threshold %.6f\n",
				1.0/time_per_frame,1.0e3*time_per_frame,
				1.0e9*time_per_frame/(wid*ht),threshold);
			fflush(stdout);
			oglDumpScreen();
			oglDumpAlpha();
			
			threshold=threshold*0.8;
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
		if (0==strcmp(argv[argi],"-bench")) { benchmode=1; interval_time=0.01; }
		else if (0==strcmp(argv[argi],"-target")) { benchmode=2; bench_target=atof(argv[++argi]); }
		else if (0==strcmp(argv[argi],"-metric")) { errormetric=atoi(argv[++argi]); }
		else if (0==strcmp(argv[argi],"-img")) { source_image=argv[++argi]; }
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
