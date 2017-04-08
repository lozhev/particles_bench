#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string.h>
#ifndef __ANDROID__
#include "glad/glad.h"
#endif
//#include <GL/glut.h>
#include <GL/freeglut.h>
//#define STB_IMAGE_IMPLEMENTATION
//#include "../stb/stb_image.h"

#define NUM_PONTS 1600
#define SECONDS_PER_METHOD 10
#define CURRENT_METHOD 0
#define WRITE_RESULT 1
#define CYCLE_METODS 1
#define OUTPUT_FPS 0

#ifdef __ANDROID__
#define EMBEDDED_DATA
#endif

// shared data
typedef struct{
	float start;
	float accum;
	int counter;
}Timer;

typedef struct{
	float init;
	float update;
	float draw;
	float deinit;
	int fps;

	float acc_init;
	float acc_update;
	float acc_draw;
	float acc_deinit;
	int acc_fps;

	int count;
	char* name;
}Table_time;

typedef struct{
	void (*init)();
	void (*update)(float);
	void (*draw)();
	void (*deinit)();
	Timer timer_init;
	Timer timer_update;
	Timer timer_draw;
	Timer timer_deinit;
	char* name;
}Method;

#define NUM_METHODS 5

float* points;
GLuint sprite_tex;
int curr_method=CURRENT_METHOD;
Method methods[NUM_METHODS];
Table_time tt[NUM_METHODS];

