#include "pch.h"

static const int num_quads = 24;
static int updated = 0;
static int updated_count = 1;
static float* quads_verts; // x,y,u,v,c
static GLuint quads_prog;
static GLint a_pos;
static GLint a_col;
static GLuint quads_buffers[2];// 0 vtx, 1 indices

static float* centers;

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

static const char quads_pos_vert_src[] =
	"attribute vec4 pos;"
	"attribute vec4 col;"
	"uniform vec2 u_pos;"
	"varying vec2 v_uv;"
	"varying vec4 v_col;"
	"void main(){"
	"	gl_Position = vec4(pos.xy + u_pos, 0.0, 1.0);"
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

static char bin_name[] = "/sdcard/Android/data/com.bench/files/test6_shader.bin";
static char bin_name_pos[] = "/sdcard/Android/data/com.bench/files/test6_pos_shader.bin";

static void init_vbuffer() {
	float c[2] = {0.f, 0.f}; //center
	float l, r, t, b;
	int i, id = 0;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
	quads_prog = creatBinProg(bin_name, quads_vert_src, quads_frag_src);	
	a_pos = glGetAttribLocation(quads_prog, "pos");
	a_col = glGetAttribLocation(quads_prog, "col");

	glGenBuffers(2, quads_buffers);

	quads_verts = (float*)calloc(20 * num_quads, 4);
	for (i = 0; i < num_quads; ++i) {
		//c[0] = (i%4)*0.5f-0.75f;
		//c[1] = (i/4)*0.5f-0.75f;
		c[0] = (i % 8) * 0.25f - 0.875f;
		c[1] = (i / 8) * 0.25f - 0.875f;
		// pos
		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;

		quads_verts[id] = l;
		quads_verts[id + 1] = t;

		quads_verts[id + 5] = l;
		quads_verts[id + 6] = b;

		quads_verts[id + 10] = r;
		quads_verts[id + 11] = t;

		quads_verts[id + 15] = r;
		quads_verts[id + 16] = b;

		//uv
		quads_verts[id + 3] = 1;// l t

		quads_verts[id + 12] = 1;// r t
		quads_verts[id + 13] = 1;

		quads_verts[id + 17] = 1;// r b

		//color
		quads_verts[id + 4] = color.f;
		quads_verts[id + 9] = color.f;
		quads_verts[id + 14] = color.f;
		quads_verts[id + 19] = color.f;

		// next quad
		id += 20;
	}

	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 80 * num_quads, quads_verts, GL_STATIC_DRAW);
	updated = rand() % (num_quads - updated_count + 1);
}

static void init_vbuffer_pos() {
	float c[2] = {0.f, 0.f}; //center
	float l, r, t, b;
	int i, id = 0;
	union {
		unsigned char uc[4];
		float f;
	} color = { { 0xff, 0xff, 0xff, 0xff } };
	quads_prog = creatBinProg(bin_name_pos, quads_pos_vert_src, quads_frag_src);
	a_pos = glGetAttribLocation(quads_prog, "pos");
	a_col = glGetAttribLocation(quads_prog, "col");

	glGenBuffers(2, quads_buffers);

	centers = (float*)malloc(num_quads * 2 * 4);
	id = 0;
	for (i = 0; i < num_quads; ++i) {
		centers[id++] = (i % 8) * 0.25f - 0.875f;
		centers[id++] = (i / 8) * 0.25f - 0.875f;
	}

	id = 0;
	quads_verts = (float*)calloc(20, 4);

	// pos
	l = c[0] - 0.032f;
	r = c[0] + 0.032f;
	t = c[1] + 0.032f;
	b = c[1] - 0.032f;

	quads_verts[id] = l;
	quads_verts[id + 1] = t;

	quads_verts[id + 5] = l;
	quads_verts[id + 6] = b;

	quads_verts[id + 10] = r;
	quads_verts[id + 11] = t;

	quads_verts[id + 15] = r;
	quads_verts[id + 16] = b;

	//uv
	quads_verts[id + 3] = 1;// l t

	quads_verts[id + 12] = 1;// r t
	quads_verts[id + 13] = 1;

	quads_verts[id + 17] = 1;// r b

	//color
	quads_verts[id + 4] = color.f;
	quads_verts[id + 9] = color.f;
	quads_verts[id + 14] = color.f;
	quads_verts[id + 19] = color.f;


	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 80, quads_verts, GL_STATIC_DRAW);
	updated = rand() % (num_quads - updated_count + 1);
}

