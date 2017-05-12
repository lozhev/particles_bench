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
#include <GLES2/gl2ext.h> //GL_OES_get_program_binary
#elif __ANDROID__
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else// win32 __linux
#include "glad/glad.h"
#define USE_GLUT
#endif

#ifdef USE_GLUT
#include <GL/freeglut.h>
#endif

// shaders
#ifdef __ANDROID__
#define PRECISION_FLOAT "precision lowp float;"
#define FRAG_VERSION
#define EMBEDDED_DATA
#elif _WIN32
#ifdef WINAPI_FAMILY_SYSTEM
#define PRECISION_FLOAT "precision lowp float;"
#define EMBEDDED_DATA
#else
#define PRECISION_FLOAT
#endif
#define FRAG_VERSION
#elif __linux
#define PRECISION_FLOAT
#define FRAG_VERSION "#version 120\n"
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// util
void print(const char* format, ...);
double seTime();
GLuint creatProg(const char* vert_src, const char* frag_src);
#ifndef __ANDROID__
#define creatBinProg(file_name, vert_src, frag_src) creatProg(vert_src, frag_src);
#else
GLuint creatBinProg(const char* file_name, const char* vert_src, const char* frag_src);
#endif
GLuint GetAttribs(GLuint program);
GLuint GetUniforms(GLuint program);
void addmethod(void(*)(), void(*)(float), void(*)(), void(*)(), char*);

float* make_points();

extern GLuint sprite_tex;
#endif
