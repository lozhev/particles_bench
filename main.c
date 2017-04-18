#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#ifndef WINAPI_FAMILY_SYSTEM
#ifndef __ANDROID__
#include "glad/glad.h"
#endif
#include <GL/freeglut.h>
// for WIN32
#undef WINAPI_FAMILY_SYSTEM
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h> //GL_OES_get_program_binary
#include <stdio.h>
#include <stdlib.h>
#define EMBEDDED_DATA
#endif

//#define STB_IMAGE_IMPLEMENTATION
//#include "../stb/stb_image.h"

#define NUM_PONTS 1600
#define SECONDS_PER_METHOD 10
#define CURRENT_METHOD 0
#define WRITE_RESULT 1
#define CYCLE_METODS 1
#define OUTPUT_FPS 0

// shared data
typedef struct {
	float start;
	float accum;
	int counter;
} Timer;

typedef struct {
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
	int memsize[2];
} Table_time;

typedef struct {
	void (*init)();
	void (*update)(float);
	void (*draw)();
	void (*deinit)();
	Timer timer_init;
	Timer timer_update;
	Timer timer_draw;
	Timer timer_deinit;
	char* name;
} Method;

float* points;
GLuint sprite_tex;
int curr_method = CURRENT_METHOD;
int num_methods = 0;
Method methods[10];//
Table_time tt[10];

//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#ifndef WINAPI_FAMILY_SYSTEM
typedef BOOL(APIENTRYP PFNWGLSWAPINTERVALEXTPROC)(int interval);
PFNWGLSWAPINTERVALEXTPROC glSwapInterval;

typedef void (APIENTRYP PFNGLUNIFORM1FARBPROC)(GLint location, GLint count, GLfloat* v0);
PFNGLUNIFORM1FARBPROC glUniform1fARB;
#endif // !WINAPI_FAMILY_SYSTEM


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

#ifdef __ANDROID__
#include <android/log.h>
extern void print(const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	__android_log_vprint(ANDROID_LOG_INFO, "native-activity", format, argptr);
	va_end(argptr);
}

#define PRECISION_FLOAT "precision lowp float;"
#define FRAG_VERSION
//#define OPENGL_ES
#define EMBEDDED_DATA
#elif _WIN32
extern void print(const char* format, ...) {
	va_list argptr;
	char buf[256];
	va_start(argptr, format);
	vsprintf(buf, format, argptr);
	OutputDebugStringA(buf);
	va_end(argptr);
}
#ifdef WINAPI_FAMILY_SYSTEM
#define PRECISION_FLOAT "precision lowp float;"
#else
#define PRECISION_FLOAT
#endif
#define FRAG_VERSION
#elif __linux
extern void print(const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);
#define PRECISION_FLOAT
#define FRAG_VERSION "#version 120\n"
}
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void Display(void);
void Idle(void);

// Timer
void t_start(Timer* t, float start) {
	t->start = start;
}

float t_stop(Timer* t, float stop) {
	float dt = stop - t->start;
	t->accum += dt;
	++t->counter;
	return dt;
}
float t_getAvg(Timer* t) {
	float ret = t->accum / (float)t->counter;
	t->accum = 0.f;
	t->counter = 0;
	return ret;
}

