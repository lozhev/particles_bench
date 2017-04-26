#include "pch.h"

static float* points;
static float* quads_verts;
static GLuint quads_prog;
// x,y,u,v,c

static void init() {
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

	points = make_points();

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

static void updete(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
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

static void draw() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 20, quads_verts);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, quads_verts + 4);
	glDrawArrays(GL_TRIANGLES, 0, NUM_PONTS * 6);
	glDisableVertexAttribArray(1);
}

static void deinit() {
	free(quads_verts);
	free(points);

	glDeleteProgram(quads_prog);
}

void test4() {
	addmethod(init, updete, draw, deinit, "Quad from 2 triangle");
}