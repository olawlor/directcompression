# Standalone makefile for C++ OpenGL program

OPTS=-O3

# Linux libraries: you probably want freeglut (dev package) for "-lglut"
SYSLIBS= -L/usr/local/lib -L/usr/X11R6/lib \
	-lglut -lGLU -lGL -lpthread -lm

# Some older Linux machines need way more libraries:
#SYSLIBS= -L/usr/local/lib -L/usr/X11R6/lib \
#	-lglut -lGLU -lGL  -lXxf86vm  -lXmu -lX11 -lXext -lXi -ldl -lpthread -lm

# For MacOS X, use this set of libraries instead:
# SYSLIBS= -framework GLUT -framework OpenGL -lobjc 

# Compiler and flags
CCC=g++
CC=gcc
INC=../../include
CFLAGS=-I$(INC) -Wall $(OPTS)

# Program pieces
DEST=main
OBJS=main.o \
	$(INC)/ogl/glew.o \
	$(INC)/glui/algebra3.o \
	$(INC)/glui/arcball.o \
	$(INC)/glui/glui_add_controls.o \
	$(INC)/glui/glui_bitmap_img_data.o \
	$(INC)/glui/glui_bitmaps.o \
	$(INC)/glui/glui_button.o \
	$(INC)/glui/glui_checkbox.o \
	$(INC)/glui/glui_column.o \
	$(INC)/glui/glui_commandline.o \
	$(INC)/glui/glui_control.o \
	$(INC)/glui/glui.o \
	$(INC)/glui/glui_edittext.o \
	$(INC)/glui/glui_filebrowser.o \
	$(INC)/glui/glui_listbox.o \
	$(INC)/glui/glui_list.o \
	$(INC)/glui/glui_mouse_iaction.o \
	$(INC)/glui/glui_node.o \
	$(INC)/glui/glui_panel.o \
	$(INC)/glui/glui_radio.o \
	$(INC)/glui/glui_rollout.o \
	$(INC)/glui/glui_rotation.o \
	$(INC)/glui/glui_scrollbar.o \
	$(INC)/glui/glui_separator.o \
	$(INC)/glui/glui_spinner.o \
	$(INC)/glui/glui_statictext.o \
	$(INC)/glui/glui_string.o \
	$(INC)/glui/glui_textbox.o \
	$(INC)/glui/glui_translation.o \
	$(INC)/glui/glui_tree.o \
	$(INC)/glui/glui_treepanel.o \
	$(INC)/glui/glui_window.o \
	$(INC)/glui/quaternion.o \
	$(INC)/glui/viewmodel.o

all: $(DEST)

# Build main from object files
$(DEST): $(OBJS) 
	$(CCC) $(CFLAGS) $(OBJS) $(SYSLIBS) -o $(DEST)

clean:
	-rm $(OBJS) $(DEST)

# Trick gmake into compiling .cpp into .o
o=o
OUTFLAG=-o
%.$o: %.cpp 
	$(CCC) $(CFLAGS) -c $< $(OUTFLAG)$@

%.$o: %.C
	$(CCC) $(CFLAGS) -c $< $(OUTFLAG)$@

%.$o: %.c
	$(CC) $(CFLAGS) -c $< $(OUTFLAG)$@

# Trick other makes into compiling .cpp's into .o's.
.SUFFIXES: .cpp .C .c

.cpp.$o:
	$(CCC) $(CFLAGS) -c $< $(OUTFLAG)$@

.C.$o:
	$(CCC) $(CFLAGS) -c $< $(OUTFLAG)$@

.c.$o:
	$(CC) $(CFLAGS) -c $< $(OUTFLAG)$@
