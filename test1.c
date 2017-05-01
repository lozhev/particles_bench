#include "pch.h"
#if !defined(__ANDROID__) && !defined(WINAPI_FAMILY_SYSTEM)

static float* points;
static float* point_verts;

static void init() {
	point_verts = (float*)calloc(3 * NUM_PONTS, 4);
	points = make_points();
}

static void updete(float time) {
	int i;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		float frac = time + points[i + 2];
		float p = frac - (long)frac;
		color.uc[3] = (GLubyte)((1 - p) * 0xff);
		point_verts[i] = p * points[i + 2] / NUM_PONTS * points[i];
		point_verts[i + 1] = p * points[i + 2] / NUM_PONTS * points[i + 1];
		point_verts[i + 2] = color.f;
	}
}

static void draw() {
	int i;
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glBegin(GL_POINTS);
	for (i = 0; i < NUM_PONTS * 3; i += 3) {
		glColor4ubv((GLubyte*)(point_verts + i + 2));
		glVertex2f(point_verts[i], point_verts[i + 1]);
	}
	glEnd();
}

static void deinit() {
	free(point_verts);
	free(points);
}

void test1() {
	addmethod(init, updete, draw, deinit, "glBegin(GL_POINTS)/glEnd()");
}
#endif
