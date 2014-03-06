#include "image.h"

/*
Horribly hacked by Dr. Orion Lawlor, olawlor@acm.org, 2008-02-14
Began life as part of Nigel Stewart's GLT library.
 \file 
	\brief   Image utility routines 
	\ingroup Misc
*/
#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <set>
#include <map>
#include <list>
#include <iostream>
using namespace std;

typedef unsigned char byte;

int little32(void *data) {
	byte *d=(byte *)data;
	return d[0]|(d[1]<<8)|(d[2]<<16)|(d[3]<<24);
}

short little16(void *data) {
	byte *d=(byte *)data;
	return d[0]|(d[1]<<8);
}


void flipImage(string &dest,const string &src,const int width,const int height)
{
	const int lineSize = src.size()/height;

	assert(lineSize>0);
	assert(src.size()%lineSize==0);

	if (lineSize==0 || src.size()%lineSize!=0)
		return;

	dest.resize(src.size());

	const char *i = src.c_str();
	const char *j = dest.c_str()+dest.size()-lineSize;

	for (int k=0; k<height; i+=lineSize,j-=lineSize,k++)
		memcpy((void *)j,i,lineSize);
}

//
// Image decoding
//

int decode(int &width,int &height,std::string &image,const std::string &data)
{
	string type;
	int ret=0;

	// PPM

	if (0!=(ret=decodePPM(type,width,height,image,data)))
		return ret;
	
	// BMP

	if (0!=(ret=decodeBMP(width,height,image,data)))
		return ret;

	// PNG

	#ifdef GLT_PNG
	if (0!=(ret=decodePNG(width,height,image,data)))
		return ret;
	#endif

	// TGA

	return decodeTGA(width,height,image,data);
}

int decodePPM(string &type,int &width,int &height,string &image,const string &data)
{
	if (data.size()<1 || data[0]!='P')
		return 0;

	//

	size_t channels = 0;

	if (data[1]=='6')	// 24-bit binary RGB PPM file
	{
		channels = 3;
		type = "P6";
	}
	else
		if (data[1]=='5')	// 8-bit binary greyscale PGM file 
		{
			channels = 1;
			type = "P5";
		}
		else
			return 0;

	//

	const string eol("\n");
	const string digits("0123456789");

	size_t i = 0;
	
	int depth = 0;

	for (;;)
	{
		// Find end-of-line

		i = data.find_first_of(eol,i);
		if (i==string::npos)
			break;
		else
			i++;

		// If this line is a comment, try next line

		if (data[i]=='#')
			continue;
		
		// Get width

		width = atoi(data.c_str()+i);
		i = data.find_first_not_of(digits,i); if (i==string::npos) break;
		i = data.find_first_of(digits,i);     if (i==string::npos) break;

		// Get height

		height = atoi(data.c_str()+i);
		i = data.find_first_not_of(digits,i); if (i==string::npos) break;
		i = data.find_first_of(digits,i);     if (i==string::npos) break;

		// Get depth

		depth = atoi(data.c_str()+i);
		i = data.find(eol,i); 
		if (i!=string::npos) 
			i++;

		break;
	}

	// Check that PPM seems to be valid

	const unsigned int imageSize = width*height*channels;
	//const int rowSize   = width*channels;

	if (width!=0 && height!=0 && depth==255 && imageSize==data.size()-i)
	{
		// Extract image from input & flip
		flipImage(image,data.substr(i),width,height);
	}

	return 3; /* PPM images always RGB */
}

