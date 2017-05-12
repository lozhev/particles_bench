#include "pch.h"

static float* points;
static float* point_uniforms;
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

static char bin_name[] = "/sdcard/Android/data/com.bench/files/test7_shader.bin";

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

	for (i = 0; i < NUM_PONTS; ++i) {
		int pi = i * 3;
		float frac = time + points[pi + 2];
		float p = frac - (long)frac;
		uni[0] = p * points[pi + 2] / NUM_PONTS * points[pi];
		uni[1] = p * points[pi + 2] / NUM_PONTS * points[pi + 1];
		uni[2] = (1 - p);
		//glUniform3fv(0, 1, uni);
		glVertexAttrib3fv(1, uni);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
	}
}

static void deinit() {
	free(points);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}
#define USE_INST
#ifdef USE_INST
// DrawElementsInstanced
#define INST_COUNT 1024
static GLint u_pos;
static const char quads_inst_vert_src[] =
	//"#version 140\n"// need for more uniforms, and in/out too need
	//"sdf"//info:0(1) : warning C7532: global variable gl_InstanceID requires "#version 140" or later
	"attribute vec4 pos;"
	"uniform vec3 u_pos["STR(INST_COUNT)"];"// x,y,a 1024 for gt 210 max
	"varying vec2 v_uv;"
	"varying float v_a;"
	"void main(){"
	"	gl_Position = vec4(pos.xy + u_pos[gl_InstanceID].xy, 0.0, 1.0);"
	"	v_uv = pos.zw;"
	"	v_a = u_pos[gl_InstanceID].z;"
	"}";

static void init_inst() {
	init_buffers();
	point_uniforms = (float*)calloc(3 * NUM_PONTS, 4);
	quads_prog = creatProg(quads_inst_vert_src, quads_frag_src);
	u_pos = glGetUniformLocation(quads_prog, "u_pos");
}

static void updete_inst(float time) {
	int i;
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		point_uniforms[i] = p * points[i + 2] / NUM_PONTS * points[i];
		point_uniforms[i + 1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		point_uniforms[i + 2] = (1 - p);
	}
}

static void draw_inst() {
	int i, n;
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	n = NUM_PONTS;
	for (i = 0; n / INST_COUNT; ++i) {
		glUniform3fv(u_pos, INST_COUNT, point_uniforms + i * 3 * INST_COUNT);
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, INST_COUNT);
		n -= INST_COUNT;
	}
	glUniform3fv(u_pos, n, point_uniforms + i * 3 * INST_COUNT);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, n);
}

static void deinit_inst() {
	free(points);
	free(point_uniforms);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}
#endif

void test7() {
	addmethod(init, updete, draw, deinit, "per quad");
#ifdef USE_INST
	addmethod(init_inst, updete_inst, draw_inst, deinit_inst, "inst");
#endif
}
