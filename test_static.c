#include "pch.h"

static const int num_quads = 64;
//static float* quads_verts; // x,y,u,v,c
static GLuint quads_prog;
static GLuint quads_buffers[2];// 0 vtx, 1 indices

///
typedef struct {
	float x0, y0;
	float x1, y1;
	float ratio;
} Character;
typedef struct {
	float pos[2];
	float tex[2];
} TexVertex;

extern Character* fnt_chars;
extern GLubyte* fnt_table;
extern GLuint fnt_tex;

static TexVertex* fnt_vtx;

#define SETVEC2(v,x,y)\
	v[0]=x;\
	v[1]=y

static void fillTextBuffer_tri(TexVertex* dest, const char* str, float x, float y, const float charWidth, const float charHeight) {
	float startx = x;

	while (*str) {
		if (*str == '\n') {
			y += charHeight;
			x = startx;
		} else {
			GLubyte ch_id = fnt_table[*(unsigned char*)str];
			Character* chr = &fnt_chars[ch_id];
			float cw = charWidth * chr->ratio;

			SETVEC2(dest[0].pos, x, y);
			SETVEC2(dest[0].tex, chr->x0, chr->y1);
			SETVEC2(dest[1].pos, x + cw, y);
			SETVEC2(dest[1].tex, chr->x1, chr->y1);
			SETVEC2(dest[2].pos, x, y + charHeight);
			SETVEC2(dest[2].tex, chr->x0, chr->y0);

			SETVEC2(dest[3].pos, x, y + charHeight);
			SETVEC2(dest[3].tex, chr->x0, chr->y0);
			SETVEC2(dest[4].pos, x + cw, y);
			SETVEC2(dest[4].tex, chr->x1, chr->y1);
			SETVEC2(dest[5].pos, x + cw, y + charHeight);
			SETVEC2(dest[5].tex, chr->x1, chr->y0);

			dest += 6;
			x += cw;
		}
		str++;
	}
}

static void fillTextBuffer_strip(TexVertex* dest, const char* str, float x, float y, const float charWidth, const float charHeight) {
	float startx = x;

	while (*str) {
		if (*str == '\n') {
			y += charHeight;
			x = startx;
		} else {
			GLubyte ch_id = fnt_table[*(unsigned char*)str];
			Character* chr = &fnt_chars[ch_id];
			float cw = charWidth * chr->ratio;

			SETVEC2(dest[0].pos, x, y);
			SETVEC2(dest[0].tex, chr->x0, chr->y1);
			SETVEC2(dest[1].pos, x + cw, y);
			SETVEC2(dest[1].tex, chr->x1, chr->y1);
			SETVEC2(dest[2].pos, x, y + charHeight);
			SETVEC2(dest[2].tex, chr->x0, chr->y0);
			SETVEC2(dest[3].pos, x + cw, y + charHeight);
			SETVEC2(dest[3].tex, chr->x1, chr->y0);

			dest += 4;
			x += cw;
		}
		str++;
	}
}

static const char quads_vert_src[] =
	"attribute vec4 pos;"
	"varying vec2 v_uv;"
	"void main(){"
	"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
	"	v_uv = pos.zw;"
	"}";

static const char quads_frag_src[] =
	PRECISION_FLOAT
	"uniform sampler2D u_tex;"
	"varying vec2 v_uv;"
	"void main(){"
	"	gl_FragColor = texture2D(u_tex, v_uv);"
	"}";


/// glVertexAttribPointer
static void init1() {
	int i;
	float c[2];
	// 4(num floats) * 6(vtx) * sizeof(float)
	fnt_vtx = (TexVertex*)malloc(96 * num_quads);
	for (i = 0; i < num_quads; ++i) {
		c[0] = (i % 8) * 0.25f - 0.875f;
		c[1] = (i / 8) * 0.25f - 0.875f;

		fillTextBuffer_tri(fnt_vtx + i * 6, "*", c[0] - 0.025f, c[1] - 0.04f, 0.08f, 0.1f);
	}

	quads_prog = creatProg(quads_vert_src, quads_frag_src);
}