int decodeBMP(int &width,int &height,string &image,const string &data)
{
	//
	// For information about the BMP File Format:
	//
	// http://www.daubnet.com/formats/BMP.html
	// http://www.dcs.ed.ac.uk/home/mxr/gfx/2d/BMP.txt
	//

	const unsigned int fileHeaderSize = 14;
	if (data.size()<fileHeaderSize)
		return 0;

	// Check for "BM" 

	if (data[0]!='B' || data[1]!='M') 
		return 0;

	// Check the filesize matches string size

	const unsigned int fileSize = little32((int *)(data.data()+2));
	if (data.size()!=fileSize)
		return 0;

	// Get the size of the image header

	const unsigned int imageHeaderSize = little32((int *)(data.data()+fileHeaderSize));
	if (fileHeaderSize+imageHeaderSize>data.size())
		return 0;

	// Get some image info

	const int imageWidth    = little32((int *)(data.data()+fileHeaderSize+4 ));
	const int imageHeight   = little32((int *)(data.data()+fileHeaderSize+8 ));
	const short imageBits     = little16((short *)(data.data()+fileHeaderSize+14));
	const int imageCompress = little32((int *)(data.data()+fileHeaderSize+16));

	// Do some checking

	// We support only RGB images

	if (imageBits!=24)
		return 0;		

	// We don't support compressed BMP.
	//
	// According to the specs, 4-bit and 8-bit RLE
	// compression is used only for indexed images.

	if (imageCompress!=0)
		return 0;		

	const unsigned int imagePos    = little32((int *)(data.data()+10));
	const int imagePixels = imageWidth*imageHeight;
	const int lineBytes   = (imageWidth*3+3)&~3;
	const unsigned int imageBytes  = lineBytes*imageHeight;

	if (imagePos+imageBytes!=data.size())
		return 0;

	// Extract the image

	width  = imageWidth;
	height = imageHeight;
	image.resize(imagePixels*3);
	
	// Destination iterator

	byte *i = (byte *) image.data();

	for (int y=0; y<imageHeight; y++)
	{
		// Source iterator, beginning of y-th scanline

		const byte *j = (const byte *) data.data()+imagePos+lineBytes*y;

		// Copy the scanline, swapping red and blue channels
		// as we go...

		for (int x=0; x<imageWidth; x++)
		{
			*(i++) = *(j++ + 2);		
			*(i++) = *(j++);
			*(i++) = *(j++ - 2);
		}
	}

 	return 3; /* we can only read RGB PPM images */
}

