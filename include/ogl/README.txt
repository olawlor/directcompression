Orion's OpenGL Utility Routines (ogl)

Contents (by order of decreasing utility):
   ogl/util: Simple self-contained OpenGL utility 
      routines for various annoying tasks.
   
   ogl/fast_mipmaps: Faster version of gluBuildMipmaps.
   
   ogl/ext: Wrapper around the excellent GLEW library 
      for OpenGL Extensions.  (GLEW sources are in
      ogl/glew, glxew, and wglew.)
   
   ogl/main: GLUT interface and wrapper routines.
   
   ogl/liltex: Dynamically combines many small textures 
      into bigger ones.


The dependencies are:
  ext -> glew
  main -> util
  util -> vector3d
  viewpoint -> vector3d



---------------------
Portability to Mac OS X:

"uname" returns "Darwin" on OS X.

http://www.stevestreeting.com/?p=599
	Attribute aliasing bugs.
