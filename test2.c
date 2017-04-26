#include "pch.h"
#ifndef WINAPI_FAMILY_SYSTEM

static float* points;
static float* point_verts;

// its work on opengles 1.0??
static GLuint prog;

static void init1() {
	point_verts = (float*)calloc(3 * NUM_PONTS, 4);
	points = make_points();
	glEnableClientState(GL_VERTEX_ARRAY);
}

static void init2() {
	const char vert_src[] =
		"void main(){"
		"	gl_FrontColor = gl_Color;"
		"	gl_Position = gl_Vertex;"
		"}";

	// gl_PointCoord #version 110 not work in my linux mesa
	const char frag_src[] =
		FRAG_VERSION
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*gl_Color;"
		"}";

	points = make_points();
	point_verts = (float*)calloc(3 * NUM_PONTS, 4);

	prog = creatProg(vert_src, frag_src);
	glEnableClientState(GL_VERTEX_ARRAY);
}

static void updete(float t) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = t + points[i + 2];
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		point_verts[i] = p * points[i + 2] / NUM_PONTS * points[i];
		point_verts[i + 1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		point_verts[i + 2] = color.f;
	}
}

static void draw1() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 12, point_verts);
	glColorPointer(4, GL_UNSIGNED_BYTE, 12, point_verts + 2);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

static void draw2() {
	glUseProgram(prog);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 12, point_verts);
	glColorPointer(4, GL_UNSIGNED_BYTE, 12, point_verts + 2);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

static void deinit1() {
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
}

static void deinit2() {
	free(point_verts);
	free(points);

	glDisableClientState(GL_COLOR_ARRAY);
	glDeleteProgram(prog);
}

void test2() {
	addmethod(init1, updete, draw1, deinit1, "glVertexPointer()");
	addmethod(init2, updete, draw2, deinit2, "glVertexPointer()/simple shader");
}

#endif