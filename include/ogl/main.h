/*
Interface between a GLUT event loop and the rest of the program.

Orion Sky Lawlor, olawlor@acm.org, 8/29/2002
*/

#ifndef __OSL_GLUTMAIN_H
#define __OSL_GLUTMAIN_H

#include "ogl/util.h"
#include "ogl/event.h"
#include "osl/viewpoint.h"
typedef osl::Viewpoint oglViewpoint;
typedef osl::Axes3d oglAxes3d;

/**
 Return the time (like oglTimer) taken by the last frame, in seconds.
 This is a decent estimate of the time to be taken by the current frame,
 for interaction purposes.
*/
extern double oglLastFrameTime;

/**
 Render a screenshot with w x h output pixels, antialized by a factor
 of aaZoom (1 for no antialiasing), and write it to the PPM
 image file named outPPM.
*/
void oglScreenshot(int w,int h,int aaZoom,const char *outPPM);

/**
  Stores pixel coordinates of last known mouse location,
  and bit mask of pressed mouse buttons.
*/
extern int oglMouseX,oglMouseY,oglMouseButtons;


/**
  A drawable scene-- one virtual "draw" method.
*/
class oglDrawable {
public:
	virtual ~oglDrawable();

	/**
	 Render into the current OpenGL buffer using 
	 this viewpoint, which OpenGL is already set up 
	 to accept.

	 This means making any glClear calls and then
	 drawing gl primitives.
	*/
	virtual void draw(const oglViewpoint &v) =0;
};
typedef oglDrawable GLdrawable;

/*
  Accept GUI events from the user.
*/
class oglEventConsumer {
public:
	/** Deletion indicates the GUI is closing down. */
	virtual ~oglEventConsumer();

	/** The screen is now w x h pixels. */
	virtual void reshape(int wid,int ht);
	
	/** No buttons are down, but the mouse is moving: */
	virtual void mouseHover(int x,int y);
	/** This button is going down: button 0 is the left button */
	virtual void mouseDown(int button,int x,int y);
	/** This set of buttons are moving: buttonMask&(1<<b) is button b */
	virtual void mouseDrag(int buttonMask,int x,int y);
	/** This button is going up: button 0 is the left button. */
	virtual void mouseUp(int button,int x,int y);
	
	/** A key has been hit or a menu has been selected */
	virtual void handleEvent(int uiCode);
};

/**
  Controls a viewpoint-- converts user events to a viewpoint.
*/
class oglController : public oglEventConsumer {
public:
	/**
	 Extract a read-only copy of the current viewpoint.
     This routine need not make any GL calls.
	*/
	virtual const oglViewpoint &getViewpoint(int width,int height) =0;
	/**
	 Return the near and far depths to keep in the z-buffer.
	*/
	virtual void getZRange(double &z_near,double &z_far);
};

/**
  Implements a trackball-- 
 */
class oglTrackballController : public oglController {
public:
	oglVector3d rotcen; ///< Center of rotation/scaling.
	oglAxes3d axes; ///< Current drawing axes.
	double eyeDist; ///< Distance from center to eye.
	double fov; ///< Horizontal field-of-view (degrees)
	inline oglVector3d getEye(void) {
		return axes.getZ()*eyeDist+rotcen;
	}

	///< Mouse state during a drag
	enum {stateNone, stateRotate, stateZoom, stateTwirl} state;
	int lastX,lastY; ///< Mouse location.
	
	oglViewpoint viewpoint; ///< Last drawn viewpoint
	
	oglTrackballController(double eyeDist_=3.0,double fov_=90.0,
			oglVector3d rotcen_=oglVector3d(0,0,0));

	virtual void mouseDown(int button,int x,int y);
	virtual void mouseDrag(int buttonMask,int x,int y);
	virtual void handleEvent(int uiCode);
	virtual const oglViewpoint &getViewpoint(int width,int height);
	virtual void getZRange(double &z_near,double &z_far);
};

/**
  Startup parameters for OpenGL.
*/
class oglOptions {
public:
	/// name is a string name to display in the window title. 
	const char *name; 

	/// fullscreen: if true, try to display fullscreen; if false, display in a window.
	bool fullscreen;

	/// If true (default), use a depth buffer and depth test.
	bool depth;

	/// Use this much dynamic range in z-buffer--default 100.
	///   More dynamic range means less precision.
	double depthRange;

	/// If true (default), turn on OpenGL lights and lighting.
	bool lighting;

	/// If true (default), enable premultiplied alpha blending.
	bool alpha;

	/// If true (default), clear the screen color before drawing.
	bool clear;
	/// Clear color, expressed as a vector (default is black).
	oglVector3d clearColor;

	/// Recommended resolution of display, in pixels.
	int width, height;

	/// If true, display framerate (in ms) onscreen.
	bool framerate;

	/// If true, allocate and clear the stencil buffer.
	bool stencil;

	oglOptions(const char *name_="OpenGL Demo") :name(name_) {
		fullscreen=false;
		depth=true;
		depthRange=100;
		lighting=true;
		alpha=true;
		clear=true;
		clearColor=oglVector3d(0,0,0);
		width=1000; height=700;
		framerate=true;
		stencil=false;
	}
};

int oglGetMainWindow(void);

/**
 Create a window and set up OpenGL.  
 Must be made before any other OpenGL calls.
*/
void oglSetup(const oglOptions &init, int *argc=0,char **argv=0);

/**
 Run the gui until the user exits the program.
*/
int oglEventloop(oglController *ctl,oglDrawable *draw);

/**
 Shut down the program.  Never returns.
*/
void oglExit(void);

/**
 Perform all OpenGL-related start-frame work:
   -Clear the back buffer.
   -Initialize matrices and enables.
*/
void oglStartFrame(const oglViewpoint &vp);

/**
 Set OpenGL projection matrix and viewport to this oglViewpoint.
*/
void oglSetViewpoint(const oglViewpoint &vp);

/**
 Perform all OpenGL-related end-frame work:
   -Swap the buffers
*/
void oglEndFrame(void);


#endif /*def(thisHeader)*/
