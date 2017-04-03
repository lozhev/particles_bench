#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include "glad/glad.h"
#include <GL/glut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"

typedef BOOL (FGAPIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

void Display(void);
void Idle(void);

int NUM_PONTS=16000;
float* points;
float* angles;

GLuint tex;
GLuint point_vbuffer;

char* vert_src="\
attribute vec3 pos;\
uniform float time;\
\
void main()\
{\
	gl_Position = vec4(pos.xy, 0.0, 1.0);\
}";

const char* frag_src="\
uniform sampler2D u_tex;\
void main() {\
	gl_FragColor = texture2D(u_tex, gl_PointCoord);\
}";

GLuint prog;

int main(int argc, char **argv){
	int i;
	FILE* f;
	GLubyte* data;
	GLuint vert_id,frag_id;
	GLint success;
	/*stbi_uc* img_data;	
	int x,y;

	img_data = stbi_load("../flare.png",&x,&y,0,0);
	f=fopen("../flare.rgba","wb");
	fwrite(img_data,x*y*4,1,f);
	fclose(f);*/


	srand((unsigned int)time(0));
	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("timer test");

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);

	gladLoadGL();

	vert_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_id,1,&vert_src,0);
	glCompileShader(vert_id);
	glGetShaderiv(vert_id, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(vert_id, 1000, NULL, log);
		//print("failed compile: %s\ninfo:%s\n",source,log);
		glDeleteShader(vert_id);
		return 0;
	}

	frag_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_id,1,&frag_src,0);
	glCompileShader(frag_id);
	glGetShaderiv(frag_id, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(frag_id, 1000, NULL, log);
		//print("failed compile: %s\ninfo:%s\n",source,log);
		glDeleteShader(frag_id);
		return 0;
	}

	prog = glCreateProgram();
	glAttachShader(prog, vert_id);
	glAttachShader(prog, frag_id);
	glLinkProgram(prog);
	glGetProgramiv(prog,GL_LINK_STATUS,&success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(prog, 1000, NULL, log);
		//print("failed compile: %s\ninfo:%s\n",source,log);
		glDeleteProgram(prog);
		return 0;
	}

	glUseProgram(prog);


	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(0);

	glClearColor(0.f, 0.0f, 0.f, 1.f);

	points = (float*)malloc(12*NUM_PONTS);
	//angles = (float*)malloc(4*NUM_PONTS);
	for (i=0;i<NUM_PONTS;i+=3){
		points[i] = 0;
		points[i+1] = 0;
		//angles[i] = 2.f * (float)M_PI * rand()/(float)RAND_MAX;
		points[i+2] = 2.f * (float)M_PI * rand()/(float)RAND_MAX;
	}

	//f = fopen("../point.rgba","rb");
	f = fopen("../flare.rgba","rb");
	data = (GLubyte*)malloc(64*64*4);
	fread(data,64*64*4,1,f);
	fclose(f);
	
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D,tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	glEnable(GL_TEXTURE_2D);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable point sprites (Note: We need more than basic OpenGL 1.0 for this functionality - so be sure to use GLEW or such)
	glEnable(GL_POINT_SPRITE);//!!!!

	// Specify the origin of the point sprite
	//glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT); // Default - only other option is GL_LOWER_LEFT

	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);//!!!

	// Specify the drawing mode for point sprites
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);       // Draw on top of stuff
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  // Try this if you like...
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);   // Or this... not sure exactly how they differ ;-)

	glPointSize(10.f);

	glEnableClientState(GL_VERTEX_ARRAY);// for 2
	
	/*glEnableVertexAttribArray(0);// for 3
	glGenBuffers(1,&point_vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER,point_vbuffer);
	glBufferData(GL_ARRAY_BUFFER,12*NUM_PONTS,points,GL_STATIC_DRAW);*/

	glutMainLoop();
	return 0;
}

void Display(void){
	int i;
	glClear(GL_COLOR_BUFFER_BIT);

	//glColor4f(1.f, 1.0f, 1.f, 0.5f);
	
    // 1
    /*glBegin(GL_POINTS);
	for (i=0;i<NUM_PONTS;i+=3){
        glVertex2f(points[i],points[i+1]);
		//glColor4f(1.f, 1.0f, 1.f, 0.2f);
	}
    glEnd();*/

	// 2	
	glVertexPointer(2,GL_FLOAT,12,points);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);

	// 3
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0/*12*/, 0);
	//glDrawArrays(GL_POINTS,0,NUM_PONTS);


	glutSwapBuffers();	
}

int frameCount = 0;
int currentTime = 0, previousTime = 0;
float lastTime=0;
void Idle(void){
	int i;
	int timeInterval;
	float elapsed,time;
	char str[128];	

	++frameCount;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	timeInterval = currentTime - previousTime;
	time = currentTime/1000.f;
	elapsed = time - lastTime;
	lastTime = time;
	if(timeInterval > 1000)	{
		previousTime = currentTime;
		
		sprintf(str,"fps: %d\n",frameCount);
		OutputDebugStringA(str);
		frameCount = 0;
	}

	for (i=0;i<NUM_PONTS;i+=3){
		points[i] = points[i]+sinf(points[i+2])*elapsed*0.49f;
		points[i+1] = points[i+1]+cosf(points[i+2])*elapsed*0.49f;
		if (points[i]>1 || points[i]<-1 ||
			points[i+1]>1 || points[i+1]<-1) {
				points[i]=0; 
				points[i+1]=0;
				points[i+2] = 2.f * (float)M_PI * rand()/(float)RAND_MAX;
		}
	}

	//glBufferSubData(GL_ARRAY_BUFFER,0,8*NUM_PONTS,points);

	glutPostRedisplay();
}