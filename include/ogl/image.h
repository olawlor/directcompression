#ifndef MISC_IMAGE_H
#define MISC_IMAGE_H

/*
Horribly hacked by Dr. Orion Lawlor, olawlor@acm.org, 2008-02-14
This file a horribly hacked version of:

  GLT OpenGL C++ Toolkit (LGPL)
  Copyright (C) 2000-2002 Nigel Stewart  

  Email: nigels@nigels.com   
  WWW:   http://www.nigels.com/glt/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*! \file 
	\brief   Image utility routines 
	\author  Nigel Stewart, RMIT (nigels@nigels.com)
	\ingroup Misc
	\todo    Image test program(s)

    These routines are not intended as a general-purpose
	image viewing, output or image-processing package.
	The priority here is to support image formats that
	are most likely to be used in the context of OpenGL
	texture mapping.

    Here are a few notes about the relative advantages
	of each type of image encoding.

    PPM is simple to read and write - greyscale and
	RGB variants are useful for input and output,
	commonly used on the UNIX platform.  PPM's are
	not compressed, and are not lossy.

	TGA come in uncompressed or compressed variants,
	and support indexed, greyscale, or 16/24/32 bit
	RGB and RGBA pixel data.  TGA's support an
	alpha (transparency) channel.

	BMP is the standard image format for the Windows
	platform.  The structure of BMP files is
	relatively convoluted, but similar to TGA.
*/

#include <string>
#include <iosfwd>
#include <vector>
#include <fstream>

//
// Image Processing
//

/*!
	\brief		Flip image vertically
	\ingroup	Misc
	\param		dest	 Destination buffer
	\param		src      Source buffer
	\param      width    Width of image  (pixels)
	\param		height   Height of image (pixels)
*/
void flipImage(std::string &dest,const std::string &src,const int width,const int height);

//
// Image decoding
//

/*!
	\brief		Decode image data from PPM, BMP, TGA and PNG
	\ingroup	Misc
	\param		width	 Image width
	\param		height	 Image height
	\param		image	 Raw pixel image data in width x height x 3 RGB array
	\param		data	 Source data buffer
	\param		return   Bytes per pixel in output image: 0=read error, 1=greyscale, 2=greyscale+alpha, 3=RGB, 4=RGBA.
*/
int decode(int &width,int &height,std::string &image,const std::string &data);

/*!
	\brief		Decode image data from PPM
	\ingroup	Misc
	\param		type	 Type of PPM, P5 for PGM, P6 for PPM
	\param		width	 Width of PPM
	\param		height	 Height of PPM
	\param		image	 Destination RGB or Greyscale buffer
	\param		data	 Source data buffer
	\todo		Only P6 and P5 variants (binary RGB and Grey) recognised
	\todo       Implement text PPM support
*/
int decodePPM(std::string &type,int &width,int &height,std::string &image,const std::string &data);

/*!
	\brief		Decode image data from BMP
	\ingroup	Misc
	\param		width	 Width of BMP
	\param		height	 Height of BMP
	\param		image	 Destination RGB buffer
	\param		data	 Source data buffer
	\todo		Windows BMP import limited to RGB images
*/
int decodeBMP(int &width,int &height,std::string &image,const std::string &data);

/*!
	\brief		Decode image data from TGA
	\ingroup	Misc
	\param		width	 Width of BMP
	\param		height	 Height of BMP
	\param		image	 Destination RGB buffer
	\param		data	 Source data buffer
	\todo		TGA import limited to uncompressed RGB or RGBA images
*/
int decodeTGA(int &width,int &height,std::string &image,const std::string &data);

#ifdef GLT_PNG

/*!
	\brief		Decode image data from PNG
	\ingroup	Misc
	\param		width	 Width of PNG image
	\param		height	 Height of PNG image
	\param		image	 Destination RGB buffer
	\param		data	 Source data buffer
*/
int decodePNG(int &width,int &height,std::string &image,const std::string &data);

#endif

//
// Image Encoding
//

/*!
	\brief		Encode image data as RGB unix PPM or PGM
	\ingroup	Misc
	\param		data	 PPM output buffer
	\param		width	 Image width
	\param		height	 Image height
	\param		image	 Input image buffer
*/
bool encodePPM(std::string &data,const int width,const int height,const std::string &image);

/*!
	\brief		Encode image data as RGB TGA
	\ingroup	Misc
	\param		data	 TGA output buffer
	\param		width	 Image width
	\param		height	 Image height
	\param		image	 Input image buffer
	\param		with_alpha  false: RGB; true: RGBA (alpha channel export)
*/
bool encodeTGA(std::string &data,const int width,const int height,bool with_alpha,const std::string &image);

#ifdef GLT_PNG

/*!
	\brief		Encode image data as RGB PNG
	\ingroup	Misc
	\param		data	 PNG output buffer
	\param		width	 Image width
	\param		height	 Image height
	\param		image	 Input image buffer
*/
bool encodePNG(std::string &data,const int &width,const int &height,const std::string &image);

#endif

/** Bottom line: read this texture image from disk, and upload to OpenGL */
bool readTextureFromFile(const char *fName,GLenum internalFormat=GL_RGBA8);

#endif
