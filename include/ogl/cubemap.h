/*
Cubemap loading and binding.

Orion Sky Lawlor, olawlor@acm.org, 8/13/2002 (Public Domain)
*/
#ifndef __OSL_CUBEMAP_H
#define __OSL_CUBEMAP_H

#include "ogl/util.h"

#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif

class oglTextureCubemap : public oglTextureName {
public:
	oglTextureCubemap(const char *srcDir,
		oglTexture_t type=oglTexture_mipmap);
	void bind(void) const {oglTextureName::bind(GL_TEXTURE_CUBE_MAP);}
};


#endif /*def(thisHeader)*/