int 
decodeTGA(int &width,int &height,string &image,const string &data)
{
	// Check that header appears to be valid

	const unsigned int tgaHeaderSize = 18;
	if (data.size()<tgaHeaderSize)
	{
		cerr << "TGA Header seems to be invalid." << endl;
		return 0;
	}

	// We only handle type 1, 2 and 10, for the moment

	const int tgaType = data[2];

	if (tgaType!=1 && tgaType!=2 && tgaType!=10)
	{
		cerr << "Found TGA type: " << tgaType << endl;
		return 0;
	}

	// There should be a color map for type 1

	if (tgaType==1 && data[1]!=1)
	{
		cerr << "Expecting color map in type 1 TGA." << endl;
		return 0;
	}
	
	// We only handle 24 bit or 32 bit images, for the moment

	int bytesPerPixel=0;

	if (tgaType==1)
		bytesPerPixel = data[7]==24 ? 3 : 4;	// Color mapped
	else
		bytesPerPixel = data[16]==24 ? 3 : 4;	// Unmapped
	
	if (bytesPerPixel!=3 && bytesPerPixel!=4)
	{
		cerr << "Found TGA pixel depth: " << bytesPerPixel << endl;
		return 0;
	}

	// Color map information

	const byte idSize = data[0];	// Upto 255 characters of text

	//const short origin   = little16((short *)(data.data()+3));
	const short length   = little16((short *)(data.data()+5));
	const byte   size     = data[7]>>3;
	const int mapSize  = length*size;
	const byte   *mapData = (const byte *) data.data()+tgaHeaderSize+idSize;

	//

	width  = little16((short *)(data.data()+12));
	height = little16((short *)(data.data()+14));

	// Check if TGA file seems to be the right size
	// (TGA allows for descriptive junk at the end of the
	//  file, we just ignore it)

	switch (tgaType)
	{
	case 1:		// Indexed
		{
			// TODO 
		}
		break;

	case 2:		// Unmapped RGB(A)
		{
			if (data.size()<(unsigned int)(tgaHeaderSize+idSize+width*height*bytesPerPixel))
				return 0;
		}
		break;

	case 10:	// Compressed RGB(A)
		{
			// TODO
		}
		break;
	}


	image.resize(width*height*bytesPerPixel);

	// Destination iterator

	      byte *i = (byte *) image.data();				             // Destination
	const byte *j = (const byte *) data.data()+tgaHeaderSize+idSize+mapSize;     // Source

	switch (tgaType)
	{

		// Indexed

		case 1:
			{
				const int pixels = width*height;

				for (int p=0; p<pixels; p++)
				{
					const byte *entry = mapData+(*(j++))*bytesPerPixel;

					*(i++) = entry[2];
					*(i++) = entry[1];
					*(i++) = entry[0];

					if (bytesPerPixel==4)
						*(i++) = entry[3];
				}
			}
			break;

		// Unmapped RGB(A)

		case 2:	
			{
				for (int y=0; y<height; y++)
				{
					// Copy the scanline, swapping red and blue channels
					// as we go...

					for (int x=0; x<width; x++)
					{
						*(i++) = *(j++ + 2);		
						*(i++) = *(j++);
						*(i++) = *(j++ - 2);

						if (bytesPerPixel==4)
							*(i++) = *(j++);	// Copy alpha
					}
				}
			}
			break;

		// Run Length Encoded, RGB(A) images 

		case 10:
			{
				const byte *iMax = (const byte *) image.data()+image.size();
				const byte *jMax = (const byte *) data.data()+data.size();

				while (i<iMax && j<jMax)
				{
					const bool rle   = ((*j)&128)==128;
					const int  count = ((*j)&127) + 1;

					j++;

					// Check if we're running out of output buffer
					if (i+count*bytesPerPixel>iMax)
						return 0;

					if (rle)
					{
						// Check if we're running out of input data
						if (j+bytesPerPixel>jMax)
							return 0;

						byte *pixel = i;

						// Get the next pixel, swapping red and blue

						*(i++) = *(j++ + 2);		
						*(i++) = *(j++);
						*(i++) = *(j++ - 2);

						if (bytesPerPixel==4)
							*(i++) = *(j++);	// Copy alpha

						// Now duplicate for rest of the run

						for (int k=0; k<count-1; k++,i+=bytesPerPixel)
							memcpy(i,pixel,bytesPerPixel);
					}
					else
					{
						// Check if we're running out of input data
						if (j+count*bytesPerPixel>jMax)
							return 0;

						// Copy pixels, swapping red and blue as
						// we go...

						for (int k=0; k<count; k++)
						{
							*(i++) = *(j++ + 2);		
							*(i++) = *(j++);
							*(i++) = *(j++ - 2);

							if (bytesPerPixel==4)
								*(i++) = *(j++);	// Copy alpha
						}

					}
				}

				if (i!=iMax)
					return 0;
			}	
			break;
	}

	// If the TGA origin is top-right
	// flip image

	if ((data[17]&32)==32)
	{
		// TODO - in-place flip
		string tmp = image;
		flipImage(image,tmp,width,height);
	}

	return bytesPerPixel;
}

//

#ifdef GLT_PNG

#include <png.h>

class GltPngReader
{
public:

	GltPngReader(const std::string &data,png_structp png_ptr)
	: pos(data.c_str()), end(data.c_str()+data.size())
	{
		png_set_read_fn(png_ptr,this,(png_rw_ptr) &GltPngReader::read);
	}

	size_t remaining() const { return end-pos; }

private:

	static void read(png_structp png_ptr, png_bytep data, png_uint_32 length)
	{
		GltPngReader *reader = (GltPngReader *) png_get_io_ptr(png_ptr); 

		assert(reader);
		if (!reader)
			return;

		const char *&pos = reader->pos;
		const char *&end = reader->end;

		assert(pos);
		assert(end);
	
		if (pos && end)
		{
			if (pos+length<=end)
			{
				memcpy(data,pos,length);
				pos += length;
			}
			else
			{
				memcpy(data,pos,end-pos);
				pos = end = NULL;
			}
		}
	}

