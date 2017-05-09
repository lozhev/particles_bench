#include "pch.h"

static GLuint prog;
static GLuint vbuffer;

static void init() {
	float* points;
	const char vert_src[] =
		"attribute vec3 pos;"
		"uniform float time;"
		"const float NUM_POINTS="STR(NUM_PONTS)".0;"
		"varying vec4 v_col;"
		"void main(){"
		"	float p = fract(time + pos.z);"
		"	vec2 ps = p*pos.z/NUM_POINTS * pos.xy;"
		"	gl_Position = vec4(ps, 0.0, 1.0);"
		"	v_col = vec4(1.0, 1.0, 1.0, 1.0-p);"
#ifdef WINAPI_FAMILY_SYSTEM
		"	gl_PointSize = 19.0;"
#endif
		"}";

	const char frag_src[] =
		FRAG_VERSION
		PRECISION_FLOAT
		"uniform sampler2D u_tex;"
		"varying vec4 v_col;"
		"void main(){"
		"	gl_FragColor = texture2D(u_tex, gl_PointCoord)*v_col;"
		"}";

	prog = creatProg(vert_src, frag_src);

	//
	points = make_points();

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, 12 * NUM_PONTS, points, GL_STATIC_DRAW);

	free(points);
}

static void updete(float time) {
	glUseProgram(prog);
	glUniform1f(0, time);
	//glVertexAttrib1f(1, time);

	//glUniform1f(0, time);
	//glUniform1fvARB(0, 1, &time);
	//glUniform1fARB(0, time);
	// mot work in win10 with Intel(R) HD Graphics Ironlake-M - Build 8.15.10.2900 :(
}

static void draw() {
	//glUseProgram(prog);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_POINTS, 0, NUM_PONTS);
}

static void deinit() {
	glDeleteProgram(prog);
	glDeleteBuffers(1, &vbuffer);
}

void test3() {
	addmethod(init, updete, draw, deinit, "VBO compute in shader");
}
