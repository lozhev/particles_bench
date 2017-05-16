#include "pch.h"

static float* points;
static GLuint quads_prog;
static GLuint quads_buffers[2];// 0 vtx, 1 indices


static const char quads_vert_src[] =
	"attribute vec4 pos;"
	//"uniform vec3 u_pos;"// x,y,a
	"attribute vec3 u_pos;"// x,y,a
	"varying vec2 v_uv;"
	"varying float v_a;"
	"void main(){"
	"	gl_Position = vec4(pos.xy + u_pos.xy, 0.0, 1.0);"
	"	v_uv = pos.zw;"
	"	v_a = u_pos.z;"
	"}";

static const char quads_frag_src[] =
	PRECISION_FLOAT
	"uniform sampler2D u_tex;"
	"varying vec2 v_uv;"
	"varying float v_a;"
	"void main(){"
	"	gl_FragColor = texture2D(u_tex, v_uv);"
	"	gl_FragColor.a *= v_a;"
	"}";
#ifdef BIN_SHADER
static char bin_name[] =
	SHADER_FOLDER
	"test7.bin";
#endif

static void init_buffers() {
	float quads_verts[16]; // x,y,u,v
	unsigned short ib[4] = {0, 1, 2, 3};
	float l, r, t, b;

	points = make_points();

	// pos
	l = - 0.032f;
	r = + 0.032f;
	t = + 0.032f;
	b = - 0.032f;

	quads_verts[0] = l;
	quads_verts[1] = t;

	quads_verts[4] = l;
	quads_verts[5] = b;

	quads_verts[8] = r;
	quads_verts[9] = t;

	quads_verts[12] = r;
	quads_verts[13] = b;

	//uv
	quads_verts[2] = 0;// l t
	quads_verts[3] = 1;

	quads_verts[6] = 0;// l b
	quads_verts[7] = 0;

	quads_verts[10] = 1;// r t
	quads_verts[11] = 1;

	quads_verts[14] = 1;// r b
	quads_verts[15] = 0;

	glGenBuffers(2, quads_buffers);

	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 64, quads_verts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 8, ib, GL_STATIC_DRAW);
}

static void init() {
	init_buffers();
	quads_prog = creatBinProg(bin_name, quads_vert_src, quads_frag_src);
}

static void updete(float time) {
}

static void draw() {
	int i;
	float uni[3];
	float time = (float)seTime() / 1000.f;

	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	for(i = 0; i < NUM_PONTS; ++i) {
		int pi = i * 3;
		float frac = time + points[pi + 2];
		float p = frac - (long)frac;
		uni[0] = p * points[pi + 2] / NUM_PONTS * points[pi];
		uni[1] = p * points[pi + 2] / NUM_PONTS * points[pi + 1];
		uni[2] = (1 - p);
									//GeForce GT 240
		//glUniform3fv(0, 1, uni);  //fps 771  cpu 1.20655
		glVertexAttrib3fv(1, uni);  //fps 1513 cpu 0.57302
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
	}
}

static void deinit() {
	free(points);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}
//#define USE_INST
#ifdef USE_INST
// DrawElementsInstanced
#define SHADER_1
#ifdef WINAPI_FAMILY_SYSTEM
#define glDrawElementsInstanced(m, c, t, i, p) glDrawElementsInstancedANGLE(m,c,t,i,p)
#define glVertexAttribDivisor(i, d) glVertexAttribDivisorANGLE(i,d)
#undef SHADER_1
#endif
#define INST_COUNT 1024
static GLint pos;
#ifdef SHADER_1
static GLint u_pos;
#else
static GLint a_pos;
#endif
static const char quads_inst_vert_src[] =
	//"#version 300 es\n"
	//"#version 140\n"// need for more uniforms, and in/out too need
	// not run in mesa. info:0(1) : warning C7532: global variable gl_InstanceID requires "#version 140" or later
	"attribute vec4 pos;"
	//"in vec4 pos;"
#ifdef SHADER_1
	"uniform vec3 u_pos["STR(INST_COUNT)"];"// x,y,a 1024?? max
#else
	"attribute vec3 a_pos;"
#endif
	"varying vec2 v_uv;"
	"varying float v_a;"
	"void main(){"
#ifdef SHADER_1
	"	gl_Position = vec4(pos.xy + u_pos[gl_InstanceID].xy, 0.0, 1.0);"
	"	v_a = u_pos[gl_InstanceID].z;"
#else
	"	gl_Position = vec4(pos.xy + a_pos.xy, 0.0, 1.0);"
	"	v_a = a_pos.z;"
#endif
	"	v_uv = pos.zw;"
	"}";

#ifdef BIN_SHADER
static char bin_name_inst[] =
	SHADER_FOLDER
	"test7_inst.bin";
#endif

static float* point_uniforms;

static void init_inst() {
	init_buffers();
	point_uniforms = (float*)calloc(3 * NUM_PONTS, 4);
	quads_prog = creatBinProg(bin_name_inst, quads_inst_vert_src, quads_frag_src);
#ifdef SHADER_1
	u_pos = glGetUniformLocation(quads_prog, "u_pos");
#else
	a_pos = glGetAttribLocation(quads_prog, "a_pos");
#endif
	pos = glGetAttribLocation(quads_prog, "pos");// always 0 if SHADER_1
}

