INCLUDES = -F/System/Library/Frameworks -framework OpenGL -framework GLUT -lm
LINUXINCLUDES = -g -Wall -F/System/Library/Frameworks -lGL -lGLU -lm -lglut

a3: a4.c graphics.c visible.c graphics.h
	gcc a4.c graphics.c visible.c -o a4 $(INCLUDES) 

linux: a4.c graphics.c visible.c graphics.h
	gcc a4.c graphics.c visible.c -o a4 $(LINUXINCLUDES)
clean:
	rm a4
