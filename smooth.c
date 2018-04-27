/*  
    smooth.c
    Nate Robins, 1998

    Model viewer program.  Exercises the glm library.
*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#ifdef __APPLE__ 
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "gltb.h"
#include "glm.h"
#include "dirent32.h"

#pragma comment( linker, "/entry:\"mainCRTStartup\"" )  // set the entry point to be main()

#define DATA_DIR "data/"

char*      model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		    /* display list for object */
GLMmodel*  model;			        /* glm model data structure */
GLfloat    scale;			        /* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint	   ascii = 0;				/* toggle ascii effect*/
GLuint	   blurring = 0;
GLboolean  cel_shading = GL_FALSE;
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			    /* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;

static GLfloat pixels[512][512][3];


#if defined(_WIN32)
#include <sys/timeb.h>
#define CLK_TCK 1000
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#endif
float
elapsed(void)
{
    static long begin = 0;
    static long finish, difference;
    
#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*1000+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif
    
    difference = finish - begin;
    begin = finish;
    
    return (float)difference/(float)CLK_TCK;
}

void
shadowtext(int x, int y, char* s) 
{
    int lines;
    char* p;
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
        0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x+1, y-1);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x+1, y-1-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(0, 128, 255);
    glRasterPos2i(x, y);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x, y-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void
lists(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat shininess = 65.0;
    
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    
    if (model_list)
        glDeleteLists(model_list, 1);
    
    /* generate a list */
    if (material_mode == 0) { 
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT);
        else
            model_list = glmList(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT | GLM_COLOR);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
        if (facet_normal)
            model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
    }
}

void
init(void)
{
    gltbInit(GLUT_LEFT_BUTTON);
    
    /* read in the model */
    model = glmReadOBJ(model_file);
    scale = glmUnitize(model);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);
    
    if (model->nummaterials > 0)
        material_mode = 2;
    
    /* create new display lists */
    lists();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    
    glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_CULL_FACE);
}

void
reshape(int width, int height)
{
    gltbReshape(width, height);
    
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.0);
}

GLfloat blurR(int i, int j, GLfloat array[512][512][3], GLfloat filter[3][3])
{
	GLfloat avg = 0;

	avg = avg + (filter[0][0])*(array[i - 1][j - 1][0]);	//1
	avg = avg + (filter[0][1])*(array[i - 1][j][0]);		//2
	avg = avg + (filter[0][2])*(array[i - 1][j + 1][0]);	//3
	avg = avg + (filter[1][0])*(array[i][j - 1][0]);		//4
	avg = avg + (filter[1][1])*(array[i][j][0]);			//5
	avg = avg + (filter[1][2])*(array[i][j + 1][0]);		//6
	avg = avg + (filter[2][0])*(array[i + 1][j - 1][0]);	//7
	avg = avg + (filter[2][1])*(array[i + 1][j][0]);		//8
	avg = avg + (filter[2][2])*(array[i + 1][j + 1][0]);	//9
	return avg;
}

GLfloat blurG(int i, int j, GLfloat array[512][512][3], GLfloat filter[3][3])
{
	GLfloat avg = 0;

	avg = avg + (filter[0][0])*(array[i - 1][j - 1][1]);	//1
	avg = avg + (filter[0][1])*(array[i - 1][j][1]);		//2
	avg = avg + (filter[0][2])*(array[i - 1][j + 1][1]);	//3
	avg = avg + (filter[1][0])*(array[i][j - 1][1]);		//4
	avg = avg + (filter[1][1])*(array[i][j][1]);			//5
	avg = avg + (filter[1][2])*(array[i][j + 1][1]);		//6
	avg = avg + (filter[2][0])*(array[i + 1][j - 1][1]);	//7
	avg = avg + (filter[2][1])*(array[i + 1][j][1]);		//8
	avg = avg + (filter[2][2])*(array[i + 1][j + 1][1]);	//9
	return avg;
}