//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
typedef BOOL (APIENTRYP PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC glSwapInterval;

typedef void (APIENTRYP PFNGLUNIFORM1FARBPROC)(GLint location, GLint count, GLfloat* v0);
PFNGLUNIFORM1FARBPROC glUniform1fARB;

static double __timeTicksPerMillis;
static double __timeStart;
static double __timeAbsolute;

double seTime() {
	LARGE_INTEGER queryTime;
	QueryPerformanceCounter(&queryTime);
	__timeAbsolute = queryTime.QuadPart / __timeTicksPerMillis;

	return __timeAbsolute - __timeStart;
}
#elif __linux
typedef int (*PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC glSwapInterval;

struct timespec __timespec;
static double __timeStart;
static double __timeAbsolute;

double timespec2millis(struct timespec* a) {
	return (1000.0 * a->tv_sec) + (0.000001 * a->tv_nsec);
}

double seTime() {
	clock_gettime(CLOCK_REALTIME, &__timespec);
	double now = timespec2millis(&__timespec);
	__timeAbsolute = now - __timeStart;

	return __timeAbsolute;
}
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void Display(void);
void Idle(void);

// Timer
void t_start(Timer* t, float start){
	t->start = start;
}

float t_stop(Timer* t, float stop){
	float dt = stop - t->start;
	t->accum += dt;
	++t->counter;
	return dt;
}
float t_getAvg(Timer* t){
	float ret = t->accum / (float)t->counter;
	t->accum = 0.f;
	t->counter = 0;
	return ret;
}

// Table_time
void tt_set(Table_time* tt, float i, float u, float d, float de, int f){
	tt->init = i;
	tt->update = u;
	tt->draw = d;
	tt->deinit = de;
	tt->fps = f;

	tt->acc_init += i;
	tt->acc_update += u;
	tt->acc_draw += d;
	tt->acc_deinit += de;
	tt->acc_fps += f;

	++tt->count;
}

int tt_cmp_fps(const void* lhs, const void* rhs){
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lf = l->acc_fps / (float)l->count;
	float rf = r->acc_fps / (float)r->count;
	if (lf < rf) return  1;
	if (lf > rf) return -1;
	return 0;
}

int tt_cmp_time(const void* lhs, const void* rhs){
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lt = (l->acc_init + l->acc_update + l->acc_draw + l->acc_deinit) / (float)l->count;
	float rt = (r->acc_init + r->acc_update + r->acc_draw + r->acc_deinit) / (float)r->count;
	if (lt < rt) return -1;
	if (lt > rt) return  1;
	return 0;
}

int tt_cmp_runtime(const void* lhs, const void* rhs){
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lt = (l->acc_update + l->acc_draw) / (float)l->count;
	float rt = (r->acc_update + r->acc_draw) / (float)r->count;
	if (lt < rt) return -1;
	if (lt > rt) return  1;
	return 0;
}

void tt_write(){
	int i, n;
	FILE* f = fopen("results.txt","w");
	char* str = (char*)malloc(256);

	n = sprintf(str,"vendor: %s\n", glGetString(GL_VENDOR));
	fwrite(str,1,n,f);
	n = sprintf(str,"renderer: %s\n", glGetString(GL_RENDERER));
	fwrite(str,1,n,f);
	n = sprintf(str,"version: %s\n", glGetString(GL_VERSION));
	fwrite(str,1,n,f);
	n = sprintf(str,"points: %d\n\n", NUM_PONTS);
	fwrite(str,1,n,f);

	n = sprintf(str,"%s\n","fps:");
	fwrite(str,1,n,f);
	qsort(tt,NUM_METHODS,sizeof(Table_time),tt_cmp_fps);
	for(i=0;i<NUM_METHODS;++i){
		n = sprintf(str,"%7.2f ",tt[i].acc_fps/(float)tt[i].count);
		fwrite(str,1,n,f);

		n = sprintf(str,"%s\n",tt[i].name);
		fwrite(str,1,n,f);
	}

	n = sprintf(str,"\n%s","cpu time millisecond\n");
	fwrite(str,1,n,f);

	n = sprintf(str,"\n| %s       |","all");
	fwrite(str,1,n,f);
	n = sprintf(str," %s      |","init");
	fwrite(str,1,n,f);
	n = sprintf(str," %s    |","update");
	fwrite(str,1,n,f);
	n = sprintf(str," %s      |","draw");
	fwrite(str,1,n,f);
	n = sprintf(str," %s    |\n","deinit");
	fwrite(str,1,n,f);

	for(i=0;i<61;++i) str[i]='-';
	str[i]='\n';
	fwrite(str,1,62,f);

	qsort(tt,NUM_METHODS,sizeof(Table_time),tt_cmp_time);
	for(i=0;i<NUM_METHODS;++i){
		float it = tt[i].acc_init / (float)tt[i].count;
		float iu = tt[i].acc_update / (float)tt[i].count;
		float id = tt[i].acc_draw / (float)tt[i].count;
		float ide = tt[i].acc_deinit / (float)tt[i].count;
		float all_time = it + iu + id + ide;
		n = sprintf(str,"| %9.5f | %9.5f | %9.5f | %9.5f | %9.5f |", all_time, it,iu,id,ide);
		fwrite(str,1,n,f);

		n = sprintf(str," %s\n",tt[i].name);
		fwrite(str,1,n,f);
	}

	n = sprintf(str,"\n| %s   |","runtime");
	fwrite(str,1,n,f);
	n = sprintf(str," %s    |","update");
	fwrite(str,1,n,f);
	n = sprintf(str," %s      |\n","draw");
	fwrite(str,1,n,f);

	for(i=0;i<37;++i) str[i]='-';
	str[i]='\n';
	fwrite(str,1,38,f);

	qsort(tt,NUM_METHODS,sizeof(Table_time),tt_cmp_runtime);
	for(i=0;i<NUM_METHODS;++i){
		float iu = tt[i].acc_update / (float)tt[i].count;
		float id = tt[i].acc_draw / (float)tt[i].count;
		float all_time = iu + id;
		n = sprintf(str,"| %9.5f | %9.5f | %9.5f |", all_time, iu,id);
		fwrite(str,1,n,f);

		n = sprintf(str," %s\n",tt[i].name);
		fwrite(str,1,n,f);
	}

	fclose(f);

	free(str);

	memset(tt,0,sizeof(tt));
	exit(0);
	//make exit!!
}





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

//precision mediump float;
const char* fnt_frag_src="\
uniform sampler2D u_tex;\
varying vec2 v_uv;\
void main(){\
	gl_FragColor = texture2D(u_tex, v_uv);\
}";

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
static const GLubyte* fnt_table=0;
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

#if 0
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
#define INT_SUCCSESS GLint success;
#else
#define CHECKSHADER(shd,src)
#define CHECKPROGRAM(p)
#define INT_SUCCSESS
#endif

GLuint creatProg(const char* vert_src, const char* frag_src){
	INT_SUCCSESS
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


void loadFont(){  
	/*
	// little modification humus font
	typedef struct {
		Character chars[256];
		int texture;
	}TexFont;
	GLuint ver;
	TexFont font;    
	GLubyte table[256];
	Character ch[256];
	FILE* f;
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

#ifdef EMBEDDED_DATA
	{
#include "fnt_table.h"
#include "fnt_chars.h"
	fnt_table = Future_fnt_table;
	fnt_chars = (Character*)Future_fnt_chars;
	}
	// build EMBEDDED fnt_table
	/*{int i, n, line_w = 0;
	char str[256];
	f = fopen("../fnt_table.h", "w");
	n = sprintf(str, "static const unsigned char Future_fnt_table[%d]={\n", 256);
	fwrite(str, 1, n, f);
	for (i = 0; i < 256 - 1; ++i) {
		n = sprintf(str, "0x%x,", fnt_table[i]);
		fwrite(str, 1, n, f);
		line_w += n;
		if (line_w > 80) {
			fwrite("\n", 1, 1, f);
			line_w = 0;
		}
	}
	n = sprintf(str, "0x%x", fnt_table[i]);
	fwrite(str, 1, n, f);
	n = sprintf(str, "%s", "};");
	fwrite(str, 1, n, f);
	fclose(f);
	}*/
/*  // build EMBEDDED fnt_chars
	{int i, n, line_w = 0;
	char str[256];
	GLubyte* data_chars = (GLubyte*)fnt_chars;// make/load it first!!
	f = fopen("../fnt_chars.h", "w");
	n = sprintf(str, "static const unsigned char Future_fnt_chars[%d]={\n", 1860);
	fwrite(str, 1, n, f);
	for (i = 0; i < 1860 - 1; ++i) {
		n = sprintf(str, "0x%x,", data_chars[i]);
		fwrite(str, 1, n, f);
		line_w += n;
		if (line_w > 80) {
			fwrite("\n", 1, 1, f);
			line_w = 0;
		}
	}
	n = sprintf(str, "0x%x", data_chars[i]);
	fwrite(str, 1, n, f);
	n = sprintf(str, "%s", "};");
	fwrite(str, 1, n, f);
	fclose(f);
	}*/
#else
	{int i;
	FILE* f = fopen("Fonts/Future.fnt","rb");
	fnt_table = (GLubyte*)malloc(256);
	fread((void*)fnt_table,1,256,f);
	fread(&i,4,1,f);
	fnt_chars = (Character*)malloc(sizeof(Character)*i);
	fread(fnt_chars,sizeof(Character),i,f);
	fclose(f);}
#endif

	/* check font
	for(i=0;i<256;++i){
	if (fnt_table[i]==0xff) continue;
	Character* ch = &fnt_chars[fnt_table[i]];
	printf("%d\n",i);
	printf("    %f %f\n",ch->x0,ch->y0);
	printf("    %f %f\n",ch->x1,ch->y1);
	printf("        %f\n",ch->ratio);
	}*/

	glGenTextures(1,&fnt_tex);
	glBindTexture(GL_TEXTURE_2D,fnt_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


#ifdef EMBEDDED_DATA
	{
#include "Future2_rgba.h"
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, Future2_rgba);
	}
	// build EMBEDDED font texture
/*  {
		stbi_uc* img_data;
		int x,y;
		int i,n,line_w=0;
		char str[256];

		img_data = stbi_load("Fonts/Future2.png",&x,&y,0,0);
		//f=fopen("Fonts/Future2.rgba","wb");
		//fwrite(img_data,1,x*y*4,f);
		//fclose(f);

		f = fopen("../Future2_rgba.h", "w");
		n = sprintf(str, "static const unsigned char Future2_rgba[%d]={\n", x*y*4);
		fwrite(str, 1, n, f);
		for (i = 0; i < (x*y*4)-1; ++i) {
			n = sprintf(str, "0x%x,", img_data[i]);
			fwrite(str, 1, n, f);
			line_w += n;
			if (line_w > 80) {
				fwrite("\n", 1, 1, f);
				line_w = 0;
			}
		}
		n = sprintf(str, "0x%x", img_data[i]);
		fwrite(str, 1, n, f);
		n = sprintf(str, "%s", "};");
		fwrite(str, 1, n, f);
		fclose(f);
	}*/
#else
	{
	FILE* f = fopen("Fonts/Future2.dds", "rb");
	char header[128];// DDSHeader
	unsigned char* pixels;
	int size = ((512 + 3) >> 2) * ((512 + 3) >> 2);

	fread(&header, 128, 1, f);
	size *= 8;
	pixels = (unsigned char*)malloc(size);
	fread(pixels,1,size,f);    
	fclose(f);
	
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x83f1,512,512,0,size,pixels);
	free(pixels);
	}
#endif
	makeText("Fps:");

	fnt_prog = creatProg(fnt_vert_src,fnt_frag_src);
}



GLuint GetUniforms(GLuint program){
	GLint i, n, max;
	char* name;

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);
	name = (char*)malloc(max);

	for (i=0;i<n;++i){
		GLint size, len, loc;
		GLenum type;        

		glGetActiveUniform(program, i, 10, &len, &size, &type, name);
		
		loc = glGetUniformLocation(program, name);
		printf("%d: %s ", i, name);
		if (type == GL_FLOAT){
			printf("float\n");
		} else if (type == GL_SAMPLER_2D){
			printf("SAMPLER_2D\n");
		}
	}

	free(name);

	return n;
}

//////////////////////////////////////////////////////////////////////////
// methods

//////////////////////////////////////////////////////////////////////////
// fixedpipeline
float* point_verts;

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
	point_verts = (float*)calloc(3*NUM_PONTS,4);
	make_point();
}

