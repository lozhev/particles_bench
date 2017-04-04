#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include "glad/glad.h"
//#include <GL/glut.h>
#include <GL/freeglut.h>
//#define STB_IMAGE_IMPLEMENTATION
//#include "../stb/stb_image.h"

#ifdef _WIN32
typedef BOOL (FGAPIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC glSwapInterval;
#elif __linux
typedef int (*PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC glSwapInterval;
#endif

void Display(void);
void Idle(void);

int NUM_PONTS=1600;
float* vb_data;
float* points;

GLuint tex;
GLuint point_vbuffer;

const char* vert_src="\
attribute vec3 pos;\
uniform float time;\
\
void main()\
{\
	gl_Position = vec4(pos.xy, 0.0, 1.0);\
}";

const char* vert_src2="\
attribute float pos;\
uniform float time;\
\
void main()\
{\
	gl_Position = vec4(pos.xy, 0.0, 1.0);\
}";

// gl_PointCoord bad implementation
// #version 110 not work in my linux mesa
const char* frag_src="\
#version 120\n\
uniform sampler2D u_tex;\
void main() {\
	gl_FragColor = texture2D(u_tex, gl_PointCoord);\
}";

GLuint prog;

typedef struct {
	float x0, y0;
	float x1, y1;
	float ratio;
} Character;

typedef struct {
	Character chars[256];
	int texture;
}TexFont;

typedef struct {
	float position[2];
	float texCoord[2];
}TexVertex;

Character* fnt_chars;
GLubyte fnt_table[256];
GLuint fnt_tex;
TexVertex* fnt_vbdata;

#define MAKEQUAD(x0, y0, x1, y1, o)\
	vec2(x0 + o, y0 + o),\
	vec2(x0 + o, y1 - o),\
	vec2(x1 - o, y0 + o),\
	vec2(x1 - o, y1 - o),

#define MAKETEXQUAD(x0, y0, x1, y1, o)\
	TexVertex(vec2(x0 + o, y0 + o), vec2(0, 0)),\
	TexVertex(vec2(x0 + o, y1 - o), vec2(0, 1)),\
	TexVertex(vec2(x1 - o, y0 + o), vec2(1, 0)),\
	TexVertex(vec2(x1 - o, y1 - o), vec2(1, 1)),
	
typedef struct {
	GLuint dwMagic;
	GLuint dwSize;
	GLuint dwFlags;
	GLuint dwHeight;
	GLuint dwWidth;
	GLuint dwPitchOrLinearSize;
	GLuint dwDepth; 
	GLuint dwMipMapCount;
	GLuint dwReserved[11];

	struct {
        GLuint dwSize;
		GLuint dwFlags;
		GLuint dwFourCC;
		GLuint dwRGBBitCount;
		GLuint dwRBitMask;
		GLuint dwGBitMask;
		GLuint dwBBitMask;
		GLuint dwRGBAlphaBitMask; 
	} ddpfPixelFormat;

	struct {
		GLuint dwCaps1;
		GLuint dwCaps2;
		GLuint Reserved[2];
	} ddsCaps;

	GLuint dwReserved2;
}DDSHeader;

void loadFont(){
    int i;
    FILE* f; 
    DDSHeader header;
    /*
    // little modified humus font
    GLuint ver;
    TexFont font;    
    GLubyte table[256];
    Character ch[256];
    int ch_count=0;
    
    f = fopen("Fonts/Future.font","rb");
    fread(&ver,4,1,f);
    fread(&font,sizeof(TexFont),1,f);
    fclose(f);
    
    for(i=0;i<256;++i){
        table[i] = -1;
        if (font.fnt_chars[i].ratio>0){
            ch[ch_count] = font.fnt_chars[i];
            table[i] = ch_count;
            ++ch_count;
        }        
    }
    f = fopen("Fonts/Future.fnt","wb");
    fwrite(table,1,256,f);
    fwrite(&ch_count,4,1,f);
    fwrite(&ch,sizeof(Character),ch_count,f);
    fclose(f);*/
    
    f = fopen("Fonts/Future.fnt","rb");
    fread(&fnt_table,1,256,f);
    fread(&i,4,1,f);
    fnt_chars = (Character*)malloc(sizeof(Character)*i);
    fread(fnt_chars,sizeof(Character),i,f);
    fclose(f);
    
    /*for(i=0;i<256;++i){
        if (fnt_table[i]==0xff) continue;
        Character* ch = &fnt_chars[fnt_table[i]];
        printf("%d\n",i);
        printf("    %f %f\n",ch->x0,ch->y0);
        printf("    %f %f\n",ch->x1,ch->y1);
        printf("        %f\n",ch->ratio);
    }*/
    
    f = fopen("Fonts/Future2.dds", "rb");
    fread(&header, sizeof(header), 1, f);
    {
    int size = ((512 + 3) >> 2) * ((512 + 3) >> 2);
    size *= 8;
    unsigned char* pixels = (unsigned char*)malloc(size);
    fread(pixels,1,size,f);    
    fclose(f);
    glGenTextures(1,&fnt_tex);
	glBindTexture(GL_TEXTURE_2D,fnt_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x83f1,512,512,0,size,pixels);
    }
}


int main(int argc, char **argv){
	int i;
	FILE* f;
	GLubyte* img_data;
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
    //glutInitContextVersion ( 2, 1 );
    //glutInitContextFlags   ( GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG );
    //glutInitContextProfile ( GLUT_CORE_PROFILE );
	glutCreateWindow("test");

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);

	gladLoadGL();
    
    loadFont();

	vert_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_id,1,&vert_src,0);
	glCompileShader(vert_id);
	glGetShaderiv(vert_id, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(vert_id, 1000, NULL, log);
		printf("failed compile: %s\ninfo:%s\n",vert_src,log);
		glDeleteShader(vert_id);
		return -1;
	}

	frag_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_id,1,&frag_src,0);
	glCompileShader(frag_id);
	glGetShaderiv(frag_id, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(frag_id, 1000, NULL, log);
		printf("failed compile: %s\ninfo:%s\n",frag_src,log);
		glDeleteShader(frag_id);
		return -1;
	}

	prog = glCreateProgram();
	glAttachShader(prog, vert_id);
	glAttachShader(prog, frag_id);
	glLinkProgram(prog);
	glGetProgramiv(prog,GL_LINK_STATUS,&success);
	if (success != GL_TRUE) {
		GLchar log[1000];
		glGetShaderInfoLog(prog, 1000, NULL, log);
		printf("failed link: :%s\n",log);
		glDeleteProgram(prog);
		return -1;
	}

	//glUseProgram(prog);

