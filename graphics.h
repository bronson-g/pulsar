
#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
	#include <GLUT/glut.h>
#elif __linux__
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glut.h>
#endif

/* world size and storage array */
#define WORLDX 100
#define WORLDY 10
#define WORLDZ 100
GLubyte  world[WORLDX][WORLDY][WORLDZ];

#define MOB_COUNT 10
#define PLAYER_COUNT 10

#define MAX_DISPLAY_LIST 500000