void update1(float time){
	update_verts(time);
}

void draw1(){    
#ifndef __ANDROID__
	int i;
	// 1 //mesa 161 fps
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glBegin(GL_POINTS);
	for (i=0;i<NUM_PONTS*3;i+=3){
		glColor4ubv((GLubyte*)(point_verts+i+2));
		glVertex2f(point_verts[i],point_verts[i+1]);
	}
	glEnd();
#else
	//glPointSizePointerOES(GL_FLOAT,12,point_verts);//1
	//glVertexPointer(2, GL_FLOAT, 12, point_verts);//2
	//glDrawArrays(GL_POINTS, 0, NUM_PONTS);
	//glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

void deinit1(){
	free(point_verts);
	free(points);
}

//////
// point_gl11
GLuint point_gl11_prog;

void init2_shader(){
	const char* point_gl11_vert_src=
		"void main(){"
		"	gl_FrontColor = gl_Color;"
		"	gl_Position = gl_Vertex;"
		"}";

	// gl_PointCoord #version 110 not work in my linux mesa
	const char* point_gl11_frag_src=
#ifndef _WIN32
		"#version 120\n"
#endif
		"uniform sampler2D u_tex;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*gl_Color;"
		"}";

	point_verts = (float*)calloc(3*NUM_PONTS,4);
	make_point();

	point_gl11_prog = creatProg(point_gl11_vert_src,point_gl11_frag_src);
	glEnableClientState(GL_VERTEX_ARRAY);
}

void init2(){
	point_verts = (float*)calloc(3*NUM_PONTS,4);
	make_point();

	glEnableClientState(GL_VERTEX_ARRAY);
}

void update2(float time){
	update_verts(time);
}

void draw2(){
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2,GL_FLOAT,12,point_verts);
	glColorPointer(4,GL_UNSIGNED_BYTE,12,point_verts+2);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);
}

