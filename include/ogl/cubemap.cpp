/*
  Reads cubemap images into OpenGL texture.
  Requires OSL graphics2d libraries.
*/

#include "GL/glew.h"
#include "osl/graphics.h"
#include "ogl/cubemap.h"
#include <string>

oglTextureCubemap::oglTextureCubemap(const char *srcDir,
		oglTexture_t type)
{
	enum {nSide=6};
	struct side_t {
		GLenum target;
		const char *fName;
	};
	const static side_t side[nSide]={
		{GL_TEXTURE_CUBE_MAP_POSITIVE_X, "Xp"},
		{GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "Xm"},
		{GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "Yp"},
		{GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "Ym"},
		{GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "Zp"},
		{GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "Zm"},
	};
	bind();
	
	for (int sn=0;sn<nSide;sn++) 
	{
		const static char *fmts[]={".jpg",".png",0};
		for (const char **f=&fmts[0];*f!=0;f++) 
		{
		  try {
			std::string s=srcDir;
			s+="/"; s+=side[sn].fName; s+=*f;
			osl::graphics2d::RgbaRaster r(s.c_str());
			oglTextureFormat_t t=oglTextureFormat(&r.at(0,0));
			oglUploadTexture2D(
				(const GLubyte *)&r.at(0,0),r.wid,r.ht,type,
				t.format,t.type, GL_RGBA8,GL_CLAMP_TO_EDGE,
				side[sn].target);
			break;
		  } catch (osl::Exception *e) {
			/* ignore missing-file read errors */
		  }
		}
	}
	oglTextureType(oglTexture_linear,GL_TEXTURE_CUBE_MAP); 
	oglTexWrap(GL_CLAMP_TO_EDGE,GL_TEXTURE_CUBE_MAP); 
	oglCheck("cubemap upload");
}