// Table_time
void tt_set(Table_time* tt, float i, float u, float d, float de, int f) {
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

int tt_cmp_fps(const void* lhs, const void* rhs) {
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lf = l->acc_fps / (float)l->count;
	float rf = r->acc_fps / (float)r->count;
	if (lf < rf) return  1;
	if (lf > rf) return -1;
	return 0;
}

int tt_cmp_time(const void* lhs, const void* rhs) {
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lt = (l->acc_init + l->acc_update + l->acc_draw + l->acc_deinit) / (float)l->count;
	float rt = (r->acc_init + r->acc_update + r->acc_draw + r->acc_deinit) / (float)r->count;
	if (lt < rt) return -1;
	if (lt > rt) return  1;
	return 0;
}

int tt_cmp_runtime(const void* lhs, const void* rhs) {
	Table_time* l = (Table_time*)lhs;
	Table_time* r = (Table_time*)rhs;
	float lt = (l->acc_update + l->acc_draw) / (float)l->count;
	float rt = (r->acc_update + r->acc_draw) / (float)r->count;
	if (lt < rt) return -1;
	if (lt > rt) return  1;
	return 0;
}

void tt_write() {
	int i, n;
#ifndef WINAPI_FAMILY_SYSTEM
	FILE* f = fopen("results.txt", "w");
#else
	// name like c:\Users\<user>\AppData\Local\Packages\00aa691b-9586-405f-b70c-ab29e58a9c49_0ys5whghx6k26\LocalState\result.txt
	// cant get in phone
	//extern const wchar_t* savefilename();
	//FILE* f = _wfopen(savefilename(),L"w");
#define fwrite(_str,_size,_count,_file) print("%s",_str)
#endif
	char* str = (char*)malloc(256);

	n = sprintf(str, "vendor: %s\n", glGetString(GL_VENDOR));
	fwrite(str, 1, n, f);
	n = sprintf(str, "renderer: %s\n", glGetString(GL_RENDERER));
	fwrite(str, 1, n, f);
	n = sprintf(str, "version: %s\n", glGetString(GL_VERSION));
	fwrite(str, 1, n, f);
	n = sprintf(str, "points: %d\n\n", NUM_PONTS);
	fwrite(str, 1, n, f);

	n = sprintf(str, "%s\n", "fps:");
	fwrite(str, 1, n, f);
	qsort(tt, num_methods, sizeof(Table_time), tt_cmp_fps);
	for (i = 0; i < num_methods; ++i) {
		n = sprintf(str, "%7.2f ", tt[i].acc_fps / (float)tt[i].count);
		fwrite(str, 1, n, f);

		n = sprintf(str, "%s\n", tt[i].name);
		fwrite(str, 1, n, f);

		/*n = sprintf(str, "memsize  %d kb( %d kb / %d kb )\n",
					(tt[i].memsize[0] + tt[i].memsize[1]) / 1024, tt[i].memsize[0] / 1024, tt[i].memsize[1] / 1024);
		fwrite(str, 1, n, f);*/
	}

	n = sprintf(str, "\n%s", "cpu time millisecond\n");
	fwrite(str, 1, n, f);

	n = sprintf(str, "\n| %s       |", "all");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s      |", "init");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s    |", "update");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s      |", "draw");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s    |\n", "deinit");
	fwrite(str, 1, n, f);

	for (i = 0; i < 61; ++i) str[i] = '-';
	str[i] = '\n'; str[i + 1] = '\0';
	fwrite(str, 1, 62, f);

	qsort(tt, num_methods, sizeof(Table_time), tt_cmp_time);
	for (i = 0; i < num_methods; ++i) {
		float it = tt[i].acc_init / (float)tt[i].count;
		float iu = tt[i].acc_update / (float)tt[i].count;
		float id = tt[i].acc_draw / (float)tt[i].count;
		float ide = tt[i].acc_deinit / (float)tt[i].count;
		float all_time = it + iu + id + ide;
		n = sprintf(str, "| %9.5f | %9.5f | %9.5f | %9.5f | %9.5f |", all_time, it, iu, id, ide);
		fwrite(str, 1, n, f);

		n = sprintf(str, " %s\n", tt[i].name);
		fwrite(str, 1, n, f);
	}

	n = sprintf(str, "\n| %s   |", "runtime");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s    |", "update");
	fwrite(str, 1, n, f);
	n = sprintf(str, " %s      |\n", "draw");
	fwrite(str, 1, n, f);

	for (i = 0; i < 37; ++i) str[i] = '-';
	str[i] = '\n'; str[i + 1] = '\0';
	fwrite(str, 1, 38, f);

	qsort(tt, num_methods, sizeof(Table_time), tt_cmp_runtime);
	for (i = 0; i < num_methods; ++i) {
		float iu = tt[i].acc_update / (float)tt[i].count;
		float id = tt[i].acc_draw / (float)tt[i].count;
		float all_time = iu + id;
		n = sprintf(str, "| %9.5f | %9.5f | %9.5f |", all_time, iu, id);
		fwrite(str, 1, n, f);

		n = sprintf(str, " %s\n", tt[i].name);
		fwrite(str, 1, n, f);
	}
#ifdef WINAPI_FAMILY_SYSTEM
#undef fwrite
#else
	fclose(f);
#endif

	free(str);

	//memset(tt, 0, sizeof(tt));
	exit(0);
	//make exit!!
}





//////////////////////////////////////////////////////////////////////////
// font

const char* fnt_vert_src = "\
attribute vec4 pos;\
varying vec2 v_uv;\
\
void main()\
{\
	gl_Position = vec4(pos.xy, 0.0, 1.0);\
	v_uv = pos.zw;\
}";

const char* fnt_frag_src =
	PRECISION_FLOAT
	"uniform sampler2D u_tex;"
	"varying vec2 v_uv;"
	"void main(){"
	"	gl_FragColor = texture2D(u_tex, v_uv);"
	"}";

typedef struct {
	float x0, y0;
	float x1, y1;
	float ratio;
} Character;

typedef struct {
	float position[2];
	float texCoord[2];
} TexVertex;

Character* fnt_chars = 0;
static const GLubyte* fnt_table = 0;
GLuint fnt_tex = 0;
GLuint fnt_prog = 0;
int fnt_textquads = 0;
TexVertex* fnt_verts = 0;
int fnt_verts_count = 0;
int fnt_verts_drawcount = 0;