	const char *pos;
	const char *end;
};

class GltPngWriter
{
public:

	GltPngWriter(std::string &data,png_structp png_ptr)
	: _png(png_ptr), _data(data), _size(0)
	{
		png_set_write_fn(_png,this,(png_rw_ptr) &GltPngWriter::write,(png_flush_ptr) &GltPngWriter::flush);
	}

	~GltPngWriter()
	{
		GltPngWriter::flush(_png);
		png_set_write_fn(_png,NULL,NULL,NULL);
	}

private:

	static void write(png_structp png_ptr, png_bytep data, png_uint_32 length)
	{
		GltPngWriter *writer = (GltPngWriter *) png_get_io_ptr(png_ptr); 

		assert(writer);
		if (!writer)
			return;

		list<string> &blocks = writer->_blocks;
		size_t       &size   = writer->_size;
	
		if (length>0)
		{
			blocks.push_back(string());
			string &block = blocks.back();
			block.resize(length);
			memcpy((void *) block.data(),data,length);
			size += length;
		}
	}

	static void flush(png_structp png_ptr)
	{
		GltPngWriter *writer = (GltPngWriter *) png_get_io_ptr(png_ptr); 

		assert(writer);
		if (!writer)
			return;

		string       &data   = writer->_data;
		list<string> &blocks = writer->_blocks;
		size_t       &size   = writer->_size;

		if (size>0)
		{
			size_t begin = data.size();
			data.resize(begin+size);
			for (list<string>::iterator i=blocks.begin(); i!=blocks.end(); i++)
			{
				memcpy((void *)(data.data()+begin),(void *) i->data(),i->size());
				begin += i->size();
			}
			blocks.clear();
			size = 0;
		}
	}

	png_structp   _png;
	string       &_data;
	list<string>  _blocks;
	size_t        _size;
};

//

int
decodePNG(int &width,int &height,std::string &image,const std::string &data)
{
	const char *pngSignature = "\211PNG\r\n\032\n";
   	if (data.size()<8 || strncmp(data.c_str(),pngSignature,8))
		return 0;

	png_structp png_ptr = 
		png_create_read_struct(PNG_LIBPNG_VER_STRING,(png_voidp) NULL,NULL,NULL);

	if (!png_ptr)
		return 0;

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr,(png_infopp)NULL, (png_infopp)NULL);
        	return 0;
	}

//  png_init_io(png_ptr, fp);

	GltPngReader read(data,png_ptr);

	png_read_info(png_ptr, info_ptr);

	int bit_depth, color_type, interlace_type,
        compression_type, filter_type;

	png_uint_32 w,h;

	png_get_IHDR(png_ptr, info_ptr, &w, &h,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_type);

	width = w;
	height = h;

	// TODO - Handle other bit-depths

	if (bit_depth<8)
		png_set_packing(png_ptr);

	if (bit_depth==16)
		png_set_strip_16(png_ptr);

	int channels = png_get_channels(png_ptr,info_ptr);
	
	int bytesPerPixel=0;
	switch (color_type)
	{
		case PNG_COLOR_TYPE_GRAY:       bytesPerPixel=1; break;
		case PNG_COLOR_TYPE_GRAY_ALPHA: bytesPerPixel=2; break;
		case PNG_COLOR_TYPE_RGB:        bytesPerPixel=3; break;
		case PNG_COLOR_TYPE_RGB_ALPHA:  bytesPerPixel=4; break;
		case PNG_COLOR_TYPE_PALETTE:    
			bytesPerPixel=4; 
			png_set_palette_to_rgb(png_ptr);
			break;
		default:
			return 0;
	}
	image.resize(width*height*bytesPerPixel); 

	int rowbytes = image.size()/height;
	assert(image.size()==rowbytes*height);

	const char **row_pointers = (const char **) malloc(height*sizeof(char *));
	for(int i=0;i<height;i++)
		row_pointers[i] = image.c_str() + rowbytes*(height-1-i);

	png_read_image(png_ptr, (png_bytepp)row_pointers);

	free(row_pointers);
	free(info_ptr);
	free(png_ptr);

	return bytesPerPixel;
}