void draw2_shader(){
	glUseProgram(point_gl11_prog);
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2,GL_FLOAT,12,point_verts);
	glColorPointer(4,GL_UNSIGNED_BYTE,12,point_verts+2);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);
}

void deinit2(){
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
}

void deinit2_shader(){
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
	glDeleteProgram(point_gl11_prog);
}

//////
// point_gl20
GLuint point_gl20_prog;
GLuint point_gl20_vbuffer;

void init3(){
	const char* point_gl20_vert_src=
		"attribute vec3 pos;"
		"uniform float time;"
		"const float NUM_POINTS="STR(NUM_PONTS)".0;"
		"varying vec4 v_col;"
		"void main(){"
		"	float p = fract(time + pos.z);"
		"	vec2 ps = p*pos.z/NUM_POINTS * pos.xy;"
		"	gl_Position = vec4(ps, 0.0, 1.0);"
		"	v_col = vec4(1.0, 1.0, 1.0, 1.0-p);"
		"}";

	const char* point_gl20_frag_src=
#ifndef _WIN32
		"#version 120\n"
#endif
		"uniform sampler2D u_tex;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*v_col;"
		"}";

	point_gl20_prog = creatProg(point_gl20_vert_src,point_gl20_frag_src);

	//GetUniforms(point_gl20_prog);

	make_point();

	glGenBuffers(1,&point_gl20_vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER,point_gl20_vbuffer);
	glBufferData(GL_ARRAY_BUFFER,12*NUM_PONTS,points,GL_STATIC_DRAW);
	
	free(points);
}