#define SETVEC2(v,x,y)\
	v[0]=x;\
	v[1]=y

void fillTextBuffer(TexVertex* dest, const char* str, float x, float y, const float charWidth, const float charHeight) {
	float startx = x;

	while (*str) {
		if (*str == '\n') {
			y += charHeight;
			x = startx;
		} else {
			GLubyte ch_id = fnt_table[*(unsigned char*) str];
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

void makeText(const char* str) {
	int n = 0;
	const char* s = str;
	while (*s) {
		if (*s != '\n') ++n;
		++s;
	}

	n *= 6;
	if (n > fnt_verts_count) {
		fnt_verts = (TexVertex*)realloc(fnt_verts, n * sizeof(TexVertex));
		fnt_verts_count = n;
	}
	fnt_verts_drawcount = n;
	fillTextBuffer(fnt_verts, str, -1.0f, 0.9f, 0.08f, 0.1f);
}

#if 0
static const char failed_compile_str[] = "failed compile: %s\ninfo:%s\n";
static const char failed_link_str[] = "failed link: %s\n";
#define CHECKSHADER(shd,src)\
	glGetShaderiv(shd, GL_COMPILE_STATUS, &success);\
	if (success != GL_TRUE) {\
	GLchar log[256];\
	glGetShaderInfoLog(shd, 256, NULL, log);\
	print(failed_compile_str,src,log);\
	glDeleteShader(shd);\
	return -1;\
	}
#define CHECKPROGRAM(p)\
	glGetProgramiv(p,GL_LINK_STATUS,&success);\
	if (success != GL_TRUE) {\
	GLchar log[256];\
	glGetShaderInfoLog(p, 256, NULL, log);\
	print(failed_link_str,log);\
	glDeleteProgram(p);\
	return -1;\
	}
#define INT_SUCCSESS GLint success;
#else
#define CHECKSHADER(shd,src)
#define CHECKPROGRAM(p)
#define INT_SUCCSESS
#endif

GLuint creatProg(const char* vert_src, const char* frag_src) {
	INT_SUCCSESS
	GLuint prog, vert_id, frag_id;

	vert_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_id, 1, &vert_src, 0);
	glCompileShader(vert_id);
	CHECKSHADER(vert_id, vert_src);

	frag_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_id, 1, &frag_src, 0);
	glCompileShader(frag_id);
	CHECKSHADER(frag_id, frag_src);

	prog = glCreateProgram();
	glAttachShader(prog, vert_id);
	glAttachShader(prog, frag_id);
	glLinkProgram(prog);
	CHECKPROGRAM(prog);

	glDeleteShader(vert_id);
	glDeleteShader(frag_id);

	return prog;
}


void loadFont() {
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
	{
		int i;
		FILE* f = fopen("Fonts/Future.fnt", "rb");
		fnt_table = (GLubyte*)malloc(256);
		fread((void*)fnt_table, 1, 256, f);
		fread(&i, 4, 1, f);
		fnt_chars = (Character*)malloc(sizeof(Character) * i);
		fread(fnt_chars, sizeof(Character), i, f);
		fclose(f);
	}
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

	glGenTextures(1, &fnt_tex);
	glBindTexture(GL_TEXTURE_2D, fnt_tex);
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
		fread(pixels, 1, size, f);
		fclose(f);

		glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x83f1, 512, 512, 0, size, pixels);
		free(pixels);
	}
#endif
	makeText("Fps:");

	fnt_prog = creatProg(fnt_vert_src, fnt_frag_src);
}



GLuint GetUniforms(GLuint program) {
	GLint i, n;
	//GLint max;
	//char* name;
	char name[16];

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
	//glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);
	//name = (char*)malloc(max);

	for (i = 0; i < n; ++i) {
		GLint size, len, loc;
		GLenum type;

		glGetActiveUniform(program, i, 10, &len, &size, &type, name);

		loc = glGetUniformLocation(program, name);
		print("%d: %s ", i, name);
		if (type == GL_FLOAT) {
			print("float\n");
		} else if (type == GL_SAMPLER_2D) {
			print("SAMPLER_2D\n");
		}
	}

	//free(name);

	return n;
}

//////////////////////////////////////////////////////////////////////////
// methods

//////////////////////////////////////////////////////////////////////////
// fixedpipeline
float* point_verts;

void make_point() {
	int i;
	points = (float*)malloc(12 * NUM_PONTS);
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float a = 2.f * (float)M_PI * rand() / (float)RAND_MAX;;
		points[i] = sinf(a);
		points[i + 1] = cosf(a);
		points[i + 2] = NUM_PONTS * (float)rand() / (float)RAND_MAX;
	}
}