bool 
encodePNG(std::string &data,const int &width,const int &height,const std::string &image)
{
	png_structp png_ptr =
		png_create_write_struct(PNG_LIBPNG_VER_STRING,(png_voidp) NULL,NULL,NULL);
	
	if (!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,(png_infopp)NULL);
        	return false;
	}

	{
		GltPngWriter write(data,png_ptr);
	
		const int channels   = image.size()/(width*height);
		const int bit_depth  = 8;
		      int color_type = 0; 

		switch (channels)
		{
			case 1:	color_type = PNG_COLOR_TYPE_GRAY;       break;
			case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA; break;
			case 3: color_type = PNG_COLOR_TYPE_RGB;        break;
			case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA;  break;
		}

		assert(color_type);

		png_set_IHDR(
			png_ptr,info_ptr,width,height,bit_depth,color_type,
			PNG_INTERLACE_ADAM7,PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
		png_set_strip_16(png_ptr);
		png_set_packing(png_ptr);

		int rowbytes = image.size()/height;
		assert(image.size()==rowbytes*height);

		const char **row_pointers = (const char **) malloc(height*sizeof(char *));
		for(int i=0;i<height;i++)
			row_pointers[i] = image.c_str() + rowbytes*(height-1-i);

		png_write_info(png_ptr, info_ptr);
		png_write_image(png_ptr,(png_bytepp) row_pointers);
		png_write_end(png_ptr,info_ptr);

		free(row_pointers);
	}

	free(info_ptr);
	free(png_ptr);
	        	
	return true;
}

#endif

//
// Image encoding
//

bool 
encodePPM(string &data,const int width,const int height,const string &image)
{
	const bool ppm = width*height*3==(int)image.size();
	const bool pgm = width*height  ==(int)image.size();

	// Check that image data matches the dimensions

	if (!ppm && !pgm)
		return false;

	// PPM P6/P5 header 

	char header[100];
	sprintf(header,"%s\n%u %u\n255\n",ppm ? "P6" : "P5",width,height);

	// Assemble PPM file

	data.resize(strlen(header)+image.size());
	assert(data.size()==strlen(header)+image.size());
	strcpy((char *) data.data(),header);

	// Copy and flip

	const int lineSize = ppm ? width*3 : width;

	// Destination pointer in data buffer
	byte *i = (byte *) data.data() + strlen(header);

	// Source pointer in image buffer
	const byte *j = (const byte *) image.data() + image.size() - lineSize;

	// Copy scan-lines until finished
	for (int k=0; k<height; i+=lineSize, j-=lineSize, k++)
		memcpy(i,j,lineSize);

	return true;
}

