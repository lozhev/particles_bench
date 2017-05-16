#include "pch.h"

static float* points;
static float* quads_verts; // x,y,u,v,c
static GLuint quads_prog;
static GLint a_pos;
static GLint a_col;
static GLuint quads_buffers[2];// 0 vtx, 1 indices

static const char quads_vert_src[] =
	"attribute vec4 pos;"
	"attribute vec4 col;"
	"varying vec2 v_uv;"
	"varying vec4 v_col;"
	"void main(){"
	"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
	"	v_uv = pos.zw;"
	"	v_col = col;"
	"}";

static const char quads_frag_src[] =
	PRECISION_FLOAT
	"uniform sampler2D u_tex;"
	"varying vec2 v_uv;"
	"varying vec4 v_col;"
	"void main(){"
	"	gl_FragColor = texture2D(u_tex, v_uv) * v_col;"
	"}";
#ifdef BIN_SHADER
static char bin_name[] =
	SHADER_FOLDER
	"test5.bin";
#endif
	
void init_vbuffer() {
	int i, id = 2;
	points = make_points();
	quads_prog = creatBinProg(bin_name, quads_vert_src, quads_frag_src);
	a_pos = glGetAttribLocation(quads_prog, "pos");
	a_col = glGetAttribLocation(quads_prog, "col");

	glGenBuffers(2, quads_buffers);

	quads_verts = (float*)calloc(20 * NUM_PONTS, 4);
	for(i = 0; i < NUM_PONTS; ++i) {
		quads_verts[id + 1] = 1;

		quads_verts[id + 10] = 1;
		quads_verts[id + 11] = 1;

		quads_verts[id + 15] = 1;
		id += 20;
	}

	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 80 * NUM_PONTS, quads_verts, GL_STATIC_DRAW);
}

static void init1() {
	int i, id;
	unsigned short* ib;

	init_vbuffer();

	// 6 inds * sizeof(short)
	ib = (unsigned short*)malloc(NUM_PONTS * 12);
	for(i = 0; i < NUM_PONTS; ++i) {
		int it = i * 4;
		id = i * 6;
		ib[id] = it;
		ib[id + 1] = it + 1;
		ib[id + 2] = it + 2;
		ib[id + 3] = it + 2;
		ib[id + 4] = it + 1;
		ib[id + 5] = it + 3;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_PONTS * 12, ib, GL_STATIC_DRAW);

	free(ib);
}

static void init2() {
	int i, id, nvtx;
	unsigned short* ib, *iptr;

	init_vbuffer();

	// (NUM_PONTS * 12 + sizeof(short)) - 4;
	nvtx = NUM_PONTS * 4;// num vertex
	id = (NUM_PONTS - 1) * 6 + 4;// count indices
	ib = (unsigned short*)malloc(id * 2);
	iptr = ib;
	for(i = 0; i < nvtx; ++i) {
		*iptr = i;
		++iptr;
		if(i < (nvtx - 1) && (i % 4) - 3 == 0) {
			*iptr = i;
			++iptr;
			*iptr = i + 1;
			++iptr;
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, id * 2, ib, GL_STATIC_DRAW);

	free(ib);
}
#ifdef GL_PRIMITIVE_RESTART
static GLuint vao;
#ifdef __ANDROID__
#define glGenVertexArrays(n, a) glGenVertexArraysOES(n ,a)
#define glBindVertexArray(a) glBindVertexArrayOES(a)
#define glDeleteVertexArrays(n, a) glDeleteVertexArraysOES(n, a)
#endif
static void init3() {
	int i, ni = 0, nvtx;
	unsigned short* ib;

	init_vbuffer();

	nvtx = NUM_PONTS * 5;// count indices
	ib = (unsigned short*)malloc(nvtx * 2);
	for(i = 0; i < NUM_PONTS*4; i+=4) {
		ib[ni++]=i;
		ib[ni++]=i+1;
		ib[ni++]=i+2;
		ib[ni++]=i+3;
		ib[ni++]=0xffff;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nvtx * 2, ib, GL_STATIC_DRAW);

	free(ib);

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xffff);

	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
}
#endif
static void updete(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
	float c[2];//center
	float l, r, t, b;
	int index = 0;
	//0.18132 -> 0.16081 ??
	for(i = 0; i < NUM_PONTS * 3; i += 3) {
		float p2 = points[i + 2];
		float frac = time + p2;
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		p *= p2;
		c[0] = p / NUM_PONTS * points[i];
		c[1] = p / NUM_PONTS * points[i + 1];
		//point_verts[i+2] = color.f;
		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;

		/*quads_verts[index] = l;
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
		quads_verts[index + 1] = b;
		quads_verts[index + 4] = color.f;
		index += 5;*/
		quads_verts[index] = l;
		quads_verts[index + 1] = t;
		quads_verts[index + 4] = color.f;
		//index += 5;
		quads_verts[index + 5] = l;
		quads_verts[index + 6] = b;
		quads_verts[index + 9] = color.f;
		//index += 5;
		quads_verts[index + 10] = r;
		quads_verts[index + 11] = t;
		quads_verts[index + 14] = color.f;
		//index += 5;
		quads_verts[index + 15] = r;
		quads_verts[index + 16] = b;
		quads_verts[index + 19] = color.f;
		index += 20;
	}
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 80 * NUM_PONTS, quads_verts);
}

static void draw1() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLES, NUM_PONTS * 6, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(a_col);
}

static void draw2() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLE_STRIP, NUM_PONTS * 6 - 2, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(a_col);
}
#ifdef GL_PRIMITIVE_RESTART
static void draw3() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLE_STRIP, NUM_PONTS * 5, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
#endif
static void deinit() {
	free(points);
	free(quads_verts);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}
#ifdef GL_PRIMITIVE_RESTART
static void deinit3() {
	free(points);
	free(quads_verts);

	glDeleteVertexArrays(1,&vao);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}
#endif

void test5() {
	addmethod(init1, updete, draw1, deinit, "Quad from 2 triangle glbuffers");
	addmethod(init2, updete, draw2, deinit, "Quad from 2 triangle glbuffers tristrip");
#ifdef GL_PRIMITIVE_RESTART
	addmethod(init3, updete, draw3, deinit3, "Quad from 2 triangle glbuffers tristrip restart vao");
#endif
}