static void init1() {
	int i, id;
	unsigned short* ib;

	init_vbuffer();

	// 6 inds * sizeof(short)
	ib = (unsigned short*)malloc(num_quads * 12);
	id = 0;
	for (i = 0; i < num_quads; ++i) {
		int it = i * 4;
		ib[id++] = it;
		ib[id++] = it + 1;
		ib[id++] = it + 2;
		ib[id++] = it + 2;
		ib[id++] = it + 1;
		ib[id++] = it + 3;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_quads * 12, ib, GL_STATIC_DRAW);

	free(ib);
}

static void init1_pos() {
	int i, id;
	unsigned short* ib;

	init_vbuffer_pos();

	// 6 inds * sizeof(short)
	ib = (unsigned short*)malloc(12);
	id = 0;
	for (i = 0; i < 1; ++i) {
		int it = i * 4;
		ib[id++] = it;
		ib[id++] = it + 1;
		ib[id++] = it + 2;
		ib[id++] = it + 2;
		ib[id++] = it + 1;
		ib[id++] = it + 3;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12, ib, GL_STATIC_DRAW);

	free(ib);
}

static void init2() {
	int i, ic, ni = 4, index = 4;
	unsigned short* ib;

	init_vbuffer();

	ic = num_quads * 6 - 2;//count indices
	ib = (unsigned short*)malloc(ic * 2);
	ib[0] = 0; ib[1] = 1; ib[2] = 2; ib[3] = 3;
	for (i = 1; i < num_quads; ++i) {
		ib[ni++] = index - 1;
		ib[ni++] = index;
		ib[ni++] = index++;
		ib[ni++] = index++;
		ib[ni++] = index++;
		ib[ni++] = index++;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads_buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ic * 2, ib, GL_STATIC_DRAW);

	free(ib);
}

static void updete(float time) {
	int i;
	float c[2];//center
	float l, r, t, b;
	int id = 0;
	for (i = 0; i < updated_count; ++i) {
		c[0] = ((updated + i) % 8) * 0.25f - 0.875f;
		c[1] = ((updated + i) / 8) * 0.25f - 0.875f;
		c[0] = c[0] + sinf(time) * 0.25f;
		c[1] = c[1] + cosf(time) * 0.25f;
		//pos
		l = c[0] - 0.032f;
		r = c[0] + 0.032f;
		t = c[1] + 0.032f;
		b = c[1] - 0.032f;

		quads_verts[id] = l;
		quads_verts[id + 1] = t;

		quads_verts[id + 5] = l;
		quads_verts[id + 6] = b;

		quads_verts[id + 10] = r;
		quads_verts[id + 11] = t;

		quads_verts[id + 15] = r;
		quads_verts[id + 16] = b;
		id += 20;
	}
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferSubData(GL_ARRAY_BUFFER, updated * 80, updated_count * 80, quads_verts);
}

static void updete_pos(float time) {
	int i;
	float c[2];
	for (i = 0; i < updated_count; ++i) {
		c[0] = ((updated + i) % 8) * 0.25f - 0.875f;
		c[1] = ((updated + i) / 8) * 0.25f - 0.875f;
		centers[(updated + i) * 2] = c[0] + sinf(time) * 0.25f;
		centers[(updated + i) * 2 + 1] = c[1] + cosf(time) * 0.25f;
	}
}

static void draw1() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	//glBindBuffer(GL_ARRAY_BUFFER,quads_buffers[0]); // bind in update
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quads_ibufer);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(a_col);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

static void draw1_pos() {
	int i;
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quads_ibufer);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	for (i = 0; i < num_quads; ++i) {
		glUniform2fv(0, 1, centers + i * 2);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	}
	glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(a_col);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

static void draw2() {
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glUseProgram(quads_prog);
	//glBindBuffer(GL_ARRAY_BUFFER,quads_buffers[0]); // bind in update
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quads_ibufer);
	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_col);
	glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, 20, 0);
	glVertexAttribPointer(a_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, 20, (const void*)16);
	glDrawElements(GL_TRIANGLE_STRIP, num_quads * 6 - 2, GL_UNSIGNED_SHORT, 0);
	glDisableVertexAttribArray(a_pos);
	glDisableVertexAttribArray(a_col);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

static void deinit() {
	free(quads_verts);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}

static void deinit_pos() {
	free(centers);
	free(quads_verts);

	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}

void test6() {
	addmethod(init1, updete, draw1, deinit, "GL_TRIANGLES");
	addmethod(init2, updete, draw2, deinit, "GL_TRIANGLE_STRIP");
	addmethod(init1_pos, updete_pos, draw1_pos, deinit_pos, "GL_TRIANGLES uni_pos");
}