GLfloat blurB(int i, int j, GLfloat array[512][512][3], GLfloat filter[3][3])
{
	GLfloat avg = 0;

	avg = avg + (filter[0][0])*(array[i - 1][j - 1][2]);	//1
	avg = avg + (filter[0][1])*(array[i - 1][j][2]);		//2
	avg = avg + (filter[0][2])*(array[i - 1][j + 1][2]);	//3
	avg = avg + (filter[1][0])*(array[i][j - 1][2]);		//4
	avg = avg + (filter[1][1])*(array[i][j][2]);			//5
	avg = avg + (filter[1][2])*(array[i][j + 1][2]);		//6
	avg = avg + (filter[2][0])*(array[i + 1][j - 1][2]);	//7
	avg = avg + (filter[2][1])*(array[i + 1][j][2]);		//8
	avg = avg + (filter[2][2])*(array[i + 1][j + 1][2]);	//9
	return avg;
}

void blurringPostProcess() {
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	int width = glutGet(GLUT_WINDOW_WIDTH);

	GLfloat boxFilter[3][3] = { { 0.111f, 0.111f, 0.111f },{ 0.111f, 0.111f, 0.111f },{ 0.111f, 0.111f, 0.111f } };
	//GLfloat boxFilter[3][3] = { { 0.1f, 0.1f, 0.1f },{ 0.1f, 0.1f, 0.1f },{ 0.1f, 0.1f, 0.1f } };
	GLfloat gaussianFilter[3][3] = { { 0.0625f, 0.125f, 0.0625f },{ 0.125f, 0.25f, 0.125f },{ 0.0625f, 0.125f, 0.0625f } };

	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, pixels);

	GLfloat * filter = NULL;
	if (blurring == 1) {
		filter = boxFilter;
	}
	else if (blurring == 2) {
		filter = gaussianFilter;
	}

	for (int x = 1; x < 511; x++)
	{
		for (int y = 1; y < 511; y++)
		{
			GLfloat value[3] = { blurR(x, y, pixels, filter), blurG(x, y, pixels, filter), blurB(x, y, pixels, filter) };

			pixels[x][y][0] = value[0];		// red		(channel 0)
			pixels[x][y][1] = value[1];		// green	(channel 1)
			pixels[x][y][2] = value[2];		// blue		(channel 2)			
		}
	}

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, pixels);
}