bool 
encodeTGA(string &data,const int width,const int height,bool with_alpha,const string &image)
{
	// Check that image data matches the
	// dimensions

	if (width*height*(with_alpha?4:3)!=(int)image.size())
		return false;

	// Allocate space for header and image
	data.resize(18+image.size());

	data[0] = 0;
	data[1] = 0;
	data[2] = 2;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;
	data[9] = 0;
	data[10] = 0;
	data[11] = 0;
	short w=width, h=height;
	*(short *) (data.data()+12) = little16(&w);
	*(short *) (data.data()+14) = little16(&h);
	data[16] = with_alpha?32:24; /* bits per pixel */
	data[17] = with_alpha?8:0; /* bits per alpha channel */

	// Destination iterator

	      byte *i = (byte *) data.data()+18;				// Destination
	const byte *j = (const byte *) image.data();			// Source

	for (int y=0; y<height; y++)
	{
		// Copy the scanline, swapping red and blue channels
		// as we go...

		for (int x=0; x<width; x++)
		{
			*(i++) = *(j++ + 2);		
			*(i++) = *(j++);
			*(i++) = *(j++ - 2);
			if (with_alpha) *(i++) = *(j++);
		}
	}

//-------------------------------------------------------------------------------
//DATA TYPE 2:  Unmapped RGB images.                                             |
//_______________________________________________________________________________|
//| Offset | Length |                     Description                            |
//|--------|--------|------------------------------------------------------------|
//|--------|--------|------------------------------------------------------------|
//|    0   |     1  |  Number of Characters in Identification Field.             |
//|        |        |                                                            |
//|        |        |  This field is a one-byte unsigned integer, specifying     |
//|        |        |  the length of the Image Identification Field.  Its value  |
//|        |        |  is 0 to 255.  A value of 0 means that no Image            |
//|        |        |  Identification Field is included.                         |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//|    1   |     1  |  Color Map Type.                                           |
//|        |        |                                                            |
//|        |        |  This field contains either 0 or 1.  0 means no color map  |
//|        |        |  is included.  1 means a color map is included, but since  |
//|        |        |  this is an unmapped image it is usually ignored.  TIPS    |
//|        |        |  ( a Targa paint system ) will set the border color        |
//|        |        |  the first map color if it is present.                     |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//|    2   |     1  |  Image Type Code.                                          |
//|        |        |                                                            |
//|        |        |  This field will always contain a binary 2.                |
//|        |        |  ( That's what makes it Data Type 2 ).                     |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//|    3   |     5  |  Color Map Specification.                                  |
//|        |        |                                                            |
//|        |        |  Ignored if Color Map Type is 0; otherwise, interpreted    |
//|        |        |  as follows:                                               |
//|        |        |                                                            |
//|    3   |     2  |  Color Map Origin.                                         |
//|        |        |  Integer ( lo-hi ) index of first color map entry.         |
//|        |        |                                                            |
//|    5   |     2  |  Color Map Length.                                         |
//|        |        |  Integer ( lo-hi ) count of color map entries.             |
//|        |        |                                                            |
//|    7   |     1  |  Color Map Entry Size.                                     |
//|        |        |  Number of bits in color map entry.  16 for the Targa 16,  |
//|        |        |  24 for the Targa 24, 32 for the Targa 32.                 |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//|    8   |    10  |  Image Specification.                                      |
//|        |        |                                                            |
//|    8   |     2  |  X Origin of Image.                                        |
//|        |        |  Integer ( lo-hi ) X coordinate of the lower left corner   |
//|        |        |  of the image.                                             |
//|        |        |                                                            |
//|   10   |     2  |  Y Origin of Image.                                        |
//|        |        |  Integer ( lo-hi ) Y coordinate of the lower left corner   |
//|        |        |  of the image.                                             |
//|        |        |                                                            |
//|   12   |     2  |  Width of Image.                                           |
//|        |        |  Integer ( lo-hi ) width of the image in pixels.           |
//|        |        |                                                            |
//|   14   |     2  |  Height of Image.                                          |
//|        |        |  Integer ( lo-hi ) height of the image in pixels.          |
//|        |        |                                                            |
//|   16   |     1  |  Image Pixel Size.                                         |
//|        |        |  Number of bits in a pixel.  This is 16 for Targa 16,      |
//|        |        |  24 for Targa 24, and .... well, you get the idea.         |
//|        |        |                                                            |
//|   17   |     1  |  Image Descriptor Byte.                                    |
//|        |        |  Bits 3-0 - number of attribute bits associated with each  |
//|        |        |             pixel.  For the Targa 16, this would be 0 or   |
//|        |        |             1.  For the Targa 24, it should be 0.  For     |
//|        |        |             Targa 32, it should be 8.                      |
//|        |        |  Bit 4    - reserved.  Must be set to 0.                   |
//|        |        |  Bit 5    - screen origin bit.                             |
//|        |        |             0 = Origin in lower left-hand corner.          |
//|        |        |             1 = Origin in upper left-hand corner.          |
//|        |        |             Must be 0 for Truevision images.               |
//|        |        |  Bits 7-6 - Data storage interleaving flag.                |
//|        |        |             00 = non-interleaved.                          |
//|        |        |             01 = two-way (even/odd) interleaving.          |
//|        |        |             10 = four way interleaving.                    |
//|        |        |             11 = reserved.                                 |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//|   18   | varies |  Image Identification Field.                               |
//|        |        |  Contains a free-form identification field of the length   |
//|        |        |  specified in byte 1 of the image record.  It's usually    |
//|        |        |  omitted ( length in byte 1 = 0 ), but can be up to 255    |
//|        |        |  characters.  If more identification information is        |
//|        |        |  required, it can be stored after the image data.          |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//| varies | varies |  Color map data.                                           |
//|        |        |                                                            |
//|        |        |  If the Color Map Type is 0, this field doesn't exist.     |
//|        |        |  Otherwise, just read past it to get to the image.         |
//|        |        |  The Color Map Specification describes the size of each    |
//|        |        |  entry, and the number of entries you'll have to skip.     |
//|        |        |  Each color map entry is 2, 3, or 4 bytes.                 |
//|        |        |                                                            |
//|--------|--------|------------------------------------------------------------|
//| varies | varies |  Image Data Field.                                         |
//|        |        |                                                            |
//|        |        |  This field specifies (width) x (height) pixels.  Each     |
//|        |        |  pixel specifies an RGB color value, which is stored as    |
//|        |        |  an integral number of bytes.                              |
//|        |        |                                                            |
//|        |        |  The 2 byte entry is broken down as follows:               |
//|        |        |  ARRRRRGG GGGBBBBB, where each letter represents a bit.    |
//|        |        |  But, because of the lo-hi storage order, the first byte   |
//|        |        |  coming from the file will actually be GGGBBBBB, and the   |
//|        |        |  second will be ARRRRRGG. "A" represents an attribute bit. |
//|        |        |                                                            |
//|        |        |  The 3 byte entry contains 1 byte each of blue, green,     |
//|        |        |  and red.                                                  |
//|        |        |                                                            |
//|        |        |  The 4 byte entry contains 1 byte each of blue, green,     |
//|        |        |  red, and attribute.  For faster speed (because of the     |
//|        |        |  hardware of the Targa board itself), Targa 24 images are  |
//|        |        |  sometimes stored as Targa 32 images.                      |
//|        |        |                                                            |
//--------------------------------------------------------------------------------

 	return true;
}


