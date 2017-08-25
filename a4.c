
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "graphics.h"

/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void getMobPosition(int, float*, float*, float*, float*, float*, float*);
extern int isMobVisible(int);
extern void hideMob(int);
extern void showMob(int);
extern void mobTrajectory(int, float, float);

   /* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void getOldViewPosition(float *, float *, float *);
extern void setViewOrientation(float, float, float);
extern void getViewOrientation(float *, float *, float *);

void startLevel();
void endLevel();

typedef enum colour {
   empty,
   green,
   blue,
   red,
   black,
   white,
   purple,
   orange,
   yellow
} colour;

typedef struct enemy {
   int id;
   int size;
   int x;
   int y;
   int z;
   int dest_x;
   int dest_z;
   colour*** form;
   char type;
   colour col;
} enemy;

colour map[WORLDX][WORLDZ] = {{empty}};
colour key[11][3] = {{empty}};
colour message[4][78] = {{empty}};
int dropBox[3] = {0};
int dest[2] = {0};
enemy* enemies = NULL;
int n_enemies = 0;
int hasKey = 0;
int dropParty = 0;
int bouncing = 0;
int shot = 0;
int level = 0;
int wall_moving = 0;
int ind = 0;

typedef struct wall_chunk {
   int x;
   int z;
   int dir;
   colour col;
} wall_chunk;

void quit() {
   endLevel();
   printf("You made it to level %d.\n", level);
   exit(0);
}

enemy* getEnemy(int x, int y, int z) {
   for(int n=0; n<n_enemies; n++) {
      int off = enemies[n].size/2;
      for(int i=0; i<enemies[n].size; i++) {
         for(int j=0; j<enemies[n].size; j++) {
            for(int k=0; k<enemies[n].size; k++) {
               if(i+(enemies[n].x)-off == x && j+(enemies[n].y)-off == y && k+(enemies[n].z)-off == z) {
                  return &enemies[n];
               }
            }
         }
      }
   }
   return NULL;
}

double radian(double degree) {
   return degree*M_PI/180.0;
}
double degree(double radian) {
   return radian*180.0/M_PI;
}

void randDestCell(enemy* mob, int dir) {
   int cell = dir%4;
   mob->dest_z = mob->z;
   mob->dest_x = mob->x;

   if(cell == 0 && mob->x < (WORLDX -16) -mob->size) {
      mob->dest_x = mob->x + 16;
   } else if(cell == 1 && mob->x > 16 +mob->size) {
      mob->dest_x = mob->x - 16;
   } else if(cell == 2 && mob->z < (WORLDZ -16) -mob->size) {
      mob->dest_z = mob->z + 16;
   } else if(cell == 3 && mob->z > 16 +mob->size) {
      mob->dest_z = mob->z - 16;
   }
}

enemy initEnemy(char type, int size, int x, int z, int temp, int dir) {
   static int id = 1;
   enemy ret;

   if((type != 'x' && type != 'o') || size < 0 || x < 0 || z < 0) {
      id = 1;
   }

   ret.id = id;
   ret.size = size;
   ret.x = x;
   ret.y = (size/2)+1;
   ret.z = z;
   ret.type = type;

   if(type == 'o') {
      ret.col = yellow;
   } else if(type == 'x') {
      ret.col = orange;
   }

   ret.form = malloc(sizeof(colour**)*size);
   for(int i=0; i<size; i++) {
      ret.form[i] = malloc(sizeof(colour*)*size);
      for(int j=0; j<size; j++) {
         ret.form[i][j] = malloc(sizeof(colour)*size);
      }
   }

   if(!temp) {
      randDestCell(&ret, dir);
      id++;
   } else {
      ret.dest_x = 0;
      ret.dest_z = 0;
   }

   return ret;
}

void destroyEnemy(enemy mob) {
   for(int i=0; i<mob.size; i++) {
      for(int j=0; j<mob.size; j++) {
         free(mob.form[i][j]);
      }
      free(mob.form[i]);
   }
   free(mob.form);
}

void eraseEnemy(enemy mob) {
   int off = mob.size/2;
   for(int i=0; i<mob.size; i++) {
      for(int j=0; j<mob.size; j++) {
         for(int k=0; k<mob.size; k++) {
            if(world[i+(mob.x)-off][j+(mob.y)-off][k+(mob.z)-off] == mob.col) {
               world[i+(mob.x)-off][j+(mob.y)-off][k+(mob.z)-off] = empty;
            }
            if(map[i+(mob.x)-off][k+(mob.z)-off] == mob.col) {
               map[i+(mob.x)-off][k+(mob.z)-off] = empty;
            }
         }
      }
   }
}

void animateEnemy(enemy* mob) {
   enemy temp = initEnemy(mob->type, mob->size, mob->x, mob->z, 1, 1);
   int off = mob->size/2;

   for(int i=0; i<mob->size; i++) {
      for(int j=0; j<mob->size; j++) {
         for(int k=0; k<mob->size; k++) {
            if(world[i+(mob->x)-off][j+(mob->y)-off][k+(mob->z)-off] == empty) {
               world[i+(mob->x)-off][j+(mob->y)-off][k+(mob->z)-off] = mob->form[i][j][k];
            }
            if(map[i+(mob->x)-off][k+(mob->z)-off] == empty) {
               map[i+(mob->x)-off][k+(mob->z)-off] = mob->form[i][off][k];
            }
         }
      }
   }
   for(int i=0; i<mob->size; i++) {
      for(int j=0; j<mob->size; j++) {
         for(int k=0; k<mob->size; k++) {
            temp.form[i][j][k] = mob->form[k][i][j];
         }
      }
   }
   colour*** swap = mob->form;
   mob->form = temp.form;
   temp.form = swap;
   destroyEnemy(temp);
}

void behaveEnemy(enemy* mob, int random) {
   float x, y, z, rotx, roty, rotz, yrot;
   int see = 1;
   getViewPosition(&x, &y, &z);
   getViewOrientation(&rotx, &roty, &rotz);

   if(mob->x != mob->dest_x) {
      int off = 0;
      if(mob->dest_x > mob->x) {
         off = 1;
      } else {
         off = -1;
      }
      for(int i=0; i<mob->size; i++) {
         for(int j=0; j<mob->size; j++) {
            if(world[mob->x+((mob->size-i)*off)][mob->y][mob->z+j-1] != empty) {
               randDestCell(mob, random);
               return;
            }
         }
      }
   }
   if(mob->z != mob->dest_z) {
      int off = 0;
      if(mob->dest_z > mob->z) {
         off = 1;
      } else {
         off = -1;
      }
      for(int i=0; i<mob->size; i++) {
         for(int j=0; j<mob->size; j++) {
            if(world[mob->x+j-1][mob->y][mob->z+((mob->size-i)*off)] != empty) {
               randDestCell(mob, random);
               return;
            }
         }
      }
   }
   
   if(mob->dest_x == mob->x && mob->dest_z == mob->z) {
      randDestCell(mob, random);
   } else if(mob->x < mob->dest_x) {
      mob->x = mob->x+1;
   } else if(mob->x > mob->dest_x) {
      mob->x = mob->x-1;
   }
   if(mob->z < mob->dest_z) {
      mob->z = mob->z+1;
   } else if(mob->z > mob->dest_z) {
      mob->z = mob->z-1;
   }

   x = fabs(x);
   z = fabs(z);
   int zdir = 1;
   int xdir = 1;

   if(mob->z > z) {
      zdir = -1;
   }
   if(mob->x > x) {
      xdir = -1;
   }

   for(int i=0; i<abs((int)mob->x-x); i++) {
      for(int j=0; j<abs((int)mob->y-y); j++) {
         for(int k=0; k<abs((int)mob->z-z); k++) {
            if(world[mob->x+i*xdir][mob->y+j][mob->z+k*zdir] != empty) {
               see = 0;
               break;
            }
         }
         if(!see) {
            break;
         }
      }
      if(!see) {
         break;
      }
   }
   float xrot, zrot;
   float mobx, moby, mobz, mobroty, mobrotx, mobrotz;
   int xoff=0, zoff=0;
   int origin = 0;
   float opp, adj, mult = 1.0;

   if(mob->x>x) {
      xoff = -1*mob->size;
   } else if(mob->x<x) {
      xoff = mob->size;
   }
   if(mob->z>z) {
      zoff = -1*mob->size;
   } else if(mob->z<z) {
      zoff = mob->size;
   }

   if(x > mob->x && z < mob->z) {
      origin = 90;
      opp = mob->z+zoff-z;
      adj = mob->x+xoff-x;
   } else if(z > mob->z && x > mob->x) {
      origin = 180;
      opp = mob->x+xoff-x;
      adj = mob->z+zoff-z;
   } else if(z > mob->z && x < mob->x) {
      origin = 270;
      opp = mob->z+zoff-z;
      adj = mob->x+xoff-x;
      mult = -1.0;
   } else if(z < mob->z && x < mob->x) {
      origin = 360;
      opp = mob->x+xoff-x;
      adj = mob->z+zoff-z;
   } else {
      return;
   }

   yrot = (float)origin-degree(atan(opp/adj))*mult;
   if(origin == 90) {
      yrot = 90-(yrot-90);
   }
   xrot = 0.0;
   zrot = 0.0;

   if(see && !isMobVisible(mob->id)) {
      getMobPosition(mob->id, &mobx, &moby, &mobz, &mobroty, &mobrotx, &mobrotz);
      
      createMob(mob->id, mob->x+xoff, mob->y, mob->z+zoff, yrot);
      mobTrajectory(mob->id, xrot, zrot);
      showMob(mob->id);
   }
   
   if(roty < 90-yrot+45 && roty > 90-yrot-45) {
      switch(origin) {
         case 90:
            mob->dest_x = mob->x-random;
            mob->dest_z = mob->z+random;
            break;
         case 180:
            mob->dest_x = mob->x-random;
            mob->dest_z = mob->z-random;
            break;
         case 270:
            mob->dest_x = mob->x+random;
            mob->dest_z = mob->z-random;
            break;
         case 360:
            mob->dest_x = mob->x+random;
            mob->dest_z = mob->z+random;
            break;
         default:
            break;
      }
   }
}

void teleEnemy(enemy* mob) {
   int dropOff = 0;
   int newx, newz;
   
   while(!dropOff) {
      newx = rand()%WORLDX;
      newz = rand()%WORLDZ;
      int occupied = 0;

      for(int x=newx; x<newx+3; x++) {
         for(int z=newz; z<newz+3; z++) {
            if(map[x][z] != empty) {
               occupied = 1;
               break;
            }
         }
         if(occupied) {
            break;
         }
      }
      if(!occupied) {
         dropOff = 1;
      }
   }

   mob->x = newx;
   mob->z = newz;
}

GLfloat* rgb(colour col) {
   static GLfloat val[4] = {0.0, 0.0, 0.0, 1.0};
   for(int i=0; i<3; i++) {
      val[i] = 0.0;
   }
   val[3] = 1.0;
   switch(col) {
      case red:
         val[0] = 1.0;
         break;
      case blue:
         val[2] = 1.0;
         break;
      case purple:
         val[2] = 1.0;
         val[0] = 1.0;
         break;
      case yellow:
         val[1] = 1.0;
         val[0] = 1.0;
         break;
      case white:
         val[0] = 1.0;
         val[1] = 1.0;
         val[2] = 1.0;
         break;
      case green:
         val[1] = 1.0;
         break;
      case orange:
         val[0] = 1.0;
         val[1] = 0.3;
         break;
      case black:
         val[0] = 0.8;
         val[3] = 0.4;
      case empty:
      default:
         break;
   }
   return val;
}

	/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

extern int getMapState();

	/* initialize graphics library */