void cel_shade_post_process()
{
    //glClearColor(1.0, 1.0, 1.0, 0.0);
    
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    int width = glutGet(GLUT_WINDOW_WIDTH);
    static GLfloat pixels[512][512][3];
    static GLfloat zbuffer[512][512];
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, zbuffer);
    glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, pixels);

    for(int i = 0; i < width; i ++)
    {
        for(int j = 0; j < height; j ++)
        {
            GLfloat r, g, b;
            GLfloat cr, cg, cb;
            
			r = pixels[i][j][0];
			g = pixels[i][j][1];
			b = pixels[i][j][2];

			GLfloat greyscale = (r + g + b) / 3;

			if (greyscale > 0.73) {
                r = 1.0;
                g = 1.0;
                b = 1.0;
			}
			else if (greyscale < 0.73 && greyscale >= 0.35){
				r = r + 0.15;
				g = g + 0.15;
				b = b + 0.15;
			}
            else if (greyscale < 0.35){
				r = 0.0;
				g = 0.0;
				b = 0.0;
			}

			pixels[i][j][0] = r;
			pixels[i][j][1] = g;
			pixels[i][j][2] = b;
        }
    }

    // Draw the outline
    for(int i = 0; i < width; ++i) 
    {
        for(int j = 0; j < height; ++j)
        {

            GLfloat z = zbuffer[i][j];
            GLfloat zavg = (z + zbuffer[i+1][j] + zbuffer[i][j+1] + zbuffer[i-1][j] + zbuffer[i][j-1]) / 5;
            if(zavg > 0.76f && zavg != 1.0f) 
            {
                pixels[i][j][0] = 0.0f;
			    pixels[i][j][1] = 0.0f;
			    pixels[i][j][2] = 0.0f;
                
                pixels[i+1][j][0] = 0.0f;
			    pixels[i+1][j][1] = 0.0f;
			    pixels[i+1][j][2] = 0.0f;

                pixels[i][j+1][0] = 0.0f;
			    pixels[i][j+1][1] = 0.0f;
			    pixels[i][j+1][2] = 0.0f;

                pixels[i-1][j][0] = 0.0f;
			    pixels[i-1][j][1] = 0.0f;
			    pixels[i-1][j][2] = 0.0f;

                pixels[i][j-1][0] = 0.0f;
			    pixels[i][j-1][1] = 0.0f;
			    pixels[i][j-1][2] = 0.0f;
                
                pixels[i+1][j+1][0] = 0.0f;
			    pixels[i+1][j+1][1] = 0.0f;
			    pixels[i+1][j+1][2] = 0.0f;

                pixels[i+1][j-1][0] = 0.0f;
			    pixels[i+1][j-1][1] = 0.0f;
			    pixels[i+1][j-1][2] = 0.0f;

                pixels[i-1][j+1][0] = 0.0f;
			    pixels[i-1][j+1][1] = 0.0f;
			    pixels[i-1][j+1][2] = 0.0f;

                pixels[i-1][j-1][0] = 0.0f;
			    pixels[i-1][j-1][1] = 0.0f;
			    pixels[i-1][j-1][2] = 0.0f;
            }
        }
    }

    glDrawPixels(width, height, GL_RGB, GL_FLOAT, pixels);

}

