#ifndef PCH_H
#define PCH_H
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <stdlib.h>

// cfg
#define NUM_PONTS 1600
#define SECONDS_PER_METHOD 10
#define CURRENT_METHOD 0
#define WRITE_RESULT 1
#define CYCLE_METODS 1
#define OUTPUT_FPS 0

// include

#if WINAPI_FAMILY_SYSTEM
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h> //GL_OES_get_program_binary instanced
#elif __ANDROID__
#include <EGL/egl.h>
//#define USE_GLES3
#ifdef USE_GLES3
#include <GLES3/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#endif
#else// win32 __linux
#include "glad/glad.h"
#define USE_GLUT
#endif

#ifdef USE_GLUT
#include <GL/freeglut.h>
#undef WINAPI_FAMILY_SYSTEM
#endif

// shaders
#ifdef __ANDROID__
#define PRECISION_FLOAT "precision lowp float;"
#define FRAG_VERSION
#define EMBEDDED_DATA
// make it if not exist!!
//#define SHADER_FOLDER "/sdcard/Android/data/com.bench/files/"
#define SHADER_FOLDER "/mnt/sdcard/Android/data/com.bench/files/"
#elif _WIN32
#ifdef WINAPI_FAMILY_SYSTEM
#define PRECISION_FLOAT "precision lowp float;"
#define EMBEDDED_DATA
//#define SHADER_FOLDER "c:/Users/<user>/AppData/Local/Packages/<packeg>/LocalState/"
#define SHADER_FOLDER
#else
#define PRECISION_FLOAT
#define SHADER_FOLDER
#endif
#define FRAG_VERSION
#elif __linux
#define PRECISION_FLOAT
#define FRAG_VERSION "#version 120\n"
#define SHADER_FOLDER
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// util
void print(const char* format, ...);
double seTime();

#if __ANDROID__ || WINAPI_FAMILY_SYSTEM
#define BIN_SHADER
#ifndef USE_GLES3
#define GL_PROGRAM_BINARY_LENGTH GL_PROGRAM_BINARY_LENGTH_OES
#define glGetProgramBinary(p, bs, l, f, b) glGetProgramBinaryOES(p, bs, l, f, b)
#define glProgramBinary(p, f, b, l) glProgramBinaryOES(p, f, b, l)
#endif
#endif

//if error like this: 
// E/emuglGLESv2_enc(1156): Function is unsupported
// or W/PGA(4729): ::::::: glGetProgramBinaryOES: Not Implmented
// or E/libEGL(1130): called unimplemented OpenGL ES API
//#undef BIN_SHADER

GLuint creatProg(const char* vert_src, const char* frag_src);
GLuint GetAttribs(GLuint program);
GLuint GetUniforms(GLuint program);

#ifdef BIN_SHADER
GLuint creatBinProg(const char* file_name, const char* vert_src, const char* frag_src);
#else
#define creatBinProg(file_name, vert_src, frag_src) creatProg(vert_src, frag_src);
#endif

void addmethod(void(*)(), void(*)(float), void(*)(), void(*)(), char*);
float* make_points();
extern GLuint sprite_tex;
#endif