extern void graphicsInit(int *, char **);

	/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

	/* add cube to display list so it will be drawn */
extern int addDisplayList(int, int, int);

	/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

	/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float []);


	/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
	/* flag used to indicate that the test world should be used */
extern int testWorld;
	/* flag to print out frames per second */
extern int fps;
	/* flag to indicate the space bar has been pressed */
extern int space;
	/* flag indicates the program is a client when set = 1 */
extern int netClient;
	/* flag indicates the program is a server when set = 1 */
extern int netServer; 
	/* size of the window in pixels */
extern int screenWidth, screenHeight;
	/* flag indicates if map is to be printed */
extern int displayMap;

	/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

	/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

/********* end of extern variable declarations **************/

void getDest(int* x, int* z) {
   int destX, destZ;
   int found = 0;
   while(!found) {
      destX = (rand()%(WORLDX-6))+3;
      destZ = (rand()%(WORLDZ-6))+3;
      for(int i=destX-1; i<destX+2; i++) {
         for(int j=destZ-1; j<destZ+2; j++) {
            if(map[i][j]) {
               found = 1;
               break;
            }
         }
      }
   }
   *x = destX;
   *z = destZ;
}

int powerUp(colour col) {
   float x,y,z;
   int destX, destZ;
   int ret = 1;

   switch(col) {
      case blue: //drop-party
         getViewPosition(&x, &y, &z);
         dropParty = 1;
         dropBox[0] = (int)fabs(x);
         dropBox[1] = (int)fabs(z);
         dropBox[2] = WORLDY-1;

         if(map[dropBox[0]-3][dropBox[1]-3] == empty) {
            colour col = (rand()%3)+1;
            map[dropBox[0]-3][dropBox[1]-3] = col;
            world[dropBox[0]-3][dropBox[2]][dropBox[1]-3] = col;
         }
         if(map[dropBox[0]+3][dropBox[1]-3] == empty) {
            colour col = (rand()%3)+1;
            map[dropBox[0]+3][dropBox[1]-3] = col;
            world[dropBox[0]+3][dropBox[2]][dropBox[1]-3] = col;
         }
         if(map[dropBox[0]-3][dropBox[1]+3] == empty) {
            colour col = (rand()%3)+1;
            map[dropBox[0]-3][dropBox[1]+3] = col;
            world[dropBox[0]-3][dropBox[2]][dropBox[1]+3] = col;
         }
         if(map[dropBox[0]+3][dropBox[1]+3] == empty) {
            colour col = (rand()%3)+1;
            map[dropBox[0]+3][dropBox[1]+3] = col;
            world[dropBox[0]+3][dropBox[2]][dropBox[1]+3] = col;
         }
         break;
      case red: //tele-other
         for(int i=0; i<n_enemies; i++) {
            eraseEnemy(enemies[i]);
            teleEnemy(&enemies[i]);
         }
         break;
      case green: //bouncy-boy
         getDest(&destX, &destZ);
         bouncing = 1;
         dest[0] = destX;
         dest[1] = destZ;

         break;
      case white: //key
         hasKey = 1;
         printf("You've acquired the key.\n");
         break;
      default:
         ret = 0;
         break;
   }
   return ret;
}


	/*** collisionResponse() ***/
	/* -performs collision detection and response */
	/*  sets new xyz  to position of the viewpoint after collision */
	/* -can also be used to implement gravity by updating y position of vp*/
	/* note that the world coordinates returned from getViewPosition()
	   will be the negative value of the array indices */