void asciiPostProcess() {
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	int width = glutGet(GLUT_WINDOW_WIDTH);
	//TODO dynamically change pixel buffer size to window size
	//GLfloat* pixels = (GLfloat*) malloc(width * height * 3 * sizeof(GLfloat));
	
	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, pixels); //get frame buffer, hardcoded to 512 by 512 for now
																 //scan pixels in 8x8 square
	for (int i = 0; i < width; i = i + 8) {
		for (int j = 0; j < height; j = j + 8) {
			//average rgb components separately
			GLfloat r_avg = 0.f;
			GLfloat g_avg = 0.f;
			GLfloat b_avg = 0.f;
			for (int k = 0; k < 8; k++) {
				for (int l = 0; l < 8; l++) {
					//discard pixel in averaging if completely black (or white?)
					r_avg += pixels[i + k][j + l][0];
					g_avg += pixels[i + k][j + l][1];
					b_avg += pixels[i + k][j + l][2];
				}
			}

			r_avg = r_avg / 64;
			g_avg = g_avg / 64;
			b_avg = b_avg / 64;

			GLfloat greyscale = (r_avg + g_avg + b_avg) / 3;

			//replace 8x8 square with # if not pure black or white for now
			if (greyscale != 1.f) {

				//poor man's bitmap of . symbol
				// 1/64 = 0.015
				// 1/49 = 0.02
				const static GLboolean period[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				//poor man's bitmap of : symbol
				// 4/64 = 0.0625
				// 4/49 = 0.082
				const static GLboolean colon[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				//poor man's bitmap of n symbol
				// 8/64 = 0.125 
				// 8/49 = 0.163
				const static GLboolean n[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 0, 1, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				//poor man's bitmap of 8 symbol
				// 12/64 = 0.19
				// 12/49 = 0.245
				const static GLboolean eight[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 1, 1, 0, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 0, 1, 1, 0, 0, 0 },
				{ 0, 0, 0, 1, 1, 0, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 0, 1, 1, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				//poor man's bitmap of * symbol
				// 17/64 = 0.27
				// 17/49 = 0.347
				const static GLboolean star[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 1, 0, 1, 0, 1, 0 },
				{ 0, 0, 0, 1, 1, 1, 0, 0 },
				{ 0, 0, 1, 1, 1, 1, 1, 0 },
				{ 0, 0, 0, 1, 1, 1, 0, 0 },
				{ 0, 0, 1, 0, 1, 0, 1, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				//poor man's bitmap of # symbol
				// 20/64 = 0.31
				// 20/49 = 0.408
				const static GLboolean pound[8][8] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 1, 1, 1, 1, 1, 1, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 1, 1, 1, 1, 1, 1, 0 },
				{ 0, 0, 1, 0, 0, 1, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0 }
				};

				GLboolean(*character)[8][8];

				//pick character based off greyscale intensity
				if (greyscale > 0.4) {
					character = &pound;
				}
				else if (greyscale > 0.35) {
					character = &star;
				}
				else if (greyscale > 0.25) {
					character = &eight;
				}
				else if (greyscale > 0.15) {
					character = &n;
				}
				else if (greyscale > 0.1) {
					character = &colon;
				}
				else {
					character = &period;
				}

				for (int k = 0; k < 8; k++) {
					for (int l = 0; l < 8; l++) {
						if ((*character)[k][l] == 1) {
							pixels[i + k][j + l][0] = r_avg;
							pixels[i + k][j + l][1] = g_avg;
							pixels[i + k][j + l][2] = b_avg;
						}
						else {
							pixels[i + k][j + l][0] = 0.f;
							pixels[i + k][j + l][1] = 0.f;
							pixels[i + k][j + l][2] = 0.f;
						}
					}
				}
			}
		}
	}

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, pixels);
}

void asciiCharacterMode() {
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	int width = glutGet(GLUT_WINDOW_WIDTH);

	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, pixels); //get frame buffer, hardcoded to 512 by 512 for now

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//using 8 by 13 bitmap characters
	for (int i = 0; i < width; i = i + 8) {
		for (int j = 0; j < height - 13; j = j + 13) {
			//average rgb components separately
			GLfloat r_avg = 0.f;
			GLfloat g_avg = 0.f;
			GLfloat b_avg = 0.f;
			for (int k = 0; k < 8; k++) {
				for (int l = 0; l < 13; l++) {
					//discard pixel in averaging if completely black (or white?)
					r_avg += pixels[i + k][j + l][0];
					g_avg += pixels[i + k][j + l][1];
					b_avg += pixels[i + k][j + l][2];
				}
			}

			r_avg = r_avg / 104;
			g_avg = g_avg / 104;
			b_avg = b_avg / 104;
			glColor3f(r_avg, g_avg, b_avg);

			GLfloat greyscale = (r_avg + g_avg + b_avg) / 3;

			glRasterPos2f(j, i);

			if (greyscale > 0.5) {
				//draw # character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '#');
			}
			else if (greyscale > 0.35) {
				//draw * character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '*');
			}
			else if (greyscale > 0.25) {
				//draw 8 character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '8');
			}
			else if (greyscale > 0.15) {
				//draw n character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, 'n');
			}
			else if (greyscale > 0.1) {
				//draw : character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ':');
			}
			else if (greyscale > 0) {
				//draw . character
				glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '.');
			}
		}
	}

	glRasterPos2i(0, 0); //reset raster pos, or else drawpixels will be off later
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