void update3(float time){
	glUseProgram(point_gl20_prog);
	glUniform1f(0, time);

	//glUniform1f(0, time);
	//glUniform1fvARB(0, 1, &time);
	//glUniform1fARB(0, time);
	// mot work in win10 with Intel(R) HD Graphics Ironlake-M - Build 8.15.10.2900 :(
}

void draw3(){
	glUseProgram(point_gl20_prog);
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,point_gl20_vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_POINTS,0,NUM_PONTS);
}

void deinit3(){
	glDeleteProgram(point_gl20_prog);
}

//////////////////////////////////////////////////////////////////////////
// Quads

float* quads_verts;
GLuint quds_prog;
// x,y,u,v,c
void init_quads(){
	int i,id=2;
	const char* quads_vert_src=
		"attribute vec4 pos;"
		"attribute vec4 col;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
		"	v_uv = pos.zw;"
		"	v_col = col;"
		"}";
	const char* quads_frag_src=
		"uniform sampler2D u_tex;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, v_uv) * v_col;"
		"}";
	quds_prog = creatProg(quads_vert_src,quads_frag_src);
	make_point();

	quads_verts = (float*)calloc(30*NUM_PONTS,4);
	for(i=0;i<NUM_PONTS;++i){
		quads_verts[id]=0;
		quads_verts[id+1]=1;
		id += 5;

		quads_verts[id]=0;
		quads_verts[id+1]=0;
		id += 5;

		quads_verts[id]=1;
		quads_verts[id+1]=1;
		id += 5;

		quads_verts[id]=1;
		quads_verts[id+1]=1;
		id += 5;

		quads_verts[id]=0;
		quads_verts[id+1]=0;
		id += 5;

		quads_verts[id]=1;
		quads_verts[id+1]=0;
		id += 5;
	}
}