#if defined(__ANDROID__) || !defined(WINAPI_FAMILY_SYSTEM)
void update_verts(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = {{0xff, 0xff, 0xff, 0xff}};
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		point_verts[i] = p * points[i + 2] / NUM_PONTS * points[i];
		point_verts[i + 1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		point_verts[i + 2] = color.f;
	}
}
#endif

#if !defined(__ANDROID__) && !defined(WINAPI_FAMILY_SYSTEM)
void init1() {
	point_verts = (float*)calloc(3 * NUM_PONTS, 4);
	make_point();
}

void update1(float time) {
	update_verts(time);
}

void draw1() {
	int i;
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glBegin(GL_POINTS);
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		glColor4ubv((GLubyte*)(point_verts + i + 2));
		glVertex2f(point_verts[i], point_verts[i + 1]);
	}
	glEnd();
}

void deinit1() {
	free(point_verts);
	free(points);
}
#endif

#if !defined(WINAPI_FAMILY_SYSTEM)
//////
// point_gl11
// its work on opengles 1.0??
GLuint point_gl11_prog;

void init2_shader() {
	const char point_gl11_vert_src[] =
		"void main(){"
		"	gl_FrontColor = gl_Color;"
		"	gl_Position = gl_Vertex;"
		"}";

	// gl_PointCoord #version 110 not work in my linux mesa
	const char point_gl11_frag_src[] =
		FRAG_VERSION
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*gl_Color;"
		"}";

	point_verts = (float*)calloc(3 * NUM_PONTS, 4);
	make_point();

	point_gl11_prog = creatProg(point_gl11_vert_src, point_gl11_frag_src);
	glEnableClientState(GL_VERTEX_ARRAY);
}

void init2() {
	point_verts = (float*)calloc(3 * NUM_PONTS, 4);
	make_point();

	glEnableClientState(GL_VERTEX_ARRAY);
}

void update2(float time) {
	update_verts(time);
}

void draw2() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 12, point_verts);
	glColorPointer(4, GL_UNSIGNED_BYTE, 12, point_verts + 2);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

void draw2_shader() {
	glUseProgram(point_gl11_prog);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 12, point_verts);
	glColorPointer(4, GL_UNSIGNED_BYTE, 12, point_verts + 2);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

void deinit2() {
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
}

void deinit2_shader() {
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
	glDeleteProgram(point_gl11_prog);
}
#endif

//////
// point_gl20
GLuint point_gl20_prog;
GLuint point_gl20_vbuffer;

void init3() {
	const char point_gl20_vert_src[] =
		"attribute vec3 pos;"
		"uniform float time;"
		"const float NUM_POINTS="STR(NUM_PONTS)".0;"
		"varying vec4 v_col;"
		"void main(){"
		"	float p = fract(time + pos.z);"
		"	vec2 ps = p*pos.z/NUM_POINTS * pos.xy;"
		"	gl_Position = vec4(ps, 0.0, 1.0);"
		"	v_col = vec4(1.0, 1.0, 1.0, 1.0-p);"
#ifdef WINAPI_FAMILY_SYSTEM
		"	gl_PointSize = 19.0;"
#endif
		"}";

	const char point_gl20_frag_src[] =
		FRAG_VERSION
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*v_col;"
		"}";

	point_gl20_prog = creatProg(point_gl20_vert_src, point_gl20_frag_src);
	/*
		//GetUniforms(point_gl20_prog);
		{
	#define	GL_PROGRAM_BINARY_RETRIEVABLE_HINT             0x8257
	#define	GL_PROGRAM_BINARY_LENGTH                       0x8741
	#define	GL_NUM_PROGRAM_BINARY_FORMATS                  0x87FE
	#define	GL_PROGRAM_BINARY_FORMATS                      0x87FF

	typedef void (APIENTRYP PFNGLGETPROGRAMBINARYPROC)  ( GLuint program, GLsizei bufSize, GLsizei * length, GLenum *binaryFormat, GLvoid * binary );
	typedef void (APIENTRYP PFNGLPROGRAMBINARYPROC)     ( GLuint program, GLenum binaryFormat, const GLvoid * binary, GLsizei length );
	typedef void (APIENTRYP PFNGLPROGRAMPARAMETERIPROC) ( GLuint program, GLenum pname, GLint value );
		PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
		PFNGLPROGRAMBINARYPROC glProgramBinary;
		PFNGLPROGRAMPARAMETERIPROC glProgramParameteri;

		GLint   binaryLength;
		GLint   numFormats;;
		GLenum binaryFormat;
		void* binary;
		FILE* f;

		glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)glutGetProcAddress("glGetProgramBinary");
		glProgramBinary = (PFNGLPROGRAMBINARYPROC)glutGetProcAddress("glProgramBinary");
		glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)glutGetProcAddress("glProgramParameteri");
		print("glGetProgramBinary: %p glProgramBinary: %p glProgramParameteri: %p\n",
			glGetProgramBinary, glProgramBinary, glProgramParameteri
		);

		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,&numFormats);
		print("numFormats: %d\n",numFormats);

		glGetProgramiv ( point_gl20_prog, GL_PROGRAM_BINARY_LENGTH, &binaryLength );
		print("binaryLength: %d\n",binaryLength);
		if(binaryLength){
		binary = malloc(binaryLength);

		glGetProgramBinary ( point_gl20_prog, binaryLength, NULL, &binaryFormat, binary );

		f = fopen("shader.bin","wb");
		fwrite(binary,1,binaryLength,f);
		fclose(f);
		print("ok\n");

		free(binary);
		}

		}
	*/
	make_point();

	glGenBuffers(1, &point_gl20_vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, point_gl20_vbuffer);
	glBufferData(GL_ARRAY_BUFFER, 12 * NUM_PONTS, points, GL_STATIC_DRAW);

	free(points);
}