void collisionResponse() {
   float x=0.0;
   float y=0.0;
   float z=0.0;
   float oldx=0.0;
   float oldy=0.0;
   float oldz=0.0;
   float newx=0.0;
   float newy=0.0;
   float newz=0.0;
   char dirz = '\0';
   char dirx = '\0';
   int i,j;
   int mapx, mapy;
   colour col = empty;

   getViewPosition(&x, &y, &z);
   getViewPosition(&newx, &newy, &newz);
   getOldViewPosition(&oldx, &oldy, &oldz);

   mapx = abs((int)oldx);
   mapy = abs((int)oldz);

   for(i=mapx-1; i<mapx+2; i++) {
      for(j=mapy-1; j<mapy+2; j++) {
         if(i>=0 && i<WORLDX && j>=0 && j<WORLDZ && map[i][j] == white) {
            map[i][j] = empty;
         }
      }
   }

   newy = oldy;

   if(oldx-x < 0) {
      dirx = 'e';
   } else {
      dirx = 'w';
   }
   if(oldz-z < 0) {
      dirz = 'n';
   } else {
      dirz = 's';
   }

   if(dirx == 'e') {
      col = world[abs((int)(x+0.25))][abs((int)y)][abs((int)z)];
      if(col > 0) {
         if(world[abs((int)(x+0.25))][abs((int)(y-1))][abs((int)z)] == 0) {
            newy = oldy-1;
            if(powerUp(col)) {
               world[abs((int)(x+0.25))][abs((int)y)][abs((int)z)] = empty;
               map[abs((int)(x+0.25))][abs((int)z)] = empty;
            } else {
               newy = oldy-1;
            }
         } else {
            if(col == white && world[abs((int)(x+0.25))][abs((int)(y-1))][abs((int)z)] == white && hasKey) {
               printf("Congratulations, you've cleared level %d!\n", level);
               endLevel();
               startLevel();
               return;
            }
            newx = oldx;
         }
      }
   } else if(dirx == 'w') {
      col = world[abs((int)(x-0.25))][abs((int)y)][abs((int)z)];
      if(col > 0) {
         if(world[abs((int)(x-0.25))][abs((int)(y-1))][abs((int)z)] == 0) {
            if(powerUp(col)) {
               world[abs((int)(x-0.25))][abs((int)y)][abs((int)z)] = empty;
               map[abs((int)(x-0.25))][abs((int)z)] = empty;
            } else {
               newy = oldy-1;
            }
         } else {
            if(col == white && world[abs((int)(x-0.25))][abs((int)(y-1))][abs((int)z)] == white && hasKey) {
               printf("Congratulations, you've cleared level %d!\n", level);
               endLevel();
               startLevel();
               return;
            }
            newx = oldx;
         }
      }
   }

   if(dirz == 'n') {
      col = world[abs((int)x)][abs((int)y)][abs((int)(z+0.25))];
      if(col > 0) {
         if(world[abs((int)x)][abs((int)(y-1))][abs((int)(z+0.25))] == 0) {
            if(powerUp(col)) {
               world[abs((int)x)][abs((int)y)][abs((int)(z+0.25))] = empty;
               map[abs((int)x)][abs((int)(z+0.25))] = empty;
            } else {
               newy = oldy-1;
            }
         } else {
            if(col == white && world[abs((int)x)][abs((int)(y-1))][abs((int)(z+0.25))] == white && hasKey) {
               printf("Congratulations, you've cleared level %d!\n", level);
               endLevel();
               startLevel();
               return;
            }
            newz = oldz;
         }
      }
   } else if(dirz == 's') {
      col = world[abs((int)x)][abs((int)y)][abs((int)(z-0.25))];
      if(col > 0) {
         if(world[abs((int)x)][abs((int)(y-1))][abs((int)(z-0.25))] == 0) {
            if(powerUp(col)) {
               world[abs((int)x)][abs((int)y)][abs((int)(z-0.25))] = empty;
               map[abs((int)x)][abs((int)(z-0.25))] = empty;
            } else {
               newy = oldy-1;
            }
         } else {
            if(col == white && world[abs((int)x)][abs((int)(y-1))][abs((int)(z-0.25))] == white && hasKey) {
               printf("Congratulations, you've cleared level %d!\n", level);
               endLevel();
               startLevel();
               return;
            }
            newz = oldz;
         }
      }
   }

   if(fabs(newx) >= WORLDX) {
      newx = -1.0*(WORLDX-1);
   } else if(newx > 0) {
      newx = 0;
   }
   if(fabs(newz) >= WORLDZ) {
      newz = -1.0*(WORLDZ-1);
   } else if(newz > 0) {
      newz = 0;
   }

   setViewPosition(newx, newy, newz);

   mapx = abs((int)newx);
   mapy = abs((int)newz);
   for(i=mapx-1; i<mapx+2; i++) {
      for(j=mapy-1; j<mapy+2; j++) {
         if(i>=0 && i<WORLDX && j>=0 && j<WORLDZ &&map[i][j] == empty) {
            map[i][j] = white;
         }
      }
   }
}

