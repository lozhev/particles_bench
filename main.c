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

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void Display(void);
void Idle(void);

// shared data
typedef struct{
    void (*init)();
    void (*update)(float);
    void (*draw)();
    void (*deinit)();
}Method;

#define NUM_PONTS 16000
float* points;
GLuint sprite_tex;
int curr_method=1;
Method methods[3];

typedef struct{
    float init_time[3];
    float update_time;
    float draw_time;
    float fps;
    int counter[4];
}Counter;

Counter counter[3];

void start_init(int id, float time){
    counter[id].init_time[0] = time;
}

void stop_init(int id, float time){
    counter[id].init_time[1] = time - counter[id].init_time[0];
    counter[id].init_time[2] += counter[id].init_time[1];
    ++counter[id].counter[0];
}

float getAver_init(int id){
    return counter[id].init_time[2]/counter[id].counter;
}


//////////////////////////////////////////////////////////////////////////
// fixedpipeline
float* point_verts;

//////////////////////////////////////////////////////////////////////////
// point_gl11

GLuint point_gl11_prog;


//////////////////////////////////////////////////////////////////////////
// point_gl20
GLuint point_gl20_prog;
GLuint point_gl20_vbuffer;


//////////////////////////////////////////////////////////////////////////
// font

const char* fnt_vert_src="\
attribute vec4 pos;\
varying vec2 v_uv;\
\
void main()\
{\
	gl_Position = vec4(pos.xy, 0.0, 1.0);\
	v_uv = pos.zw;\
}";

const char* fnt_frag_src="\
uniform sampler2D u_tex;\
varying vec2 v_uv;\
void main() {\
	gl_FragColor = texture2D(u_tex, v_uv);\
}";

#if 1
static const char failed_compile_str[] = "failed compile: %s\ninfo:%s\n";
static const char failed_link_str[] = "failed link: %s\n";
#define CHECKSHADER(shd,src)\
    glGetShaderiv(shd, GL_COMPILE_STATUS, &success);\
    if (success != GL_TRUE) {\
    GLchar log[256];\
    glGetShaderInfoLog(shd, 256, NULL, log);\
    printf(failed_compile_str,src,log);\
    glDeleteShader(shd);\
    return -1;\
    }
#define CHECKPROGRAM(p)\
    glGetProgramiv(p,GL_LINK_STATUS,&success);\
    if (success != GL_TRUE) {\
    GLchar log[256];\
    glGetShaderInfoLog(p, 256, NULL, log);\
    printf(failed_link_str,log);\
    glDeleteProgram(p);\
    return -1;\
    }
#else
#define CHECKSHADER(shd,src)
#define CHECKPROGRAM(p)
#endif

GLuint creatProg(const char* vert_src, const char* frag_src){
    GLint success;
    GLuint prog,vert_id,frag_id;

    vert_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_id,1,&vert_src,0);
    glCompileShader(vert_id);
    CHECKSHADER(vert_id,vert_src);

    frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_id,1,&frag_src,0);
    glCompileShader(frag_id);
    CHECKSHADER(frag_id,frag_src);

    prog = glCreateProgram();
    glAttachShader(prog, vert_id);
    glAttachShader(prog, frag_id);
    glLinkProgram(prog);
    CHECKPROGRAM(prog);

    glDeleteShader(vert_id);
    glDeleteShader(frag_id);

    return prog;
}

typedef struct {
	float x0, y0;
	float x1, y1;
	float ratio;
} Character;

typedef struct {
	float position[2];
	float texCoord[2];
}TexVertex;

Character* fnt_chars=0;
GLubyte fnt_table[256]={0};
GLuint fnt_tex=0;
GLuint fnt_prog=0;
int fnt_textquads=0;
TexVertex* fnt_verts=0;
int fnt_verts_count=0;
int fnt_verts_drawcount=0;

#define SETVEC2(v,x,y)\
	v[0]=x;\
	v[1]=y

void fillTextBuffer(TexVertex *dest, const char *str, float x, float y, const float charWidth, const float charHeight) {
	float startx = x;

	while (*str){
		if (*str == '\n'){
			y += charHeight;
			x = startx;
		} else {
			GLubyte ch_id = fnt_table[*(unsigned char *) str];
			Character* chr = &fnt_chars[ch_id];
			float cw = charWidth * chr->ratio;

			SETVEC2(dest[0].position, x, y);
			SETVEC2(dest[0].texCoord, chr->x0, chr->y1);
			SETVEC2(dest[1].position, x + cw, y);
			SETVEC2(dest[1].texCoord, chr->x1, chr->y1);
			SETVEC2(dest[2].position, x, y + charHeight);
			SETVEC2(dest[2].texCoord, chr->x0, chr->y0);

			SETVEC2(dest[3].position, x, y + charHeight);
			SETVEC2(dest[3].texCoord, chr->x0, chr->y0);
			SETVEC2(dest[4].position, x + cw, y);
			SETVEC2(dest[4].texCoord, chr->x1, chr->y1);
			SETVEC2(dest[5].position, x + cw, y + charHeight);
			SETVEC2(dest[5].texCoord, chr->x1, chr->y0);

			dest += 6;
			x += cw;
		}
		str++;
	}
}

