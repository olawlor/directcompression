A rendering technique that attempts to gain 10-100x speed using the same methods that image compression tools use to gain 10-100x space--frequency domain transformation and quantization. The basic idea is to check geometric and inter-frame predictions using a sparse grid of raytraced samples. The method should scale to either CPU or GPU renderers, offline or realtime.

Dr. Orion Lawlor, lawlor@alaska.edu
Dr. Jon Genetti, jdgenetti@alaska.edu

This repository can be accessed via:
	git clone http://projects.cs.uaf.edu/directcompression

See also:
	https://projects.cs.uaf.edu/redmine/projects/directcompression



---------- Still Image Rendering via Multigrid --------------
One implementation of this idea for individual frames is essentially multigrid for rendering:
   - Start by rendering a coarse grid
   - For each pixel of a finer grid,
   	- Evaluate an error function by examining the pixels of the coarse grid
   	- If interesting things seem to be happening, render the pixel
   	- If nothing interesting is happening, interpolate the pixel
   - Repeat for finer grids until you reach screen pixels (or beyond)

The simplest error function and "interesting" criteria could be based on the final rendered color, which works for scenes with smoothly varying color such as cloudy volume rendering, but fails for many textured objects such as a checkerboard.  Instead, the error function could be based on object depths, object IDs, texture coordinates, or other deferred shader parameters.

Note that at each stage, our render target is a regular dense 2D grid of pixels, which makes this approach amenable to even decade-old programmable graphics hardware.  The output is also not only a finished full-resolution image, but also a pyramid of coarser images, which may be useful for postprocess HDR bloom filtering, antialiasing, or simulating camera depth of field.

For example, starting with 16x16 pixel "macropixels" (the same size as JPEG/MPEG chroma macroblocks), the first pass does 1/256th the pixel count of full resolution.  If subsequent passes only need to render 25% of the pixels, they each add another 1/256th of the full resolution cost, for a total of 5/256 (under 2%) of the full screen cost.  Even if every level turns out to require all the pixels, the overhead is a geometric series of powers of 1/4, this is only 4/3 of the fullscreen pixel count, or 33.33% overhead.

*Anything* that supports per-pixel rendering can in theory benefit from this method, including:
- Raytracers and raycasters, especially for volume rendering or global illumination.  This technique is entirely orthogonal to the raytracing accelleration structure used.
- Conetracers, which can also give some clue when only a single object was hit
- Fractal renderers
	https://www.cs.uaf.edu/2013/spring/cs493/lecture/demo/03_19_fractal.html


Prior work:

"Terrain simplification simplified: A general framework for view-dependent out-of-core visualization"
Peter Lindstrom and Valerio Pascucci
  IEEE Transactions on Visualization and Computer Graphics, 8(3), 2002
     They show how to build a manifold spatial tree without hanging nodes, by baking an opening criteria into each node.  This could be used to create geometry between our interpolated scene samples.



----------- Animation: MPEG output --------------
A renderer can use the known geometry of the rendered objects and the camera paths to determine how rendered objects will move onscreen.  This could be used in two ways: first, in MPEG output to create perfect motion vectors for block motion "prediction"; second, during realtime rendering to exploit frame coherence.

Prior work: 

"Accelerating Ray Tracing by Exploiting Frame-to-Frame Coherence"
Joe Demers, Ilmi Yoon, Tae-Yong Kim, Ulrich Neumann
1998, USC Computer Science Tech Report 98-668
	http://www.cs.usc.edu/assets/004/83269.pdf
	Aliasing is a major issue with this approach