/* chooses whether or not a wall spawns from this pillar, and if so, which direction
      0 for no change
      1 for north
      2 for east
      3 for south
      4 for west
   r is a random integer please pass in rand()
   x and z are the coords of the southeast corner of the pilar
      (only used to handle edge cases)
   */
int randomWall(int r, int x, int z, colour* col) {
   r = r%100;
   if(r <= 44) {
      if(r <= 12) {
         if(z==0 || x==WORLDX-4 || z==WORLDZ-4) {
            return 0;
         }
         if(world[x+5][(int)(WORLDY/2)][z+2] == purple) {
            *col = empty;
         } else {
            *col = purple;
         }
         return 1;
      } else if(r<=24) {
         if(x==WORLDX-4 || x==0 || z==WORLDZ-4) {
            return 0;
         }
         if(world[x+2][(int)(WORLDY/2)][z+5] == purple) {
            *col = empty;
         } else {
            *col = purple;
         }
         return 2;
      } else if(r<=36) {
         if(z==0 || x==0 || z==WORLDZ-4) {
            return 0;
         }
         if(world[x-2][(int)(WORLDY/2)][z+2] == purple) {
            *col = empty;
         } else {
            *col = purple;
         }
         return 3;
      } else {
         if(z==0 || x==0 || x==WORLDX-4) {
            return 0;
         }
         if(world[x+2][(int)(WORLDY/2)][z-2] == purple) {
            *col = empty;
         } else {
            *col = purple;
         }
         return 4;
      }
   }
   return 0;
}


	/******* draw2D() *******/
	/* draws 2D shapes on screen */
	/* use the following functions: 			*/
	/*	draw2Dline(int, int, int, int, int);		*/
	/*	draw2Dbox(int, int, int, int);			*/
	/*	draw2Dtriangle(int, int, int, int, int, int);	*/
	/*	set2Dcolour(float []); 				*/
	/* colour must be set before other functions are called	*/
