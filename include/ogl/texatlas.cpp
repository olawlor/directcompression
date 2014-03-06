/*
Utility routine to agglomerate a series of different-sized textures
into one giant texture.

Orion Sky Lawlor, olawlor@acm.org, 2008-12-01 (Public Domain)
*/
#include "texatlas.h"

oglTexAtlas::oglTexAtlas(int tileSize_)
	:tileSizeX(tileSize_), tileSizeY(tileSize_),
	 T(0,0), P(0,0), Pinv(-999.0,-999.0) {resize(1,1);}

/* Make room for this image in the atlas's bookkeeping.  
   Image size is given in pixels.
   Returns the image's atlas number. */
int oglTexAtlas::add(int wid,int ht)
{
	sizeP.push_back(point2i(wid,ht));
	/* Size of the incoming image, in tiles: */
	point2i imgT(
		(wid+tileSizeX-1)/tileSizeX, /* round up to next full tile */
		(ht+tileSizeY-1)/tileSizeY
	);
	sizeT.push_back(imgT);
	
	/* If we're *way* too small for this image-- make plenty of room before even looking */
	while (imgT.x>T.x) resize(T.x*2,T.y);
	while (imgT.y>T.y) resize(T.x,T.y*2);
	int x,y,dx,dy, ret;
	
find_slot:
	/* Quite possibly the world's slowest greedy rectangle fitting code.
	   It's brute force, and hence asymptotically inefficient.
	*/
	for (y=0;y<T.y-imgT.y;y++)
	for (x=0;x<T.x-imgT.x;x++) 
	{ /* Consider storing image at tile (x,y): */
		for (dy=0;dy<imgT.y;dy++)
		for (dx=0;dx<imgT.x;dx++) {
			if (inuse[(x+dx)+(y+dy)*T.x])
				goto keep_looking; /* can't store image at (x,y) */
		}
		/* If we got here, none of those tiles are in use, so 
		  the image can be stored at (x,y)!  Excellent! */
		ret=cornerT.size();
		cornerT.push_back(point2i(x,y));
		cornerP.push_back(point2i(x*tileSizeX,y*tileSizeY));
		for (dy=0;dy<imgT.y;dy++) /* mark these slots as in-use */
		for (dx=0;dx<imgT.x;dx++) 
			inuse[(x+dx)+(y+dy)*T.x]=true;
		return ret;
		
	keep_looking:;
	}
	
	/* Well, if we got here, there *is* no place to store our image,
	so we have to enlarge the atlas. */

	if (T.x>T.y) resize(T.x,T.y*2); /* make Y bigger */
	else resize(T.x*2,T.y); /* make X bigger */
	goto find_slot; /* try again for an empty slot (might take a while) */
}


/* Resize the inuse vector to be this big */
void oglTexAtlas::resize(int newTx,int newTy)
{
	std::vector<bool> newuse(newTx*newTy,false);
	for (int y=0;y<T.y;y++)
	for (int x=0;x<T.x;x++) 
		newuse[x+y*newTx]=inuse[x+y*T.x];
	T=point2i(newTx,newTy);
	P=point2i(newTx*tileSizeX,newTy*tileSizeY);
	Pinv=point2f(1.0/P.x,1.0/P.y);
	inuse=newuse;
}