void update3(float time) {
	glUseProgram(point_gl20_prog);
	glUniform1f(0, time);

	//glUniform1f(0, time);
	//glUniform1fvARB(0, 1, &time);
	//glUniform1fARB(0, time);
	// mot work in win10 with Intel(R) HD Graphics Ironlake-M - Build 8.15.10.2900 :(
}

void draw3() {
	glUseProgram(point_gl20_prog);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, point_gl20_vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

void deinit3() {
	glDeleteProgram(point_gl20_prog);
	glDeleteBuffers(1, &point_gl20_vbuffer);
}

//////////////////////////////////////////////////////////////////////////
// Quads

float* quads_verts;
GLuint quads_prog;
// x,y,u,v,c
void init_quads() {
	int i, id = 2;
	const char quads_vert_src[] =
		"attribute vec4 pos;"
		"attribute vec4 col;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
		"	v_uv = pos.zw;"
		"	v_col = col;"
		"}";
	const char quads_frag_src[] =
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, v_uv) * v_col;"
		"}";
	quads_prog = creatProg(quads_vert_src, quads_frag_src);
	make_point();

	//GetUniforms(quads_prog);

	quads_verts = (float*)calloc(30 * NUM_PONTS, 4);
	for (i = 0; i < NUM_PONTS; ++i) {
		quads_verts[id] = 0;
		quads_verts[id + 1] = 1;
		id += 5;

		quads_verts[id] = 0;
		quads_verts[id + 1] = 0;
		id += 5;

		quads_verts[id] = 1;
		quads_verts[id + 1] = 1;
		id += 5;

		quads_verts[id] = 1;
		quads_verts[id + 1] = 1;
		id += 5;

		quads_verts[id] = 0;
		quads_verts[id + 1] = 0;
		id += 5;

		quads_verts[id] = 1;
		quads_verts[id + 1] = 0;
		id += 5;
	}
}

void update_quads(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = {{0xff, 0xff, 0xff, 0xff}};
	float c[2];//center
	float l, r, t, b;
	int index = 0;
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		c[0] = p * points[i + 2] / NUM_PONTS * points[i];
		c[1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		//point_verts[i+2] = color.f;
		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;
		quads_verts[index] = l;
		quads_verts[index + 1] = t;
		quads_verts[index + 4] = color.f;
		index += 5;
		quads_verts[index] = l;
		quads_verts[index + 1] = b;
		quads_verts[index + 4] = color.f;
		index += 5;
		quads_verts[index] = r;
		quads_verts[index + 1] = t;
		quads_verts[index + 4] = color.f;
		index += 5;

		quads_verts[index] = r;
		quads_verts[index + 1] = t;
		quads_verts[index + 4] = color.f;
		index += 5;
		quads_verts[index] = l;
		quads_verts[index + 1] = b;
		quads_verts[index + 4] = color.f;
		index += 5;
		quads_verts[index] = r;
		quads_verts[index + 1] = b;
		quads_verts[index + 4] = color.f;
		index += 5;
	}
}

void draw_quads() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, quads_verts);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, quads_verts + 4);
	glDrawArrays(GL_TRIANGLES, 0, NUM_PONTS * 6);
	glDisableVertexAttribArray(1);
}

void deinit_quads() {
	free(quads_verts);
	free(points);

	glDeleteProgram(quads_prog);
}

//////////////////////////////////////////////////////////////////////////
// Quads buffers

float* quads_verts_indexbufer; // x,y,u,v,c
GLuint quads_prog_indexbufer;
GLuint quads_vbufer;
GLuint quads_ibufer;