void draw2D() {
   if (!testWorld) {
      int i,j;
      int xdis = screenWidth-WORLDX;
      int ydis = screenHeight-WORLDZ;
      int smallestRes = 100;
      int mapInflation = 1;

      if(hasKey) {
         set2Dcolour(rgb(white));
         for(i=0; i<11; i++) {
            for(j=0; j<3; j++) {
               if(key[i][j]) {
                  draw2Dbox(i*10+10,j*10+10,(i+1)*10+10,(j+1)*10+10);
               }
            }
         }
      }

	   switch(getMapState()) {
         case 0:
            return;
         case 1:
            for(i=0; i<WORLDX; i++) {
               for(j=0; j<WORLDZ; j++) {
                  set2Dcolour(rgb(map[i][j]));
                  draw2Dbox(j+xdis,i+ydis,j+1+xdis,i+1+ydis);
               }
            }
            break;
         case 2:
            if(screenHeight > screenWidth) {
               smallestRes = screenWidth;
            } else {
               smallestRes = screenHeight;
            }
            mapInflation = (int)((float)smallestRes/(float)WORLDZ);

            xdis = (screenWidth-(WORLDX*mapInflation))/2;
            ydis = (screenHeight-(WORLDZ*mapInflation))/2;
            
            for(i=0; i<WORLDX; i++) {
               for(j=0; j<WORLDZ; j++) {
                  set2Dcolour(rgb(map[i][j]));
                  draw2Dbox((j*mapInflation)+xdis,(i*mapInflation)+ydis,((j+1)*mapInflation)+xdis,((i+1)*mapInflation)+ydis);
               }
            }
            break;
      }
      if(shot) {
         set2Dcolour(rgb(black));
         for(i=0; i<screenWidth; i++) {
            for(j=0; j<screenHeight; j++) {
               if(rand()%20 == 0) {
                  draw2Dbox(i,j,i+(rand()%16),j+(rand()%16));
               }
            }
         }
         shot--;
      }
   }
}


	/*** update() ***/
	/* background process, it is called when there are no other events */
	/* -used to control animations and perform calculations while the  */
	/*  system is running */
	/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
   int i, j, k;
   static clock_t gravity_timer = 0;
   static clock_t wall_timer = 0;
   static clock_t inner_wall = 0;
   static clock_t mob_timer = 0;
   static wall_chunk chunks[WORLDX+WORLDZ];
   static int num_chunks = 0;
   static clock_t projectile_timer = 0;
   static clock_t drop_timer = 0;
   static clock_t bounce_timer = 0;

	/* sample animation for the test world, don't remove this code */
	/* -demo of animating mobs */
   if (!testWorld) {
      //do stuff to update world
      srand(time(NULL));
      float x=0.0;
      float y=0.0;
      float z=0.0;

      if(gravity_timer == 0) {
         gravity_timer = clock();
      }

      if(bounce_timer == 0) {
         bounce_timer = clock();
      }

      if(wall_timer == 0) {
         wall_timer = clock();
      }

      if(inner_wall == 0) {
         inner_wall = clock();
      }

      if(projectile_timer == 0) {
         projectile_timer = clock();
      }

      if(mob_timer == 0) {
         mob_timer = clock();
      }
      
      if(drop_timer == 0) {
         drop_timer = clock();
      }

      getViewPosition(&x, &y, &z);

      time_t end = clock();

      if(bouncing && (float)(end - bounce_timer) / (float)CLOCKS_PER_SEC > 0.05) {
         int destX = dest[0], destZ = dest[1];
         float xRatio = 0.0, zRatio = 0.0;
         int xdir = 1, zdir = 1;
         if((int)fabs(x) < destX) {
            xRatio = (destX-fabs(x))/10.0;
         } else if((int)fabs(x) > destX) {
            xRatio = (fabs(x)-destX)/10.0;
            xdir = -1;
         }
         if((int)fabs(z) < destZ) {
            zRatio = (destZ-fabs(z))/10.0;
         } else if((int)fabs(z) > destZ) {
            zRatio = (fabs(z)-destZ)/10.0;
            zdir = -1;
         }

         if(xRatio < 1 && zRatio < 1) {
            bouncing = 0;
         }
         for(i=(int)fabs(x)-1; i<(int)fabs(x)+2; i++) {
            for(j=(int)fabs(z)-1; j<(int)fabs(z)+2; j++) {
               if(map[i][j] == white) {
                  map[i][j] = empty;
               }
            }
         }
         for(i=(int)fabs(x-(xRatio*xdir))-1; i<(int)fabs(x-(xRatio*xdir))+2; i++) {
            for(j=(int)fabs(z-(zRatio*zdir))-1; j<(int)fabs(z-(zRatio*zdir))+2; j++) {
               if(!map[i][j]) {
                  map[i][j] = white;
               }
            }
         }
         float yoff = sqrt(((xRatio*10)*(xRatio*10))+((zRatio*10)*(zRatio*10)));
         setViewPosition(x-(xRatio*xdir), -1*yoff, z-(zRatio*zdir));
         getViewPosition(&x, &y, &z);
         bounce_timer = end;
      }

      if((float)(end - projectile_timer) / (float)CLOCKS_PER_SEC > 0.01) {
         for(i=0; i<=n_enemies; i++) {
            if(isMobVisible(i)) {
               float mobx, moby, mobz, mobroty, mobrotx, mobrotz, difx, difz, dify;
               getMobPosition(i, &mobx, &moby, &mobz, &mobroty, &mobrotx, &mobrotz);
               while(mobroty >= 360.0) {
                  mobroty -= 360.0;
               }
               while(mobrotz >= 360.0) {
                  mobrotz -= 360.0;
               }
               while(mobrotx >= 360.0) {
                  mobrotx -= 360.0;
               }

               difz = fabs(cos(radian(mobroty)));
               difx = fabs(sin(radian(mobroty)));
               dify = fabs(sin(radian(mobrotx)));

               if(mobrotx > 0 && mobrotx < 180) {
                  dify = -1.0*dify;
               }
               
               if(mobroty > 180 && mobroty <= 270) {
                  difx = -1.0*difx;
               } else if(mobroty > 270 && mobroty < 360) {
                  difz = -1.0*difz;
                  difx = -1.0*difx;
               } else if(mobroty >= 0 && mobroty < 90) {
                  difz = -1.0*difz;
               }
               if(map[(int)round(mobx)][(int)round(mobz)] == yellow) {
                  map[(int)round(mobx)][(int)round(mobz)] = empty;
               }
               if(mobx+difx > WORLDX || mobx+difx < 0 || mobz+difz > WORLDZ || mobz+difz < 0 || moby+dify > WORLDY || moby+dify < 0) {
                  hideMob(i);
               } else {
                  setMobPosition(i, mobx+difx, moby+dify, mobz+difz, mobroty);
                  switch(world[(int)round(mobx+difx)][(int)round(moby+dify)][(int)round(mobz+difz)]) {
                     case blue:
                     case black:
                        hideMob(i);
                        break;
                     case empty:
                        if(map[(int)round(mobx+difx)][(int)round(mobz+difz)] == empty) {
                           map[(int)round(mobx+difx)][(int)round(mobz+difz)] = yellow;
                        } else if(map[(int)round(mobx+difx)][(int)round(mobz+difz)] == white) {
                           if(round(mobx+difx) == round(fabs(x)) && round(mobz+difz) == round(fabs(z))) {
                              if((moby+dify) < fabs(y)+1.0 && (moby+dify) > fabs(y)-1.0) {
                                 printf("You've been shot!\n");
                                 shot = 12;
                                 hideMob(i);
                              }
                           }
                        }
                        break;
                     default:
                        if((int)round(mobx+difx) > 3 && (int)round(mobx+difx) < WORLDX-3 && (int)round(mobz+difz) > 3 && (int)round(mobz+difz) < WORLDZ-3 && world[(int)round(mobx+difx)][(int)round(moby+dify)][(int)round(mobz+difz)] != white) {
                           world[(int)round(mobx+difx)][(int)round(moby+dify)][(int)round(mobz+difz)] = empty;
                           map[(int)round(mobx+difx)][(int)round(mobz+difz)] = empty;
                        }
                        hideMob(i);
                  }
               }
            }
         }
      }

      if((float)(end - gravity_timer) / (float)CLOCKS_PER_SEC > 0.02 && !bouncing) {
         if(abs((int)y)-1 >= WORLDY || world[abs((int)x)][abs((int)y)-1][abs((int)z)] == 0) {
            setViewPosition(x, y+1, z);
            gravity_timer = end;
         }
         if(y>-1.8) {
            setViewPosition(x, -1.8, z);
         }
      }

      if((float)(end - wall_timer) / (float)CLOCKS_PER_SEC > 5.0) {
         int w=0;

         for(w=0; w<WORLDX; w++) {
            int l=0;
            for(l=0; l<WORLDZ; l++) {
               if(w%16==0 && l%16==0) {
                  wall_chunk chunk;

                  chunk.x = w;
                  chunk.z = l;
                  chunk.dir = randomWall(rand(), w, l, &(chunk.col));

                  if(chunk.dir > 0) {
                     chunks[num_chunks] = chunk;
                     num_chunks++;
                  }
               }
            }
         }
         wall_moving = 1;
         wall_timer = end;
      }

      if(wall_moving && (float)(end - inner_wall) / (float)CLOCKS_PER_SEC > 0.08) {
         float xpos, ypos, zpos;
         
         for(i=0; i<num_chunks; i++) {
            enemy* mob = NULL;
            switch(chunks[i].dir) {
               case 1: //north
                     for(j=chunks[i].z+1; j<chunks[i].z+3; j++) {
                        for(k=1; k<WORLDY; k++) {
                           getViewPosition(&xpos, &ypos, &zpos);
                           if(abs((int)xpos) == (chunks[i].x+15)-ind &&
                              abs((int)ypos) == k && abs((int)zpos) == j) {
                              if(j==chunks[i].z+1) {
                                 setViewPosition(xpos, ypos, zpos+1.25);
                              } else {
                                 setViewPosition(xpos, ypos, zpos-1.25);
                              }
                           }
                           if((mob = getEnemy((chunks[i].x+15)-ind, k, j))) {
                              eraseEnemy(*mob);
                              if(j==chunks[i].z+1) {
                                 mob->z = mob->z - mob->size;
                              } else {
                                 mob->z = mob->z + mob->size;
                              }
                              mob->dest_x = mob->x;
                              mob->dest_z = mob->z;
                           }
                           world[(chunks[i].x+15)-ind][k][j] = chunks[i].col;
                        }
                        map[(chunks[i].x+15)-ind][j] = chunks[i].col;
                     }
                  break;
               case 2: //east
                     for(j=chunks[i].x+1; j<chunks[i].x+3; j++) {
                        for(k=1; k<WORLDY; k++) {
                           getViewPosition(&xpos, &ypos, &zpos);
                           if(abs((int)xpos) == j && abs((int)ypos) == k && 
                              abs((int)zpos) == (chunks[i].z+15)-ind) {
                              if(j==chunks[i].x+1) {
                                 setViewPosition(xpos+1.25, ypos, zpos);
                              } else {
                                 setViewPosition(xpos-1.25, ypos, zpos);
                              }
                           }
                           if((mob = getEnemy(j, k, (chunks[i].z+15)-ind))) {
                              eraseEnemy(*mob);
                              if(j==chunks[i].x+1) {
                                 mob->x = mob->x - mob->size;
                              } else {
                                 mob->x = mob->x + mob->size;
                              }
                              mob->dest_x = mob->x;
                              mob->dest_z = mob->z;
                           }
                           world[j][k][(chunks[i].z+15)-ind] = chunks[i].col;
                        }
                        map[j][(chunks[i].z+15)-ind] = chunks[i].col;
                     }
                  break;
               case 3: //south
                     for(j=chunks[i].z+1; j<chunks[i].z+3; j++) {
                        for(k=1; k<WORLDY; k++) {
                           getViewPosition(&xpos, &ypos, &zpos);
                           if(abs((int)xpos) == (chunks[i].x-1)-ind &&
                              abs((int)ypos) == k && abs((int)zpos) == j) {
                              if(j==chunks[i].z+1) {
                                 setViewPosition(xpos, ypos, zpos+1.25);
                              } else {
                                 setViewPosition(xpos, ypos, zpos-1.25);
                              }
                           }
                           if((mob = getEnemy((chunks[i].x-1)-ind, k, j))) {
                              eraseEnemy(*mob);
                              if(j==chunks[i].z+1) {
                                 mob->z = mob->z - mob->size;
                              } else {
                                 mob->z = mob->z + mob->size;
                              }
                              mob->dest_x = mob->x;
                              mob->dest_z = mob->z;
                           }
                           world[(chunks[i].x-1)-ind][k][j] = chunks[i].col;
                        }
                        map[(chunks[i].x-1)-ind][j] = chunks[i].col;
                     }
                  break;
               case 4: //west
                     for(j=chunks[i].x+1; j<chunks[i].x+3; j++) {
                        for(k=1; k<WORLDY; k++) {
                           getViewPosition(&xpos, &ypos, &zpos);
                           if(abs((int)xpos) == k && abs((int)ypos) == k && 
                              abs((int)zpos) == (chunks[i].z-1)-ind) {
                              if(j==chunks[i].x+1) {
                                 setViewPosition(xpos+1.25, ypos, zpos);
                              } else {
                                 setViewPosition(xpos-1.25, ypos, zpos);
                              }
                           }
                           if((mob = getEnemy(j, k, (chunks[i].z-1)-ind))) {
                              eraseEnemy(*mob);
                              if(j==chunks[i].x+1) {
                                 mob->x = mob->x - mob->size;
                              } else {
                                 mob->x = mob->x + mob->size;
                              }
                              mob->dest_x = mob->x;
                              mob->dest_z = mob->z;
                           }
                           world[j][k][(chunks[i].z-1)-ind] = chunks[i].col;
                        }
                        map[j][(chunks[i].z-1)-ind] = chunks[i].col;
                     }
                  break;
            }
         }
         ind++;
         if(ind == 12) {
            ind = 0;
            wall_moving = 0;
            num_chunks = 0;
         }
         inner_wall = end;
      }

      if((float)(end - mob_timer) / (float)CLOCKS_PER_SEC > 0.1) {
         for(i=0; i<n_enemies; i++) {
            eraseEnemy(enemies[i]);
            behaveEnemy(&enemies[i], rand());
            animateEnemy(&enemies[i]);
         }
         mob_timer = end;
      }

      if((float)(end - drop_timer) / (float)CLOCKS_PER_SEC > 0.1 && dropParty) {
         if(dropBox[2] > 1) {
            dropBox[2] = dropBox[2]-1;

            if(world[dropBox[0]-3][dropBox[2]][dropBox[1]-3] == empty) {
               colour col = (rand()%3)+1;
               map[dropBox[0]-3][dropBox[1]-3] = col;
               world[dropBox[0]-3][dropBox[2]][dropBox[1]-3] = col;
               world[dropBox[0]-3][dropBox[2]+1][dropBox[1]-3] = empty;
            }
            if(world[dropBox[0]+3][dropBox[2]][dropBox[1]-3] == empty) {
               colour col = (rand()%3)+1;
               map[dropBox[0]+3][dropBox[1]-3] = col;
               world[dropBox[0]+3][dropBox[2]][dropBox[1]-3] = col;
               world[dropBox[0]+3][dropBox[2]+1][dropBox[1]-3] = empty;
            }
            if(world[dropBox[0]-3][dropBox[2]][dropBox[1]+3] == empty) {
               colour col = (rand()%3)+1;
               map[dropBox[0]-3][dropBox[1]+3] = col;
               world[dropBox[0]-3][dropBox[2]][dropBox[1]+3] = col;
               world[dropBox[0]-3][dropBox[2]+1][dropBox[1]+3] = empty;
            }
            if(world[dropBox[0]+3][dropBox[2]][dropBox[1]+3] == empty) {
               colour col = (rand()%3)+1;
               map[dropBox[0]+3][dropBox[1]+3] = col;
               world[dropBox[0]+3][dropBox[2]][dropBox[1]+3] = col;
               world[dropBox[0]+3][dropBox[2]+1][dropBox[1]+3] = empty;
            }
         } else {
            dropParty = 0;
         }
         drop_timer = end;
      }
   }
}


	/* called by GLUT when a mouse button is pressed or released */
	/* -button indicates which button was pressed or released */
	/* -state indicates a button down or button up event */
	/* -x,y are the screen coordinates when the mouse is pressed or */
	/*  released */ 
