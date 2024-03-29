CC=gcc
CFLAGS=-I.

LIBS=-framework OpenGL -framework GLUT

_DEPS = glm.h dirent32.h gltb.h
DEPS = $(_DEPS)

_OBJ = smooth.o gltb.o glm.o
OBJ = $(_OBJ)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

smooth: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)