static void updete_inst(float time) {
	int i;
	for(i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		point_uniforms[i] = p * points[i + 2] / NUM_PONTS * points[i];
		point_uniforms[i + 1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		point_uniforms[i + 2] = (1 - p);
	}
}

static void draw_inst() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 4, GL_FLOAT, GL_FALSE, 0, 0);
#ifdef SHADER_1
	{int i, n = NUM_PONTS;
		for(i = 0; n / INST_COUNT; ++i) {
			glUniform3fv(u_pos, INST_COUNT, point_uniforms + i * 3 * INST_COUNT);
			glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, INST_COUNT);
			n -= INST_COUNT;
		}
		glUniform3fv(u_pos, n, point_uniforms + i * 3 * INST_COUNT);
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, n); }
#else
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(a_pos);
	glVertexAttribPointer(a_pos, 3, GL_FLOAT, GL_FALSE, 0, point_uniforms);
	glVertexAttribDivisor(a_pos, 1);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, NUM_PONTS);
	glDisableVertexAttribArray(pos);
	glDisableVertexAttribArray(a_pos);
#endif
}

static void deinit_inst() {
	free(points);
	free(point_uniforms);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
#ifndef SHADER_1
	glVertexAttribDivisor(a_pos, 0);// move to draw??
#endif
}
#endif

//#define USE_MULTI
#ifdef USE_MULTI
#if __ANDROID__ || WINAPI_FAMILY_SYSTEM
//Exclusive to PowerVR..
#define glMultiDrawElements(m, c, t, i, pc) glMultiDrawElementsEXT(m, c, t, i, pc)
#endif
// glMultiDrawElements
static const char quads_multi_vert_src[] =
	"attribute vec4 pos;"
	"attribute float a;"
	"varying vec2 v_uv;"
	"varying float v_a;"
	"void main(){"
	"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
	"	v_a = a;"
	"	v_uv = pos.zw;"
	"}";
#ifdef BIN_SHADER
static char bin_name_multi[] =
	SHADER_FOLDER
	"test7_multi.bin";
#endif

static float* verts; // x,y,u,v,a
static void** indices;
static GLsizei* i_count;

static void init_multi(){
	int i, id=2;
	GLushort* ib;
	//float l, r, t, b;

	points = make_points();
	
	verts = (float*)calloc(20 * NUM_PONTS, 4);
	for(i = 0; i < NUM_PONTS; ++i) {
		verts[id + 1] = 1;

		verts[id + 10] = 1;
		verts[id + 11] = 1;

		verts[id + 15] = 1;
		id += 20;
	}

	ib = (GLushort*)malloc(NUM_PONTS * 8); //4*2;

	glGenBuffers(2, quads_buffers);
	for(i = 0; i < NUM_PONTS * 4; ++i) {
		ib[i] = i;
	}
	//ib[i] = 0;

	indices = malloc(NUM_PONTS*sizeof(void*));
	i_count = malloc(NUM_PONTS * 4);
	for(i = 0; i < NUM_PONTS; ++i) {
		indices[i] = (char*)(0) + (i * 8);
		i_count[i] = 4;
	}
	
	glGenBuffers(2, quads_buffers);

	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 80 * NUM_PONTS, verts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_PONTS * 8, ib, GL_STATIC_DRAW);
	free(ib);

	quads_prog = creatBinProg(bin_name_multi, quads_multi_vert_src, quads_frag_src);
}

static void update_multi(float time){
	float c[2], color;
	float l, r, t, b;
	int i, index = 0;
	for(i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		color = 1 - p;
		c[0] = p * points[i + 2] / NUM_PONTS * points[i];
		c[1] = p * points[i + 2] / NUM_PONTS * points[i + 1];

		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;

		verts[index] = l;
		verts[index + 1] = t;
		verts[index + 4] = color;

		verts[index + 5] = l;
		verts[index + 6] = b;
		verts[index + 9] = color;

		verts[index + 10] = r;
		verts[index + 11] = t;
		verts[index + 14] = color;

		verts[index + 15] = r;
		verts[index + 16] = b;
		verts[index + 19] = color;
		index += 20;
	}
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 80 * NUM_PONTS, verts);
}

static void draw_multi(){
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 20, (const void*)16);
	glMultiDrawElements(GL_TRIANGLE_STRIP, i_count, GL_UNSIGNED_SHORT, (const void**)indices, NUM_PONTS);
	//glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(1);
}

static void deinit_multi(){
	free(points);
	free(verts);
	free(indices);
	free(i_count);

	glDeleteBuffers(2,quads_buffers);

	glDeleteProgram(quads_prog);
}
#endif

void test7() {
	addmethod(init, updete, draw, deinit, "per quad");
#ifdef USE_MULTI
	addmethod(init_multi, update_multi, draw_multi, deinit_multi, "glMultiDrawElements");
#endif
#ifdef USE_INST
	addmethod(init_inst, updete_inst, draw_inst, deinit_inst, "inst");
#endif
}