void mouse(int button, int state, int x, int y) {
   float xpos, ypos, zpos;
   float xrot, yrot, zrot;
   float mobx, moby, mobz, mobroty, mobrotx, mobrotz;

   getViewPosition(&xpos, &ypos, &zpos);
   getViewOrientation(&xrot, &yrot, &zrot);
   getMobPosition(0, &mobx, &moby, &mobz, &mobroty, &mobrotx, &mobrotz);
   
   if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
      map[(int)(mobx)][(int)(mobz)] = empty;
      createMob(0, fabs(xpos), fabs(ypos), fabs(zpos), yrot);
      mobTrajectory(0, xrot, zrot);
      showMob(0);
   } else if (button == GLUT_MIDDLE_BUTTON) {
   } else {
   }
   if (state == GLUT_UP) {
   } else {
   }
}

void startLevel() {
   int x=0;
   int h=WORLDY;
   int l=0, randX, randZ;
   int i,j,k, keySet = 0;

   level++;

   for(x=0; x<WORLDX; x++) {
      int z=0;

      for(z=0; z<WORLDZ; z++) {
         int y=0;

         for(y=0; y<WORLDY; y++) {
            world[x][y][z] = 0;
         }
      }
   }

   for(l=0; l<WORLDX; l++) {
      int y=0;
      for(y=0; y<h; y++) {
         int i=0;
         for (i=1; i<3; i++) {
            world[l][y][i] = purple;
            world[l][y][WORLDZ-(i+1)] = purple;
            map[l][i] = purple;
            map[l][WORLDZ-(i+1)] = purple;
         }
      }
   }
   for(l=0; l<WORLDZ; l++) {
      int y=0;
      for(y=0; y<h; y++) {
         int i=0;
         for (i=1; i<3; i++) {
            world[i][y][l] = purple;
            world[WORLDX-(i+1)][y][l] = purple;
            map[i][l] = purple;
            map[WORLDX-(i+1)][l] = purple;
         }
      }
   }
   for(x=0; x<WORLDX; x++) {
      int z=0;
      colour col;
      for(z=0; z<WORLDZ; z++) {
         world[x][0][z] = black;
         if(x%16==0 && z%16==0) {
            int y=0;
            int wall = randomWall(rand(), x, z, &col);
               
            for(y=1; y<h; y++) {
               int i=0;
               for(i=0; i<4; i++) {
                  int j=0;
                  for(j=0; j<4; j++) {
                     world[x+i][y][z+j] = blue;
                     map[x+i][z+j] = blue;
                  }
               }
            }
            switch(wall) {
               int i=0;
               int j=0;
               int k=0;

               case 1: //north
                  for(i=z+4; i<z+16; i++) {
                     for(j=x+1; j<x+3; j++) {
                        map[j][i] = purple;
                        for(k=1; k<h; k++) {
                           world[j][k][i] = purple;
                        }
                     }
                  }
                  break;
               case 2: //east
                  for(i=z+1; i<z+3; i++) {
                     for(j=x+4; j<x+16; j++) {
                        map[j][i] = purple;
                        for(k=1; k<h; k++) {
                           world[j][k][i] = purple;
                        }
                     }
                  }
                  break;
               case 3: //south
                  for(i=z-1; i>=z-12; i--) {
                     for(j=x+1; j<x+3; j++) {
                        map[j][i] = purple;
                        for(k=1; k<h; k++) {
                           world[j][k][i] = purple;
                        }
                     }
                  }
                  break;
               case 4: //west
                  for(i=z+1; i<z+3; i++) {
                     for(j=x-1; j>=x-12; j--) {
                        map[j][i] = purple;
                        for(k=1; k<h; k++) {
                           world[j][k][i] = purple;
                        }
                     }
                  }
                  break;
            }
         } else if((x+7)%16==0 && (z+7)%16==0 && x > 16 && z > 16) {
            if(rand() % 3 == 0 && n_enemies < MOB_COUNT-1) {
               char type = 'x';
               int size = 3;
               if(rand()%2 == 0) {
                  type = 'o';
               }
               n_enemies++;
               enemies = realloc(enemies, sizeof(enemy)*n_enemies);
               enemies[n_enemies-1] = initEnemy(type, size, x, z, 0, rand());

               for(i=0; i<size; i++) {
                  for(j=0; j<size; j++) {
                     for(k=0; k<size; k++) {
                        enemies[n_enemies-1].form[i][j][k] = empty;
                     }
                  }
               }

               if(type == 'o') {
                  enemies[n_enemies-1].form[0][0][0] = yellow;
                  enemies[n_enemies-1].form[2][2][2] = yellow;
                  enemies[n_enemies-1].form[1][0][1] = yellow;
                  enemies[n_enemies-1].form[1][2][1] = yellow;
                  enemies[n_enemies-1].form[0][1][0] = yellow;
                  enemies[n_enemies-1].form[0][1][1] = yellow;
                  enemies[n_enemies-1].form[0][1][0] = yellow;
                  enemies[n_enemies-1].form[1][1][0] = yellow;
                  enemies[n_enemies-1].form[0][1][2] = yellow;
                  enemies[n_enemies-1].form[2][1][2] = yellow;
                  enemies[n_enemies-1].form[2][1][0] = yellow;
               } else if(type == 'x') {
                  for(i=0; i<size; i++) {
                     enemies[n_enemies-1].form[i][i][i] = orange;
                     enemies[n_enemies-1].form[i][i][size-1-i] = orange;
                     enemies[n_enemies-1].form[size-1-i][i][i] = orange;
                     enemies[n_enemies-1].form[i][size-1-i][i] = orange;
                  }
                  enemies[n_enemies-1].form[1][1][0] = orange;
                  enemies[n_enemies-1].form[1][1][2] = orange;
                  enemies[n_enemies-1].form[2][1][1] = orange;
                  enemies[n_enemies-1].form[0][1][1] = orange;
               }
            }
         }
      }
      
      setViewPosition(-10.0, -1.8, -10.0);
      setViewPosition(-10.0, -1.8, -10.0);
      setViewOrientation(0.0, 90.0, 0.0);
      for(i=9; i<12; i++) {
         for(j=9; j<12; j++) {
            map[i][j] = white;
         }
      }
      for(i=0; i<n_enemies; i++) {
         animateEnemy(&enemies[i]);
      }
      for(i=5; i<WORLDX-4; i++) {
         for(j=5; j<WORLDZ-4; j++) {
            if(!map[i][j] && rand()%20000 == 0) {
               colour col = (rand()%3)+1;
               map[i][j] = col;
               world[i][1][j] = col;
            }
         }
      }
      while(!keySet) {
         randX = (rand()%(WORLDX-10))+5;
         randZ = (rand()%(WORLDZ-10))+5;
         keySet = (map[randX][randZ] == empty);
      }
      world[randX][1][randZ] = white;
      map[randX][randZ] = white;
      for(i=7; i<13; i++) {
         for(j=1; j<WORLDY/2; j++) {
            world[i][j][WORLDZ-3] = white;
         }
         map[i][WORLDZ-3] = white;
      }
   }
}