void init_quads_bufer() {
	int i, id = 2;
	unsigned short* ib;
	const char* quads_vert_src =
		"attribute vec4 pos;"
		"attribute vec4 col;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
		"	v_uv = pos.zw;"
		"	v_col = col;"
		"}";
	const char* quads_frag_src =
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, v_uv) * v_col;"
		"}";
	quads_prog_indexbufer = creatProg(quads_vert_src, quads_frag_src);
	make_point();

	//GetUniforms(quads_prog);

	quads_verts_indexbufer = (float*)calloc(20 * NUM_PONTS, 4);
	for (i = 0; i < NUM_PONTS; ++i) {
		quads_verts_indexbufer[id + 1] = 1;

		quads_verts_indexbufer[id + 10] = 1;
		quads_verts_indexbufer[id + 11] = 1;

		quads_verts_indexbufer[id + 15] = 1;
		id += 20;
	}
	glGenBuffers(1, &quads_vbufer);
	glBindBuffer(GL_ARRAY_BUFFER, quads_vbufer);
	glBufferData(GL_ARRAY_BUFFER, 80 * NUM_PONTS, quads_verts_indexbufer, GL_STATIC_DRAW);

	// 6 inds * sizeof(short)
	ib = (unsigned short*)malloc(NUM_PONTS * 12);
	for (i = 0; i < NUM_PONTS; ++i) {
		int it = i * 4;
		id = i * 6;
		ib[id] = it;
		ib[id + 1] = it + 1;
		ib[id + 2] = it + 2;
		ib[id + 3] = it + 2;
		ib[id + 4] = it + 1;
		ib[id + 5] = it + 3;
	}

	glGenBuffers(1, &quads_ibufer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_ibufer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_PONTS * 12, ib, GL_STATIC_DRAW);

	free(ib);
}

void init_quads_bufer2() {
	int i, id = 2;
	unsigned short* ib, *iptr;
	const char* quads_vert_src =
		"attribute vec4 pos;"
		"attribute vec4 col;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
		"	v_uv = pos.zw;"
		"	v_col = col;"
		"}";
	const char* quads_frag_src =
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"varying vec2 v_uv;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, v_uv) * v_col;"
		"}";
	quads_prog_indexbufer = creatProg(quads_vert_src, quads_frag_src);
	make_point();

	//GetUniforms(quads_prog);

	quads_verts_indexbufer = (float*)calloc(20 * NUM_PONTS, 4);
	for (i = 0; i < NUM_PONTS; ++i) {
		quads_verts_indexbufer[id + 1] = 1;

		quads_verts_indexbufer[id + 10] = 1;
		quads_verts_indexbufer[id + 11] = 1;

		quads_verts_indexbufer[id + 15] = 1;
		id += 20;
	}
	glGenBuffers(1, &quads_vbufer);
	glBindBuffer(GL_ARRAY_BUFFER, quads_vbufer);
	glBufferData(GL_ARRAY_BUFFER, 80 * NUM_PONTS, quads_verts_indexbufer, GL_STATIC_DRAW);

	// 4 tri * NUM_PONTS * 1.5 * sizeof(short);
	// NUM_PONTS * 12
	id = ((NUM_PONTS * 4)/2 + (NUM_PONTS * 4)) * 2;
	ib = (unsigned short*)malloc(id);
	iptr = ib;
	for (i = 0; i < NUM_PONTS * 4; ++i) {
		*iptr = i;
		++iptr;
		if ((i % 4) - 3 == 0) {
			*iptr = i;
			++iptr;
			*iptr = i + 1;
			++iptr;
		}
	}

	glGenBuffers(1, &quads_ibufer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_ibufer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, id, ib, GL_STATIC_DRAW);

	free(ib);
}

void update_quads_bufer(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = {{0xff, 0xff, 0xff, 0xff}};
	float c[2];//center
	float l, r, t, b;
	int index = 0;
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		c[0] = p * points[i + 2] / NUM_PONTS * points[i];
		c[1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		//point_verts[i+2] = color.f;
		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;
		quads_verts_indexbufer[index] = l;
		quads_verts_indexbufer[index + 1] = t;
		quads_verts_indexbufer[index + 4] = color.f;
		index += 5;
		quads_verts_indexbufer[index] = l;
		quads_verts_indexbufer[index + 1] = b;
		quads_verts_indexbufer[index + 4] = color.f;
		index += 5;
		quads_verts_indexbufer[index] = r;
		quads_verts_indexbufer[index + 1] = t;
		quads_verts_indexbufer[index + 4] = color.f;
		index += 5;

		quads_verts_indexbufer[index] = r;
		quads_verts_indexbufer[index + 1] = b;
		quads_verts_indexbufer[index + 4] = color.f;
		index += 5;
	}
	glBindBuffer(GL_ARRAY_BUFFER, quads_vbufer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 80 * NUM_PONTS, quads_verts_indexbufer);
}