/** Read an entire file into a C++ string. */
std::string readTexFileIntoString(const char *fName) {
	char c; std::string ret;
	std::ifstream f(fName,std::ios::binary);
	if (!f) {return ret;} /* empty */
	while (f.read(&c,1)) ret+=c;
        return ret;
}
/** Bottom line: read this texture image from disk, and upload to OpenGL */
bool readTextureFromFile(const char *fName,GLenum internalFormat) {
	std::string pixel_data;
	int wid,ht;
	int bpp=decode(wid,ht,pixel_data,readTexFileIntoString(fName));
	GLenum data_format=GL_RGB;
	switch (bpp) {
	case 1: data_format=GL_LUMINANCE; break;
	case 2: data_format=GL_LUMINANCE_ALPHA; break;
	case 3: data_format=GL_RGB; break;
	case 4: data_format=GL_RGBA; break;
	case 0:
		std::printf("Cannot read texture '%s'; skipping...\n",fName);
		return false;
	default:
		std::printf("Texture '%s' has unexpected depth %d; skipping...\n",fName,bpp);
		return false;
	}
	gluBuild2DMipmaps(GL_TEXTURE_2D,
		internalFormat, /* internal format (on graphics card) */
		wid,ht, /* size of image, in pixels */
		data_format, /* format of passed-in data */
		GL_UNSIGNED_BYTE, /* data type of passed-in data */
		&pixel_data[0]);
	return true;
}
