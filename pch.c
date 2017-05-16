#include "pch.h"

#ifdef __ANDROID__
#include <android/log.h>
void print(const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	__android_log_vprint(ANDROID_LOG_WARN, "native-activity", format, argptr);
	va_end(argptr);
}
#elif _WIN32
void print(const char* format, ...) {
	va_list argptr;
	char buf[256];
	va_start(argptr, format);
	vsprintf(buf, format, argptr);
	OutputDebugStringA(buf);
	va_end(argptr);
}
#elif __linux
void print(const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);
}
#endif

float* make_points() {
	int i;
	float* points = (float*)malloc(12 * NUM_PONTS);
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float a = 2.f * (float)M_PI * rand() / (float)RAND_MAX;;
		points[i] = sinf(a);
		points[i + 1] = cosf(a);
		points[i + 2] = NUM_PONTS * (float)rand() / (float)RAND_MAX;
	}
	return points;
}

#if 1
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
		print("%d: %s ", loc, name);
		if (type == GL_FLOAT) {
			print("float\n");
		} else if (type == GL_SAMPLER_2D) {
			print("SAMPLER_2D\n");
		}
	}

	//free(name);

	return n;
}

GLuint GetAttribs(GLuint program) {
	GLint i, n;
	//GLint max;
	//char* name;
	char name[16];

	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
	//glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);
	//name = (char*)malloc(max);

	for (i = 0; i < n; ++i) {
		GLint size, len, loc;
		GLenum type;

		glGetActiveAttrib(program, i, 16, &len, &size, &type, name);

		loc = glGetAttribLocation(program, name);
		print("%d: %s ", loc, name);
		if (type == GL_FLOAT) {
			print("float\n");
		} else if (type == GL_SAMPLER_2D) {
			print("SAMPLER_2D\n");
		}
	}

	//free(name);

	return n;
}

#ifdef BIN_SHADER
GLuint creatBinProg(const char* file_name, const char* vert_src, const char* frag_src){
	GLuint prog;
	GLint  binaryLength;
	GLenum binaryFormat;
	void* binary;
	FILE* f;
	f = fopen(file_name,"rb");
	if (f==0){
		prog = creatProg(vert_src, frag_src);
		glGetProgramiv(prog, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
		print("binaryLength: %d\n",binaryLength);
		if(binaryLength <= 0) return prog;
		binary = malloc(binaryLength);

		glGetProgramBinary(prog, binaryLength, NULL, &binaryFormat, binary);

		f = fopen(file_name,"wb");
		fwrite(&binaryFormat,1,4,f);
		fwrite(&binaryLength,1,4,f);
		fwrite(binary,1,binaryLength,f);
		fclose(f);
		print("ok\n");

		free(binary);
	} else {
		fread(&binaryFormat,4,1,f);
		fread(&binaryLength,4,1,f);
		binary = malloc(binaryLength);
		fread(binary,1,binaryLength,f);
		fclose(f);
		prog = glCreateProgram();
		glProgramBinary(prog, binaryFormat, binary, binaryLength); 
	}
	return prog;
}
#endif