void makeText(const char *str){
	int n=0;
	const char* s = str;
	while(*s){
		if (*s != '\n') ++n;
		++s;
	}

	n*=6;
	if (n > fnt_verts_count){
		fnt_verts = (TexVertex*)realloc(fnt_verts,n*sizeof(TexVertex));
		fnt_verts_count = n;
	}
	fnt_verts_drawcount = n;
	fillTextBuffer(fnt_verts,str,-1.0f,0.9f,0.08f,0.1f);
}

	
void loadFont(){
    int i;
    FILE* f;    
    /*
    // little modified humus font
	typedef struct {
		Character chars[256];
		int texture;
	}TexFont;
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
        if (font.chars[i].ratio>0){
            ch[ch_count] = font.chars[i];
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
    {
	char header[128];// DDSHeader
	unsigned char* pixels;
    int size = ((512 + 3) >> 2) * ((512 + 3) >> 2);
	
	fread(&header, 128, 1, f);	
    size *= 8;
    pixels = (unsigned char*)malloc(size);
    fread(pixels,1,size,f);    
    fclose(f);

    glGenTextures(1,&fnt_tex);
	glBindTexture(GL_TEXTURE_2D,fnt_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x83f1,512,512,0,size,pixels);
	free(pixels);
    }

	makeText("Fps:");

    fnt_prog = creatProg(fnt_vert_src,fnt_frag_src);
}



GLuint GetUniforms(GLuint program){
    GLint i, n, max;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);

    for (i=0;i<n;++i){
        GLint size, len, loc;
        GLenum type;
        char name[10];

        glGetActiveUniform(program, i, 10, &len, &size, &type, name);
        
        loc = glGetUniformLocation(program, name);
        if (type == GL_FLOAT){
            printf("%s float\n",name);
        } else if (type == GL_SAMPLER_2D){
            printf("%s SAMPLER_2D\n",name);
        }
    }

    return n;
}

void make_point(){
    int i;
    points = (float*)malloc(12*NUM_PONTS);
    for (i=0;i<NUM_PONTS*3;i+=3){
        float a = 2.f * (float)M_PI * rand()/(float)RAND_MAX;;
        points[i] = sinf(a);
        points[i+1] = cosf(a);
        points[i+2] = NUM_PONTS * (float)rand()/(float)RAND_MAX;        
    }
}

void update_verts(float time){
    int i;
    union{
        unsigned char uc[4];
        float f;
    }color;
    for (i=0;i<NUM_PONTS*3;i+=3){		
		float frac = time+points[i+2];
		float p = frac - (long)frac;
		color.uc[0]=0xff;color.uc[1]=0xff;color.uc[2]=0xff;color.uc[3]=(GLubyte)((1-p)*0xff);
		point_verts[i] = p*points[i+2]/NUM_PONTS * points[i];
		point_verts[i+1] = p*points[i+2]/NUM_PONTS * points[i+1];
		point_verts[i+2] = color.f;        
	}
}

void init1(){
    point_verts = (float*)calloc(12*NUM_PONTS,4);
    make_point();
}

void update1(float time){
    update_verts(time);
}

void draw1(){
    int i;
    // 1 //mesa 161 fps
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
    glBegin(GL_POINTS);
	for (i=0;i<NUM_PONTS*3;i+=3){
        glColor4ubv((GLubyte*)(point_verts+i+2));
        glVertex2f(point_verts[i],point_verts[i+1]);        
	}
    glEnd();
}

void deinit1(){
    free(point_verts);
    free(points);
}

void init2(){
    const char* point_gl11_vert_src=
        "void main(){\n"
        "gl_FrontColor = gl_Color;\n"
        "   gl_Position = gl_Vertex;\n"
        "}";

    // gl_PointCoord #version 110 not work in my linux mesa
    const char* point_gl11_frag_src=
        "#version 120\n"
        "uniform sampler2D u_tex;\n"
        "void main(){\n"
        "   gl_FragColor = texture2D(u_tex, gl_PointCoord)*gl_Color;\n"
        "}";

    point_verts = (float*)calloc(12*NUM_PONTS,4);
    make_point();

    point_gl11_prog = creatProg(point_gl11_vert_src,point_gl11_frag_src);
    glEnableClientState(GL_VERTEX_ARRAY);
}

void update2(float time){
    update_verts(time);
}

void draw2(){
    // 2 mesa ~220 fps
	glUseProgram(point_gl11_prog);
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
    //glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2,GL_FLOAT,12,point_verts);
	glColorPointer(4,GL_UNSIGNED_BYTE,12,point_verts+2);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);
}

void deinit2(){
    free(point_verts);
    free(points);

    glDeleteProgram(point_gl11_prog);
}

void init3(){
    const char* point_gl20_vert_src=
        "attribute vec3 pos;\n"
        "uniform float time;\n"
        "const float NUM_POINTS="STR(NUM_PONTS)";\n"
        "varying vec4 v_col;\n"
        "void main(){\n"
        "   float p = fract(time + pos.z);\n"
        "   vec2 ps = p*pos.z/NUM_POINTS * pos.xy;\n"
        "   gl_Position = vec4(ps, 0.0, 1.0);\n"
        "   v_col = vec4(1.0, 1.0, 1.0, 1.0-p);\n"
        "}";

    const char* point_gl20_frag_src=
        "uniform sampler2D u_tex;\n"
        "varying vec4 v_col;\n"
        "void main(){\n"
        "   gl_FragColor = texture2D(u_tex, gl_PointCoord)*v_col;\n"
        "}";

    point_gl20_prog = creatProg(point_gl20_vert_src,point_gl20_frag_src);

    make_point();

    //glEnableVertexAttribArray(0);// for 3
    glGenBuffers(1,&point_gl20_vbuffer);
    glBindBuffer(GL_ARRAY_BUFFER,point_gl20_vbuffer);
    glBufferData(GL_ARRAY_BUFFER,12*NUM_PONTS,points,GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER,0);
    //glDisableVertexAttribArray(0);

    free(points);
}

void update3(float time){
    glUseProgram(point_gl20_prog);
    glUniform1f(0, time);
}

void draw3(){
    // 3
    glUseProgram(point_gl20_prog);
    glBindTexture(GL_TEXTURE_2D,sprite_tex);    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,point_gl20_vbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_POINTS,0,NUM_PONTS);
    //glDisableVertexAttribArray(0);*/

}

