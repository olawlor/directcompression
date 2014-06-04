/**
  Load GLSL raytracer, and draw proxy geometry to run the raytracer.
  
  Dr. Orion Sky Lawlor, olawlor@acm.org, 2012-01-30 (Public Domain)
*/
#include "physics/world.h" /* physics::library and physics::object */
#include "ogl/glsl.h"

/* Just include library bodies here, for easy linking */
#include "physics/world.cpp" 
#include "physics/config.cpp"
#include "ogl/glsl.cpp"


/* A raytraced object */
class rayObject : public physics::object {
public:
	rayObject(void) 
		:physics::object(0.01)  /* <- our timestep, in seconds */
	{ }
	
	void simulate(physics::library &lib) { }
	
	void draw(physics::library &lib) {
		/* Load up shader */
		static programFromFiles prog; 
		prog.load("vertex.txt","fragment.txt");
		glUseProgramObjectARB(prog);
		unsigned int ul=glGetUniformLocationARB(prog,"camera");
		glUniform3fvARB(ul,1,camera);
		ul=glGetUniformLocationARB(prog,"time");
		static double time=0.0;
		time+=lib.dt;
		glUniform1fARB(ul,time);

		/* Draw raytracer proxy geometry *HUGE* (to cover everything) */
		glColor4f(1.0f,1.0f,1.0f,0.4f);
		glutSolidSphere(500.0,4,8);
		
		glUseProgramObjectARB(0);
	}
};


/* Called to create a new simulation */
void physics_setup(physics::library &lib) {
	lib.world->add(new rayObject());
	lib.background[0]=lib.background[1]=0.6;
	lib.background[2]=1.0; // light blue
}

/* Called every frame to draw everything */
void physics_draw_frame(physics::library &lib) {
	/* Point and line smoothing needs usual alpha blending */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	lib.world->draw(lib);
}