void draw_quads_bufer() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog_indexbufer);
	//glBindBuffer(GL_ARRAY_BUFFER,quads_vbufer);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quads_ibufer);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLES, NUM_PONTS * 6, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(1);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void draw_quads_bufer2() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog_indexbufer);
	//glBindBuffer(GL_ARRAY_BUFFER,quads_vbufer);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quads_ibufer);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLE_STRIP, (NUM_PONTS - 1) * 6 + 4, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(1);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void deinit_quads_bufer() {
	free(quads_verts_indexbufer);
	free(points);

	glDeleteBuffers(1, &quads_vbufer);
	glDeleteBuffers(1, &quads_ibufer);

	glDeleteProgram(quads_prog_indexbufer);
}

//////////////////////////////////////////////////////////////////////////
// main
int main(int argc, char** argv) {
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

#ifndef WINAPI_FAMILY_SYSTEM
	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitContextVersion(2, 0);
	//glutInitContextFlags   ( GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG );
	//glutInitContextProfile ( GLUT_CORE_PROFILE );
	glutCreateWindow("test");

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
#ifndef __ANDROID__
	gladLoadGL();
#endif
#endif

	loadFont();

	// init methods
	// FIXME: use list or dynamic array
	num_methods = 0;
#if !defined(__ANDROID__) && !defined(WINAPI_FAMILY_SYSTEM)
	methods[num_methods].init = init1;
	methods[num_methods].update = update1;
	methods[num_methods].draw = draw1;
	methods[num_methods].deinit = deinit1;
	methods[num_methods].name = "glBegin(GL_POINTS)/glEnd()";
	tt[num_methods].memsize[0] = NUM_PONTS * 3 * 4 * 2;//(vtx_points) + (points)
	tt[num_methods].memsize[1] = 0;
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;
#endif
#if !defined(WINAPI_FAMILY_SYSTEM)
	methods[num_methods].init = init2;
	methods[num_methods].update = update2;
	methods[num_methods].draw = draw2;
	methods[num_methods].deinit = deinit2;
	methods[num_methods].name = "glVertexPointer()";
	tt[num_methods].memsize[0] = NUM_PONTS * 3 * 4 * 2;//(vtx_points) + (points)
	tt[num_methods].memsize[1] = 0;
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;

	methods[num_methods].init = init2_shader;
	methods[num_methods].update = update2;
	methods[num_methods].draw = draw2_shader;
	methods[num_methods].deinit = deinit2_shader;
	methods[num_methods].name = "glVertexPointer()/simple shader";
	tt[num_methods].memsize[0] = NUM_PONTS * 3 * 4 * 2;//(vtx_points) + (points)
	tt[num_methods].memsize[1] = 0;// compiled shader size??
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;
#endif
	methods[num_methods].init = init3;
	methods[num_methods].update = update3;
	methods[num_methods].draw = draw3;
	methods[num_methods].deinit = deinit3;
	methods[num_methods].name = "VBO compute in shader";
	tt[num_methods].memsize[0] = 0;
	tt[num_methods].memsize[1] = 12 * NUM_PONTS;// compiled shader size?? size point_gl20_vbuffer
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;

	methods[num_methods].init = init_quads;
	methods[num_methods].update = update_quads;
	methods[num_methods].draw = draw_quads;
	methods[num_methods].deinit = deinit_quads;
	methods[num_methods].name = "Quad from 2 triangle";
	tt[num_methods].memsize[0] = (30 * NUM_PONTS * 4) + (NUM_PONTS * 3 * 4); //(vtx) + (points)
	tt[num_methods].memsize[1] = 0;// compiled shader size??
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;

	methods[num_methods].init = init_quads_bufer;
	methods[num_methods].update = update_quads_bufer;
	methods[num_methods].draw = draw_quads_bufer;
	methods[num_methods].deinit = deinit_quads_bufer;
	methods[num_methods].name = "Quad from 2 triangle glbuffers";
	tt[num_methods].memsize[0] = (20 * NUM_PONTS * 4) + (NUM_PONTS * 3 * 4); //(vtx) + (points)
	tt[num_methods].memsize[1] = (20 * NUM_PONTS * 4) + (NUM_PONTS * 12); // compiled shader size?? (vbufer) + (ibufer)
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;

	methods[num_methods].init = init_quads_bufer2;
	methods[num_methods].update = update_quads_bufer;
	methods[num_methods].draw = draw_quads_bufer2;
	methods[num_methods].deinit = deinit_quads_bufer;
	methods[num_methods].name = "Quad from 2 triangle glbuffers tristrip";
	tt[num_methods].memsize[0] = (20 * NUM_PONTS * 4) + (NUM_PONTS * 3 * 4); //(vtx) + (points)
	tt[num_methods].memsize[1] = (20 * NUM_PONTS * 4) + ((NUM_PONTS * 4) / 2 + (NUM_PONTS * 4)) * 2; // compiled shader size?? (vbufer) + (ibufer)
	tt[num_methods].name = methods[num_methods].name;
	++num_methods;

	// FIXME: !!!
	/*tt[0].name = methods[0].name;
	tt[1].name = methods[1].name;
	tt[2].name = methods[2].name;
	tt[3].name = methods[3].name;
	tt[4].name = methods[4].name;*/

#if defined(__ANDROID__) || WINAPI_FAMILY_SYSTEM
	//patch freeglut
	//eglSwapInterval()
	//TODO: not use freeglut..
#else
#ifdef _WIN32
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("wglSwapIntervalEXT");
#elif __linux
	// TODO: ARB EXT
	glSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)glutGetProcAddress("glXSwapIntervalMESA");