void deinit3(){
    glDeleteProgram(point_gl20_prog);
}

int main(int argc, char **argv){
//	int i;
	FILE* f;
	GLubyte* img_data;
//	GLuint vert_id,frag_id;
//	GLint success;
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

    methods[0].init = init1;
    methods[0].update = update1;
    methods[0].draw = draw1;
    methods[0].deinit = deinit1;

    methods[1].init = init2;
    methods[1].update = update2;
    methods[1].draw = draw2;
    methods[1].deinit = deinit2;

    methods[2].init = init3;
    methods[2].update = update3;
    methods[2].draw = draw3;
    methods[2].deinit = deinit3;
	

#ifdef _WIN32
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("wglSwapIntervalEXT");
#elif __linux
    glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("glXSwapIntervalMESA");
#endif
    glSwapInterval(0);

	glClearColor(0.f, 0.0f, 0.f, 1.f);

	

	//f = fopen("../point.rgba","rb");
	f = fopen("../flare.rgba","rb");
	img_data = (GLubyte*)malloc(64*64*4);
	fread(img_data,1,64*64*4,f);
	fclose(f);
	
	glGenTextures(1,&sprite_tex);
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,img_data);
	free(img_data);

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
	//glEnableClientState(GL_COLOR_ARRAY);
	
	methods[curr_method].init();

	glutMainLoop();
	return 0;
}

void Display(void){
//	int i;
	glClear(GL_COLOR_BUFFER_BIT);

    methods[curr_method].draw();

	// draw font
	glUseProgram(fnt_prog);
	glBindTexture(GL_TEXTURE_2D,fnt_tex);
    //glDisableClientState(GL_COLOR_ARRAY);
	//glVertexPointer(4,GL_FLOAT,0,fnt_verts);
    glBindBuffer(GL_ARRAY_BUFFER,0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, fnt_verts);
	glDrawArrays(GL_TRIANGLES,0,fnt_verts_drawcount);
	glUseProgram(0);
	glDisableVertexAttribArray(0);


	glutSwapBuffers();
}

int frameCount = 0;
int currentTime = 0, previousTime = 0;
float lastTime=0;
void Idle(void){
//	int i;
	int timeInterval;
	float elapsed,time;

	++frameCount;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	timeInterval = currentTime - previousTime;
	time = currentTime/1000.f;
	elapsed = time - lastTime;
	lastTime = time;
	if(timeInterval > 1000){
		char str[128];
		sprintf(str,"Fps: %d\n",frameCount);
#ifdef _MSC_VER        
		OutputDebugStringA(str);
#else
        printf("%s",str);
#endif
		makeText(str);
		previousTime = currentTime;
		frameCount = 0;
	}

    methods[curr_method].update(time);

	glutPostRedisplay();
}