void update_quads(float time){
	int i;
	union{
		unsigned char uc[4];
		float f;
	}color;
	float c[2];//center
	float l,r,t,b;
	int index=0;
	for (i=0;i<NUM_PONTS*3;i+=3){
		float frac = time+points[i+2];
		float p = frac - (long)frac;
		color.uc[0]=0xff;color.uc[1]=0xff;color.uc[2]=0xff;color.uc[3]=(GLubyte)((1-p)*0xff);
		c[0] = p*points[i+2]/NUM_PONTS * points[i];
		c[1] = p*points[i+2]/NUM_PONTS * points[i+1];
		//point_verts[i+2] = color.f;
		l = c[0]-0.032f;
		r = c[0]+0.032f;
		t = c[1]+0.032f;
		b = c[1]-0.032f;
		quads_verts[index]=l;
		quads_verts[index+1]=t;
		quads_verts[index+4]=color.f;
		index += 5;
		quads_verts[index]=l;
		quads_verts[index+1]=b;
		quads_verts[index+4]=color.f;
		index += 5;
		quads_verts[index]=r;
		quads_verts[index+1]=t;
		quads_verts[index+4]=color.f;
		index += 5;

		quads_verts[index]=r;
		quads_verts[index+1]=t;
		quads_verts[index+4]=color.f;
		index += 5;
		quads_verts[index]=l;
		quads_verts[index+1]=b;
		quads_verts[index+4]=color.f;
		index += 5;
		quads_verts[index]=r;
		quads_verts[index+1]=b;
		quads_verts[index+4]=color.f;
		index += 5;
	}
}

void draw_quads(){
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glUseProgram(quds_prog);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, quads_verts);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, quads_verts+4);
	glDrawArrays(GL_TRIANGLES,0,NUM_PONTS*6);
	glDisableVertexAttribArray(1);
}

void deinit_quads(){
	free(quads_verts);
	free(points);
}


//////////////////////////////////////////////////////////////////////////
// main
int main(int argc, char **argv){
	// convert image to raw data
	/*stbi_uc* img_data;
	int x,y;

	img_data = stbi_load("../flare.png",&x,&y,0,0);
	f=fopen("../flare.rgba","wb");
	fwrite(img_data,x*y*4,1,f);
	fclose(f);*/

	// setup time
#ifdef _WIN32
	LARGE_INTEGER tps;
	LARGE_INTEGER queryTime;
	QueryPerformanceFrequency(&tps);
	__timeTicksPerMillis = (double)(tps.QuadPart / 1000L);
	QueryPerformanceCounter(&queryTime);
	__timeStart = queryTime.QuadPart / __timeTicksPerMillis;
#elif __linux
	clock_gettime(CLOCK_REALTIME, &__timespec);
	__timeStart = timespec2millis(&__timespec);
	__timeAbsolute = 0L;
#endif

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
#ifndef __ANDROID__
	gladLoadGL();
#endif

	loadFont();

	// init methods 
	// FIXME: use list or dynamic array
	methods[0].init = init1;
	methods[0].update = update1;
	methods[0].draw = draw1;
	methods[0].deinit = deinit1;
	methods[0].name = "glBegin(GL_POINTS)/glEnd()";

	methods[1].init = init2;
	methods[1].update = update2;
	methods[1].draw = draw2;
	methods[1].deinit = deinit2;
	methods[1].name = "glVertexPointer()";

	methods[2].init = init2_shader;
	methods[2].update = update2;
	methods[2].draw = draw2_shader;
	methods[2].deinit = deinit2_shader;
	methods[2].name = "glVertexPointer()/simple shader";

	methods[3].init = init3;
	methods[3].update = update3;
	methods[3].draw = draw3;
	methods[3].deinit = deinit3;
	methods[3].name = "VBO compute in shader";

	methods[4].init = init_quads;
	methods[4].update = update_quads;
	methods[4].draw = draw_quads;
	methods[4].deinit = deinit_quads;
	methods[4].name = "Quad from 2 triangle";

	// FIXME: !!!
	tt[0].name = methods[0].name;
	tt[1].name = methods[1].name;
	tt[2].name = methods[2].name;
	tt[3].name = methods[3].name;
	tt[4].name = methods[4].name;

#ifdef _WIN32
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("wglSwapIntervalEXT");
#elif __linux
	// TODO: ARB EXT
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("glXSwapIntervalMESA");
#endif
	glSwapInterval(0);

	glClearColor(0.f, 0.0f, 0.f, 1.f);

	glGenTextures(1,&sprite_tex);
	glBindTexture(GL_TEXTURE_2D,sprite_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
#ifdef EMBEDDED_DATA
	{
#include "flare_rgba.h"
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare_rgba);
	}
#else
	{GLubyte* img_data;
	FILE* f = fopen("../flare.rgba","rb");
	img_data = (GLubyte*)malloc(64*64*4);
	fread(img_data,1,64*64*4,f);
	fclose(f);    
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
	free(img_data);}
#endif

	glEnable(GL_TEXTURE_2D);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef __ANDROID__
	glEnable(GL_POINT_SPRITE);
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
#else
	glEnable(GL_POINT_SPRITE_OES);
	glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
#endif
	//glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT); // default GL_LOWER_LEFT

	// drawing mode for point sprites
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glPointSize(19.f);

	methods[curr_method].timer_init.start = (float)seTime();
	methods[curr_method].init();
	t_stop(&methods[curr_method].timer_init,(float)seTime());

	glutMainLoop();
	return 0;
}