#define NUM_FRAMES 5
void
display(void)
{
    static char s[256], t[32];
    static char* p;
    static int frames = 0;
    
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix();
    
    glTranslatef(pan_x, pan_y, 0.0);
    
    gltbMatrix();
    
#if 0   /* glmDraw() performance test */
    if (material_mode == 0) { 
        if (facet_normal)
            glmDraw(model, GLM_FLAT);
        else
            glmDraw(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
        if (facet_normal)
            glmDraw(model, GLM_FLAT | GLM_COLOR);
        else
            glmDraw(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
        if (facet_normal)
            glmDraw(model, GLM_FLAT | GLM_MATERIAL);
        else
            glmDraw(model, GLM_SMOOTH | GLM_MATERIAL);
    }
#else
    glCallList(model_list);
#endif

    if (cel_shading)
    {
        cel_shade_post_process();
    }
    
	/* post-processing ascii affect*/
	if (ascii == 1) {
		asciiPostProcess();
	}

	if (blurring) {
		blurringPostProcess();
	}

    glDisable(GL_LIGHTING);
    if (bounding_box) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glColor4f(1.0, 0.0, 0.0, 0.25);
        glutSolidCube(2.0);
        glDisable(GL_BLEND);
    }
    
    glPopMatrix();

	if (ascii == 2) {
		asciiCharacterMode();
	}
	
	
    if (stats) {
        /* XXX - this could be done a _whole lot_ faster... */
		/*
        int height = glutGet(GLUT_WINDOW_HEIGHT);
        glColor3ub(0, 0, 0);
        sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
            "%d texcoords\n%d groups\n%d materials",
            model->pathname, model->numvertices, model->numtriangles, 
            model->numnormals, model->numtexcoords, model->numgroups,
            model->nummaterials);
        shadowtext(5, height-(5+18*1), s);
		*/
    }
	

    /* spit out frame rate. */
	/*
    frames++;
    if (frames > NUM_FRAMES) {
        sprintf(t, "%g fps", frames/elapsed());
        frames = 0;
    }
    if (performance) {
        shadowtext(5, 5, t);
    }
	*/

    
    glutSwapBuffers();
    glEnable(GL_LIGHTING);
}

void
keyboard(unsigned char key, int x, int y)
{
    GLint params[2];
    
    switch (key) {
    case 'h':
        printf("help\n\n");
        printf("w         -  Toggle wireframe/filled\n");
        printf("c         -  Toggle culling\n");
        printf("n         -  Toggle facet/smooth normal\n");
        printf("b         -  Toggle bounding box\n");
        printf("r         -  Reverse polygon winding\n");
        printf("m         -  Toggle color/material/none mode\n");
        printf("p         -  Toggle performance indicator\n");
        printf("s/S       -  Scale model smaller/larger\n");
        printf("t         -  Show model stats\n");
        printf("o         -  Weld vertices in model\n");
        printf("+/-       -  Increase/decrease smoothing angle\n");
        printf("W         -  Write model to file (out.obj)\n");
		printf("a         -  Toggle ascii effect\n");
		printf("B		  -  Toggle blur\n");
		printf("q		  -  Toggle cel-shading\n");
        printf("q/escape  -  Quit\n\n");
        break;

	case 'a':
		ascii++;
		if (ascii > 2) {
			ascii = 0;
		}
		break;

	case 'q':
		cel_shading = !cel_shading;
		break;

    case 't':
        stats = !stats;
        break;
        
    case 'p':
        performance = !performance;
        break;
        
    case 'm':
        material_mode++;
        if (material_mode > 2)
            material_mode = 0;
        printf("material_mode = %d\n", material_mode);
        lists();
        break;
        
    case 'd':
        glmDelete(model);
        init();
        lists();
        break;
        
    case 'w':
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (params[0] == GL_FILL)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
        
    case 'c':
        if (glIsEnabled(GL_CULL_FACE))
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);
        break;
        
    case 'b':
        bounding_box = !bounding_box;
        break;
    
	case 'B':
		blurring++;
		if (blurring > 2) {
			blurring = 0;
		}
		break;

    case 'n':
        facet_normal = !facet_normal;
        lists();
        break;
        
    case 'r':
        glmReverseWinding(model);
        lists();
        break;
        
    case 's':
        glmScale(model, 0.8);
        lists();
        break;
        
    case 'S':
        glmScale(model, 1.25);
        lists();
        break;
        
    case 'o':
        //printf("Welded %d\n", glmWeld(model, weld_distance));
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case 'O':
        weld_distance += 0.01;
        printf("Weld distance: %.2f\n", weld_distance);
        glmWeld(model, weld_distance);
        glmFacetNormals(model);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case '-':
        smoothing_angle -= 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case '+':
        smoothing_angle += 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;
        
    case 'W':
        glmScale(model, 1.0/scale);
        glmWriteOBJ(model, "out.obj", GLM_SMOOTH | GLM_MATERIAL);
        break;
        
    case 'R':
        {
            GLuint i;
            GLfloat swap;
            for (i = 1; i <= model->numvertices; i++) {
                swap = model->vertices[3 * i + 1];
                model->vertices[3 * i + 1] = model->vertices[3 * i + 2];
                model->vertices[3 * i + 2] = -swap;
            }
            glmFacetNormals(model);
            lists();
            break;
        }
        
    case 27:
        exit(0);
        break;
    }
    
    glutPostRedisplay();
}

