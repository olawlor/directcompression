/*
Camera and 4x4 projection matrix:

Orion Sky Lawlor, olawlor@acm.org, 2003/2/21
*/
#include "osl/osl.h" /* for "bad" */
#include "osl/camera.h"

using osl::Vector3d;

inline bool orthogonal(const Vector3d &a,const Vector3d &b) {
	return fabs(a.dot(b))<1.0e-4;
}

/// Fill our projection matrix m with values from E, R, X, Y, Z
///   Assert: X, Y, and Z are orthogonal
void osl::graphics3d::Camera::buildM(void) {
	// if (!(orthogonal(X,Y) && orthogonal(Y,Z) && orthogonal(X,Z)))
	//	osl::bad("Camera axes are non-orthogonal");
	
	/*
	  The projection matrix derivation begins by postulating
	  a universe point P, which we need to project into the view plane.
	  The view plane projection, S, satisfies S=E+t*(P-E) for 
	  some parameter value t, and also Z.dot(S-R)=0.
	  Solving this and taking screen_x=sX.dot(S-R), screen_y=sY.dot(S-R),
	  and screen_z=Z.dot(R-E)/Z.dot(P-E) leads to our matrix.
	 */
	// Scale X and Y so screen pixels==sX.dot(S)
	Vector3d sX=X/X.magSqr();
	Vector3d sY=Y/Y.magSqr();
	
	// Compute skew factors and skewed axes
	double skew_x=sX.dot(R-E), skew_y=sY.dot(R-E), skew_z=Z.dot(R-E);
	Vector3d gX=-skew_x*Z+skew_z*sX;
	Vector3d gY=-skew_y*Z+skew_z*sY;
	
	// Assign values to the matrix
	m(0,0)=gX.x; m(0,1)=gX.y; m(0,2)=gX.z; m(0,3)=-gX.dot(E); 
	m(1,0)=gY.x; m(1,1)=gY.y; m(1,2)=gY.z; m(1,3)=-gY.dot(E); 
	m(2,0)=0;    m(2,1)=0;    m(2,2)=0;    m(2,3)=skew_z;
	m(3,0)=Z.x;  m(3,1)=Z.y;  m(3,2)=Z.z;  m(3,3)=-Z.dot(E); 
}

/// Build a camera at eye point E pointing toward R, with up vector Y.
///   The up vector is not allowed to be parallel to E-R.
osl::graphics3d::Camera::Camera(const Vector3d &E_,const Vector3d &R_,Vector3d Y_)
	:E(E_), R(R_), Y(Y_), wid(-1), ht(-1)
{
	Z=(E-R).dir(); //Make view plane orthogonal to eye-origin line
	X=Y.cross(Z).dir();
	Y=Z.cross(X).dir();
	// assert: X, Y, and Z are orthogonal
	buildM();
}

/// Make this camera, fresh-built with the above constructor, have
///  this X and Y resolution and horizontal full field-of-view (degrees).
/// This routine rescales X and Y to have the appropriate length for
///  the field of view, and shifts the projection origin by (-wid/2,-ht/2). 
void osl::graphics3d::Camera::discretize(int w,int h,double hFOV) {
	wid=w; ht=h;
	double pixSize=E.dist(R)*tan(0.5*(M_PI/180.0)*hFOV)*2.0/w;
	X*=pixSize;
	Y*=pixSize;
	R-=X*(0.5*w)+Y*(0.5*h);
	buildM();
}

/// Like discretize, but flips the Y axis (for typical raster viewing)
void osl::graphics3d::Camera::discretizeFlip(int w,int h,double hFOV) {
	discretize(w,h,hFOV);
	R+=Y*h;
	Y*=-1;
	buildM();
}


/// Build a camera at eye point E for view plane with origin R
///  and X and Y as pixel sizes.  Note X and Y must be orthogonal.
osl::graphics3d::Camera::Camera(const Vector3d &E_,const Vector3d &R_,
	const Vector3d &X_,const Vector3d &Y_,int w,int h)
	:E(E_), R(R_), X(X_), Y(Y_), wid(w), ht(h)
{
	//if (!orthogonal(X,Y))
	//	osl::bad("Non-orthogonal X and Y passed to Camera::Camera");
	Z=X.cross(Y).dir();
	buildM();
}