#ifdef _WIN32
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("wglSwapIntervalEXT");
#elif __linux
    glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("glXSwapIntervalMESA");
#endif
    glSwapInterval(0);

	glClearColor(0.f, 0.0f, 0.f, 1.f);

	vb_data = (float*)malloc(12*NUM_PONTS);
	points = (float*)malloc(12*NUM_PONTS);
	for (i=0;i<NUM_PONTS*3;i+=3){
        float a = 2.f * (float)M_PI * rand()/(float)RAND_MAX;;
		points[i] = sinf(a);
		points[i+1] = cosf(a);
		points[i+2] = NUM_PONTS * (float)rand()/(float)RAND_MAX;
        
        vb_data[i] = vb_data[i+1] = vb_data[i+2] = 0;
	}

	//f = fopen("../point.rgba","rb");
	f = fopen("../flare.rgba","rb");
	img_data = (GLubyte*)malloc(64*64*4);
	fread(img_data,64*64*4,1,f);
	fclose(f);
	
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D,tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,img_data);
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

	glPointSize(19.f);

	glEnableClientState(GL_VERTEX_ARRAY);// for 2
	glEnableClientState(GL_COLOR_ARRAY);
	
	/*glEnableVertexAttribArray(0);// for 3
	glGenBuffers(1,&point_vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER,point_vbuffer);
	glBufferData(GL_ARRAY_BUFFER,12*NUM_PONTS,points,GL_STATIC_DRAW);*/

	glutMainLoop();
	return 0;
}

void Display(void){
	//int i;
	glClear(GL_COLOR_BUFFER_BIT);

	//glColor4f(1.f, 1.0f, 1.f, 0.5f);
	
    // 1 //mesa 161 fps
    /*glBegin(GL_POINTS);
	for (i=0;i<NUM_PONTS*3;i+=3){
        glColor4ubv((GLubyte*)(vb_data+i+2));
        glVertex2f(vb_data[i],vb_data[i+1]);        
	}
    glEnd();*/

	// 2 mesa ~220 fps
	glVertexPointer(2,GL_FLOAT,12,vb_data);
	glColorPointer(4,GL_UNSIGNED_BYTE,12,vb_data+2);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);

	// 3
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 12, 0);
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

	++frameCount;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	timeInterval = currentTime - previousTime;
	time = currentTime/1000.f;
	elapsed = time - lastTime;
	lastTime = time;
	if(timeInterval > 1000){
#ifdef _MSC_VER
        char str[128];
		sprintf(str,"fps: %d\n",frameCount);
		OutputDebugStringA(str);
#else
        printf("fps: %d\n",frameCount);
#endif
		previousTime = currentTime;
		frameCount = 0;
	}

	for (i=0;i<NUM_PONTS*3;i+=3){
		union{
			unsigned char uc[4];
			float f;
		}color;
		//points[i] = points[i]+sinf(points[i+2])*elapsed*0.49f;
		//points[i+1] = points[i+1]+cosf(points[i+2])*elapsed*0.49f;
        float intpart;//float intpart; modff();
		float p = (float)modff(time+points[i+2],&intpart);
		color.uc[0]=0xff;color.uc[1]=0xff;color.uc[2]=0xff;color.uc[3]=(1-p)*0xff;
		vb_data[i] = p*points[i+2]/NUM_PONTS * points[i];
		vb_data[i+1] = p*points[i+2]/NUM_PONTS * points[i+1];
		vb_data[i+2] = color.f;        
	}

	//glBufferSubData(GL_ARRAY_BUFFER,0,12*NUM_PONTS,points);

	glutPostRedisplay();
}