static void draw1() {
	glBindTexture(GL_TEXTURE_2D, fnt_tex);
	glUseProgram(quads_prog);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, fnt_vtx);
	glDrawArrays(GL_TRIANGLES, 0, num_quads * 6);
}

static void deinit1() {
	free(fnt_vtx);

	glDeleteProgram(quads_prog);
}

/// glBufferData
static void init3() {
	int i;
	float c[2];
	// 4(num floats) * 6(vtx) * sizeof(float)
	fnt_vtx = (TexVertex*)malloc(96 * num_quads);
	for (i = 0; i < num_quads; ++i) {
		c[0] = (i % 8) * 0.25f - 0.875f;
		c[1] = (i / 8) * 0.25f - 0.875f;

		fillTextBuffer_tri(fnt_vtx + i * 6, "*", c[0] - 0.025f, c[1] - 0.04f, 0.08f, 0.1f);
	}

	glGenBuffers(1, quads_buffers);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 96 * num_quads, fnt_vtx, GL_STATIC_DRAW);
	free(fnt_vtx);

	quads_prog = creatProg(quads_vert_src, quads_frag_src);
}

static void draw3() {
	glBindTexture(GL_TEXTURE_2D, fnt_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, num_quads * 6);
}

static void deinit3() {
	glDeleteBuffers(1, quads_buffers);

	glDeleteProgram(quads_prog);
}

/// glBufferData index tristrip
static void init2() {
	int i, ic, ni = 4, index = 4;
	unsigned short* ib;
	float c[2]; //center

	quads_prog = creatProg(quads_vert_src, quads_frag_src);

	glGenBuffers(2, quads_buffers);

	// 4(num floats) * 4(vtx) * sizeof(float)
	fnt_vtx = (TexVertex*)malloc(64 * num_quads);
	for (i = 0; i < num_quads; ++i) {
		c[0] = (i % 8) * 0.25f - 0.875f;
		c[1] = (i / 8) * 0.25f - 0.875f;

		fillTextBuffer_strip(fnt_vtx + i * 4, "*", c[0] - 0.025f, c[1] - 0.04f, 0.08f, 0.1f);

	}
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 64 * num_quads, fnt_vtx, GL_STATIC_DRAW);
	free(fnt_vtx);

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

///// glBufferSubData index tristrip
static void init2_sub() {
	int i, ic, ni = 4, index = 4;
	unsigned short* ib;
	float c[2]; //center

	quads_prog = creatProg(quads_vert_src, quads_frag_src);

	glGenBuffers(2, quads_buffers);

	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 64 * num_quads, 0, GL_STATIC_DRAW);
	fnt_vtx = (TexVertex*)malloc(64);
	for (i = 0; i < num_quads; ++i) {
		c[0] = (i % 8) * 0.25f - 0.875f;
		c[1] = (i / 8) * 0.25f - 0.875f;

		fillTextBuffer_strip(fnt_vtx, "*", c[0] - 0.025f, c[1] - 0.04f, 0.08f, 0.1f);
		glBufferSubData(GL_ARRAY_BUFFER, i * 64, 64, fnt_vtx);
	}
	free(fnt_vtx);

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

static void draw2() {
	glBindTexture(GL_TEXTURE_2D, fnt_tex);
	glUseProgram(quads_prog);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffers[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawElements(GL_TRIANGLE_STRIP, num_quads * 6 - 2, GL_UNSIGNED_SHORT, 0);
}

static void deinit2() {
	glDeleteBuffers(2, quads_buffers);

	glDeleteProgram(quads_prog);
}

static void updete(float time) {
	//
}

void test_static() {
	addmethod(init3, updete, draw3, deinit3, "glBufferData tri");
	addmethod(init1, updete, draw1, deinit1, "glVertexAttribPointer tri");
	addmethod(init2, updete, draw2, deinit2, "glBufferData index tristrip");
	addmethod(init2_sub, updete, draw2, deinit2, "glBufferSubData index tristrip");
}
