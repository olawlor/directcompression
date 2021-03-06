/* osl/fft.h:
	This file is an external interface to osl_fft.a.
It contains the 1-D fft routines.  Also see osl/fft2d.h

Note: To use these routines, be SURE to call fftInit
with the size you intend to use-- the routines, for speed,
do NOT check to make sure they have been initialized.
*/
/*******************************************************************
	This file extends the fftlib with calls to maintain the cosine and bit reversed tables
	for you (including mallocs and free's).  Call the init routine for each fft size you will
	be using.  Then you can call the fft routines below which will make the fftlib library 
	call with the appropriate tables passed.  When you are done with all fft's you can call 
	fftfree to release the storage for the tables.  Note that you can call fftinit repeatedly
	with the same size, the extra calls will be ignored. So, you could make a macro to call
	fftInit every time you call ffts. For example you could have someting like:
	#define FFT(a,n) if(!fftInit(roundtol(LOG2(n)))) ffts(a,roundtol(LOG2(n)),1);else printf("fft error\n");
*******************************************************************/
#ifndef __OSL_FFT_H
#define __OSL_FFT_H
#ifdef __cplusplus
 extern "C" {
#endif

int fftInit(int M);
/* malloc and init cosine and bit reversed tables for a given size fft, ifft, rfft, rifft*/
/* INPUTS */
/* M = log2 of fft size	(ex M=10 for 1024 Point fft) */
/* OUTPUTS */
/* private cosine and bit reversed tables	*/

void fftFree();
/* release storage for all private cosine and bit reversed tables*/

void ffts(float *data, int M, int Rows);
/* Compute in-place complex fft on the rows of the input array	*/
/* INPUTS */
/* *ioptr = input data array	*/
/* M = log2 of fft size	(ex M=10 for 1024 Point fft) */
/* Rows = number of rows in ioptr array (use 1 for Rows for a single fft)	*/
/* OUTPUTS */
/* *ioptr = output data array	*/

void iffts(float *data, int M, int Rows);
/* Compute in-place inverse complex fft on the rows of the input array	*/
/* INPUTS */
/* *ioptr = input data array	*/
/* M = log2 of fft size	(ex M=10 for 1024 Point fft) */
/* Rows = number of rows in ioptr array (use 1 for Rows for a single fft)	*/
/* OUTPUTS */
/* *ioptr = output data array	*/

void rffts(float *data, int M, int Rows);
/* Compute in-place real fft on the rows of the input array	*/
/* The result is the complex spectra of the positive frequencies */
/* except the location for the first complex number contains the real */
/* values for DC and Nyquest */
/* See rspectprod for multiplying two of these spectra together- ex. for fast convolution */
/* INPUTS */
/* *ioptr = real input data array	*/
/* M = log2 of fft size	(ex M=10 for 1024 Point fft) */
/* Rows = number of rows in ioptr array (use 1 for Rows for a single fft)	*/
/* OUTPUTS */
/* *ioptr = output data array	in the following order */
/* Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]), Re(x[2]), Im(x[2]), ... Re(x[N/2-1]), Im(x[N/2-1]). */

void riffts(float *data, int M, int Rows);
/* Compute in-place real ifft on the rows of the input array	*/
/* data order as from rffts */
/* INPUTS */
/* *ioptr = input data array in the following order	*/
/* M = log2 of fft size	(ex M=10 for 1024 Point fft) */
/* Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]), Re(x[2]), Im(x[2]), ... Re(x[N/2-1]), Im(x[N/2-1]). */
/* Rows = number of rows in ioptr array (use 1 for Rows for a single fft)	*/
/* OUTPUTS */
/* *ioptr = real output data array	*/

void rspectprod(float *data1, float *data2, float *outdata, int N);
/* When multiplying a pair of spectra from rfft care must be taken to multiply the*/
/* two real values seperately from the complex ones. This routine does it correctly.*/
/* the result can be stored in-place over one of the inputs*/
/* INPUTS */
/* *data1 = input data array	first spectra */
/* *data2 = input data array	second spectra */
/* N = fft input size for both data1 and data2 */
/* OUTPUTS */
/* *outdata = output data array spectra */


/* The following is FYI*/


/*Note that most of the fft routines require full matrices, ie Rsiz==Ncols*/
/*This is how I like to define a real Matrix:*/
/*struct Matrix {		// real Matrix*/
/*	float *d; 		// Pointer to data*/
/*	int Nrows;		// number of rows in the Matrix*/
/*	int Ncols;		// number of columns in the Matrix (can be less than Rsiz)*/
/*	int Rsiz;		// number of floats from one row to the next*/
/*};*/
/*typedef struct Matrix Matrix;*/



/* CACHEFILLMALLOC and CEILCACHELINE can be used instead of malloc to make*/
/* arrays that start exactly on a cache line start.*/
/* First we CACHEFILLMALLOC a void * (use this void * when free'ing),*/
/* then we set our array Pointer equal to the properly cast CEILCACHELINE of this void **/
/* example:*/
/* aInit = CACHEFILLMALLOC( NUMFLOATS*sizeof(float) );*/
/* a = (float *) CEILCACHELINE(ainit);*/
/* ... main body of code ...*/
/* free(aInit);*/
/**/
/* To disable this alignment, set CACHELINESIZE to 1*/
/*#define CACHELINESIZE 32				// Bytes per cache line*/
/*#define CACHELINEFILL (CACHELINESIZE-1)*/
/*#define CEILCACHELINE(p) ((((unsigned long)p+CACHELINEFILL)/CACHELINESIZE)*CACHELINESIZE)*/
/*#define CACHEFILLMALLOC(n) malloc((n)+CACHELINEFILL)*/

#ifdef __cplusplus
 };
#endif
#endif // __OSL_FFT_H