void Display(void){
	Method *m;
	glClear(GL_COLOR_BUFFER_BIT);

	m = &methods[curr_method];
	m->timer_draw.start = (float)seTime();
	m->draw();
	t_stop(&m->timer_draw,(float)seTime());

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
int first_circle=0;
void Idle(void){
	int timeInterval;
	float elapsed,time;

	++frameCount;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	timeInterval = currentTime - previousTime;
	time = currentTime/1000.f;
	elapsed = time - lastTime;
	lastTime = time;

	methods[curr_method].timer_update.start = (float)seTime();
	methods[curr_method].update(time);
	t_stop(&methods[curr_method].timer_update,(float)seTime());

	if(timeInterval > 1000){
		Method* m;
		char str[16];
		sprintf(str,"Fps: %d %d\n",frameCount,curr_method);
		makeText(str);

#if OUTPUT_FPS
#ifdef _MSC_VER
		OutputDebugStringA(str);
#else
		printf("%s",str);
#endif
#endif

		m = &methods[curr_method];
		
		m->timer_deinit.start = (float)seTime();
		m->deinit();
		t_stop(&m->timer_deinit,(float)seTime());

		if (first_circle){
			float i = t_getAvg(&m->timer_init);
			float u = t_getAvg(&m->timer_update);
			float d = t_getAvg(&m->timer_draw);
			float de = t_getAvg(&m->timer_deinit);
			tt_set(&tt[curr_method],i,u,d,de,frameCount);
			if (tt[curr_method].count>SECONDS_PER_METHOD){
				tt_write();
			}
		}
#if CYCLE_METODS
		++curr_method;
#endif
		if (curr_method>=NUM_METHODS) {
			curr_method = 0;
#if WRITE_RESULT
			first_circle = 1;
#endif
		}

		m = &methods[curr_method];
		m->timer_init.start = (float)seTime();
		m->init();
		t_stop(&m->timer_init,(float)seTime());

		m->timer_update.start = (float)seTime();
		m->update(time);
		t_stop(&m->timer_update,(float)seTime());

		previousTime = currentTime;
		frameCount = 0;
	}

	glutPostRedisplay();
}