#endif
	glSwapInterval(0);
#endif

	glClearColor(0.f, 0.0f, 0.f, 1.f);

	glGenTextures(1, &sprite_tex);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
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
	{
		GLubyte* img_data;
		FILE* f = fopen("../flare.rgba", "rb");
		img_data = (GLubyte*)malloc(64 * 64 * 4);
		fread(img_data, 1, 64 * 64 * 4, f);
		fclose(f);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
		free(img_data);
	}
#endif

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef WINAPI_FAMILY_SYSTEM
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
	t_stop(&methods[curr_method].timer_init, (float)seTime());

	glutMainLoop();
#else
	methods[curr_method].timer_init.start = (float)seTime();
	methods[curr_method].init();
	t_stop(&methods[curr_method].timer_init, (float)seTime());
#endif
	return 0;
}

void Display(void) {
	Method* m;
	glClear(GL_COLOR_BUFFER_BIT);

	m = &methods[curr_method];
	m->timer_draw.start = (float)seTime();
	m->draw();
	t_stop(&m->timer_draw, (float)seTime());

	// draw font
	glUseProgram(fnt_prog);
	glBindTexture(GL_TEXTURE_2D, fnt_tex);
	//glDisableClientState(GL_COLOR_ARRAY);
	//glVertexPointer(4,GL_FLOAT,0,fnt_verts);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, fnt_verts);
	glDrawArrays(GL_TRIANGLES, 0, fnt_verts_drawcount);
	glUseProgram(0);
	glDisableVertexAttribArray(0);

#ifndef WINAPI_FAMILY_SYSTEM
	glutSwapBuffers();
#endif
}

int frameCount = 0;
int currentTime = 0, previousTime = 0;
float lastTime = 0;
int first_circle = 0;
void Idle(void) {
	int timeInterval;
	float elapsed, time;

	++frameCount;
#ifndef WINAPI_FAMILY_SYSTEM
	currentTime = glutGet(GLUT_ELAPSED_TIME);
#else
	currentTime = (float)seTime();
#endif
	timeInterval = currentTime - previousTime;
	time = currentTime / 1000.f;
	elapsed = time - lastTime;
	lastTime = time;

	methods[curr_method].timer_update.start = (float)seTime();
	methods[curr_method].update(time);
	t_stop(&methods[curr_method].timer_update, (float)seTime());

	if (timeInterval > 1000) {
		Method* m;
		char str[16];
		sprintf(str, "Fps: %d %d\n", frameCount, curr_method);
		makeText(str);

#if OUTPUT_FPS
		print("%s", str);
#endif

		m = &methods[curr_method];

		m->timer_deinit.start = (float)seTime();
		m->deinit();
		t_stop(&m->timer_deinit, (float)seTime());

		if (first_circle) {
			float i = t_getAvg(&m->timer_init);
			float u = t_getAvg(&m->timer_update);
			float d = t_getAvg(&m->timer_draw);
			float de = t_getAvg(&m->timer_deinit);
			tt_set(&tt[curr_method], i, u, d, de, frameCount);
			if (tt[curr_method].count > SECONDS_PER_METHOD) {
				tt_write();
			}
		}
#if CYCLE_METODS
		++curr_method;
#endif
		if (curr_method >= num_methods) {
			curr_method = 0;
#if WRITE_RESULT
			first_circle = 1;
#endif
		}

		m = &methods[curr_method];
		m->timer_init.start = (float)seTime();
		m->init();
		t_stop(&m->timer_init, (float)seTime());

		m->timer_update.start = (float)seTime();
		m->update(time);
		t_stop(&m->timer_update, (float)seTime());

		previousTime = currentTime;
		frameCount = 0;
	}
#ifndef WINAPI_FAMILY_SYSTEM
	glutPostRedisplay();
#endif
}
