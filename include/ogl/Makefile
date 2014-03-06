# Makefile for OpenGL library

GLDIR=../../build
include $(GLDIR)/Make.inc
include $(GLDIR)/Make.oslgraphics

LIBDEST=../libogl.$a
LIBDEST_CORE=../libcharm_ogl.$a
OBJS_CORE=util.$o fast_mipmaps.$o glew.$o ext.$o liltex.$o ../osl/viewpoint.$o \
	framebuffer.$o readback.$o cubemap.$o program.$o event.$o \
	texatlas.$o tex2tex.$o 
OBJS=$(OBJS_CORE) ../ckhashtable.$o 

all: main.$o $(LIBDEST) $(LIBDEST_CORE)
include $(GLDIR)/Make.library

$(LIBDEST_CORE): $(OBJS_CORE)
	$(CREATELIB)$@ $(OBJS_CORE)

superclean: clean
	- rm main.$o