void
menu(int item)
{
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
    
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
        dirp = opendir(DATA_DIR);
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;
        name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
        strcpy(name, DATA_DIR);
        strcat(name, direntp->d_name);
        model = glmReadOBJ(name);
        scale = glmUnitize(model);
        glmFacetNormals(model);
        glmVertexNormals(model, smoothing_angle);
        
        if (model->nummaterials > 0)
            material_mode = 2;
        else
            material_mode = 0;
        
        lists();
        free(name);
        
        glutPostRedisplay();
    }
}

static GLint      mouse_state;
static GLint      mouse_button;

void
mouse(int button, int state, int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    /* fix for two-button mice -- left mouse + shift = middle mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
        button = GLUT_MIDDLE_BUTTON;
    
    gltbMouse(button, state, x, y);
    
    mouse_state = state;
    mouse_button = button;
    
    if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }
    
    glutPostRedisplay();
}

void
motion(int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    gltbMotion(x, y);
    
    if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }
    
    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    int buffering = GLUT_DOUBLE;
    struct dirent* direntp;
    DIR* dirp;
    int models;
    
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    
    while (--argc) {
        if (strcmp(argv[argc], "-sb") == 0)
            buffering = GLUT_SINGLE;
        else
            model_file = argv[argc];
    }
    
    if (!model_file) {
        model_file = "data/dolphins.obj";
    }
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
    glutCreateWindow("Smooth");
    
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    
    models = glutCreateMenu(menu);
    dirp = opendir(DATA_DIR);
    if (!dirp) {
        fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
    
    glutCreateMenu(menu);
    glutAddMenuEntry("Smooth", 0);
    glutAddMenuEntry("", 0);
    glutAddSubMenu("Models", models);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[w]   Toggle wireframe/filled", 'w');
    glutAddMenuEntry("[c]   Toggle culling on/off", 'c');
    glutAddMenuEntry("[n]   Toggle face/smooth normals", 'n');
    glutAddMenuEntry("[b]   Toggle bounding box on/off", 'b');
	glutAddMenuEntry("[B]   Toggle blurring effect", 'B');
    glutAddMenuEntry("[p]   Toggle frame rate on/off", 'p');
    glutAddMenuEntry("[t]   Toggle model statistics", 't');
    glutAddMenuEntry("[m]   Toggle color/material/none mode", 'm');
    glutAddMenuEntry("[r]   Reverse polygon winding", 'r');
    glutAddMenuEntry("[s]   Scale model smaller", 's');
    glutAddMenuEntry("[S]   Scale model larger", 'S');
    glutAddMenuEntry("[o]   Weld redundant vertices", 'o');
    glutAddMenuEntry("[+]   Increase smoothing angle", '+');
    glutAddMenuEntry("[-]   Decrease smoothing angle", '-');
    glutAddMenuEntry("[W]   Write model to file (out.obj)", 'W');
	glutAddMenuEntry("[a]   Toggle ascii effect", 'a');
	glutAddMenuEntry("[q]   Toggle cel-shading", 'q');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    init();
    
    glutMainLoop();
    return 0;
}
