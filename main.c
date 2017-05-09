#include "pch.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "../stb/stb_image.h"

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

Character* fnt_chars;
GLubyte* fnt_table;
GLuint fnt_tex;
GLuint fnt_prog;
int fnt_textquads;
TexVertex* fnt_verts;
int fnt_verts_count;
int fnt_verts_drawcount;

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
#include "res/fnt_table.h"
#include "res/fnt_chars.h"
		fnt_table = (GLubyte*)Future_fnt_table;
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
		FILE* f = fopen("../res/Future2.fnt", "rb");
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
#include "res/Future2_rgba.h"
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
		FILE* f = fopen("../res/Future2.dds", "rb");
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


/*
//{
#define	GL_PROGRAM_BINARY_RETRIEVABLE_HINT             0x8257
#define	GL_PROGRAM_BINARY_LENGTH                       0x8741
#define	GL_NUM_PROGRAM_BINARY_FORMATS                  0x87FE
#define	GL_PROGRAM_BINARY_FORMATS                      0x87FF

typedef void (APIENTRYP PFNGLGETPROGRAMBINARYPROC)  ( GLuint program, GLsizei bufSize, GLsizei * length, GLenum *binaryFormat, GLvoid * binary );
typedef void (APIENTRYP PFNGLPROGRAMBINARYPROC)     ( GLuint program, GLenum binaryFormat, const GLvoid * binary, GLsizei length );
typedef void (APIENTRYP PFNGLPROGRAMPARAMETERIPROC) ( GLuint program, GLenum pname, GLint value );
PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
PFNGLPROGRAMBINARYPROC glProgramBinary;
PFNGLPROGRAMPARAMETERIPROC glProgramParameteri;* /

GLint   binaryLength;
GLint   numFormats;;
GLenum binaryFormat;
void* binary;
FILE* f;

/ *glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)glutGetProcAddress("glGetProgramBinary");
glProgramBinary = (PFNGLPROGRAMBINARYPROC)glutGetProcAddress("glProgramBinary");
glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)glutGetProcAddress("glProgramParameteri");
print("glGetProgramBinary: %p glProgramBinary: %p glProgramParameteri: %p\n",
	glGetProgramBinary, glProgramBinary, glProgramParameteri
);* /

glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS_OES,&numFormats);
print("numFormats: %d\n",numFormats);

glGetProgramiv ( point_gl20_prog, GL_PROGRAM_BINARY_LENGTH_OES, &binaryLength );
print("binaryLength: %d\n",binaryLength);
if(binaryLength){
	binary = malloc(binaryLength);

	glGetProgramBinaryOES ( point_gl20_prog, binaryLength, NULL, &binaryFormat, binary );

	f = fopen("shader.bin","wb");
	fwrite(binary,1,binaryLength,f);
	fclose(f);
	print("ok\n");

	free(binary);
//}*/

extern void test1();
extern void test2();
extern void test3();
extern void test4();
extern void test5();
extern void test6();
extern void test7();
extern void test_static();

void Display(void);
void Idle(void);

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
	test1();
#endif
#if !defined(WINAPI_FAMILY_SYSTEM)
	test2();
#endif
	test3();
	test4();
	test5();
	///test6();
	test7();
	//test_static();

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
#include "res/flare_rgba.h"
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare_rgba);
	}
#else
	{
		GLubyte* img_data;
		FILE* f = fopen("../res/flare.rgba", "rb");
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

int frameCount;
int currentTime, previousTime;
float lastTime;
int first_circle;
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

		if (first_circle/*>3*/) {
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
			//++first_circle;
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

void addmethod(void(*init)(), void(*update)(float), void(*draw)(), void(*deinit)(), char* name) {
	methods[num_methods].init = init;
	methods[num_methods].update = update;
	methods[num_methods].draw = draw;
	methods[num_methods].deinit = deinit;
	methods[num_methods].name = name;
	tt[num_methods].memsize[0] = 0;
	tt[num_methods].memsize[1] = 0;
	tt[num_methods].name = name;
	++num_methods;
}