void endLevel() {
   hideMob(0);
   for(int i=0; i<n_enemies; i++) {
      destroyEnemy(enemies[i]);
      hideMob(i+1);
   }
   free(enemies);
   enemies = NULL;
   n_enemies = 0;

   initEnemy('E', -1, -1, -1, -1, -1);

   for(int i=0; i<WORLDX; i++) {
      for(int j=0; j<WORLDZ; j++) {
         map[i][j] = empty;
      }
   }
   hasKey = 0;
   dropParty = 0;
   bouncing = 0;
   shot = 0;
   wall_moving = 0;
   ind = 0;
}

int main(int argc, char** argv)
{
   int i;
	/* initialize the graphics system */
   graphicsInit(&argc, argv);

	/* the first part of this if statement builds a sample */
	/* world which will be used for testing */
	/* DO NOT remove this code. */
	/* Put your code in the else statment below */
	/* The testworld is only guaranteed to work with a world of
		with dimensions of 100,50,100. */
   if (!testWorld) {
      srand(time(NULL));
      for(i=0; i<3; i++) {
         for(int j=0; j<3; j+=2) {
            key[i][j] = white;
         }
      }
      for(i=0; i<11; i++) {
         key[i][1] = white;
      }
      key[1][1] = empty;
      key[10][2] = white;
      key[7][2] = white;

      startLevel();


   	/* starts the graphics processing loop */
   	/* code after this will not run until the program exits */
      glutMainLoop();
   }

   return 0; 
}

