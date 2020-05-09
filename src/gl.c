/**
 * OpenGL to SDL2 Texture renderer.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 *
 * Portions of this file originate from the sdlarch project, the license of
 * which is reproduced below. Note that this file is relicensed to GNU AGPLv3.
 */

#if 0
Copyright(c) 2015, Higor Eurípedes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

*Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

* Neither the name of sdlarch nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
		SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <libretro.h>
#include <gl.h>

struct gl_shader
{
	GLuint vao;
	GLuint vbo;
	GLuint program;

	GLint i_pos;
	GLint i_coord;
	GLint u_tex;
	GLint u_mvp;
};

struct gl_fn
{
	GLuint (*glCreateShader)(GLenum type);
	void (*glCompileShader)(GLuint shader);
	void (*glShaderSource)(GLuint shader, GLsizei count,
			const GLchar *const *string, const GLint *length);
	void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
	void (*glGetShaderInfoLog)(GLuint shader, GLsizei bufSize,
				   GLsizei *length, GLchar *infoLog);
	GLuint (*glCreateProgram)(void);
	void (*glAttachShader)(GLuint program, GLuint shader);
	void (*glLinkProgram)(GLuint program);
	void (*glDeleteShader)(GLuint shader);
	void (*glValidateProgram)(GLuint program);
	void (*glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
	void (*glGetProgramInfoLog)(GLuint program, GLsizei bufSize,
				    GLsizei *length, GLchar *infoLog);
	GLint (*glGetAttribLocation)(GLuint program, const GLchar *name);
	GLint (*glGetUniformLocation)(GLuint program, const GLchar *name);
	void (*glGenVertexArrays)(GLsizei n, GLuint *arrays);
	void (*glGenBuffers)(GLsizei n, GLuint *buffers);
	void (*glUseProgram)(GLuint program);
	void (*glUniform1i)(GLint location, GLint v0);
	void (*glUniformMatrix4fv)(GLint location, GLsizei count,
				   GLboolean transpose, const GLfloat *value);
	void (*glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
	void (*glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void (*glRenderbufferStorage)(GLenum target, GLenum internalformat,
				      GLsizei width, GLsizei height);
	void (*glFramebufferRenderbuffer)(GLenum target, GLenum attachment,
					  GLenum renderbuffertarget, GLuint renderbuffer);
	void (*glBindVertexArray)(GLuint array);
	void (*glBindBuffer)(GLenum target, GLuint buffer);
	void (*glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
	void (*glEnableVertexAttribArray)(GLuint index);
	void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
	void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
};

struct gl_ctx_s
{
	/* Set by core. */
	unsigned char depth : 1;
	unsigned char stencil : 1;
	unsigned char bottom_left_origin : 1;
	unsigned char reset : 1;
	Uint8 framebuffer;
	retro_hw_context_reset_t context_reset;
	retro_hw_context_reset_t context_destroy;

	/* Internal. */
	int w, h;
	SDL_Renderer *rend;
	SDL_Texture *tex;
	struct gl_shader gl_sh;
	struct gl_fn fn;
};

uintptr_t get_current_framebuffer(void)
{
	/* FIXME */
	return 1;
}

static int gl_init_fn(glctx *ctx)
{
	struct gl_fn_gen_s {
		const char *fn_str;
		void **fn;
	} const fngen[] = {
		{ "glCreateShader",	(void **)&ctx->fn.glCreateShader },
		{ "glCompileShader",	(void **)&ctx->fn.glCompileShader },
		{ "glShaderSource",	(void **)&ctx->fn.glShaderSource },
		{ "glGetShaderiv",	(void **)&ctx->fn.glGetShaderiv },
		{ "glGetShaderInfoLog",	(void **)&ctx->fn.glGetShaderInfoLog },
		{ "glCreateProgram",	(void **)&ctx->fn.glCreateProgram },
		{ "glAttachShader",	(void **)&ctx->fn.glAttachShader },
		{ "glLinkProgram",	(void **)&ctx->fn.glLinkProgram },
		{ "glDeleteShader",	(void **)&ctx->fn.glDeleteShader },
		{ "glValidateProgram",	(void **)&ctx->fn.glValidateProgram },
		{ "glGetProgramiv",	(void **)&ctx->fn.glGetProgramiv },
		{ "glGetProgramInfoLog",(void **)&ctx->fn.glGetProgramInfoLog },
		{ "glGetAttribLocation",(void **)&ctx->fn.glGetAttribLocation },
		{ "glGetUniformLocation",(void **)&ctx->fn.glGetUniformLocation },
		{ "glGenVertexArrays",	(void **)&ctx->fn.glGenVertexArrays },
		{ "glGenBuffers",	(void **)&ctx->fn.glGenBuffers },
		{ "glUseProgram",	(void **)&ctx->fn.glUseProgram },
		{ "glUniform1i",	(void **)&ctx->fn.glUniform1i },
		{ "glUniformMatrix4fv",	(void **)&ctx->fn.glUniformMatrix4fv },
		{ "glGenRenderbuffers",	(void **)&ctx->fn.glGenRenderbuffers },
		{ "glBindRenderbuffer",	(void **)&ctx->fn.glBindRenderbuffer },
		{ "glRenderbufferStorage",(void **)&ctx->fn.glRenderbufferStorage },
		{ "glFramebufferRenderbuffer",(void **)&ctx->fn.glFramebufferRenderbuffer },
		{ "glBindVertexArray",	(void **)&ctx->fn.glBindVertexArray },
		{ "glBindBuffer",	(void **)&ctx->fn.glBindBuffer },
		{ "glBufferData",	(void **)&ctx->fn.glBufferData },
		{ "glEnableVertexAttribArray",(void **)&ctx->fn.glEnableVertexAttribArray },
		{ "glVertexAttribPointer",(void **)&ctx->fn.glVertexAttribPointer },
		{ "glDrawArrays",	(void **)&ctx->fn.glDrawArrays }
	};
	int ret = 0;

	for(size_t i = 0; i < SDL_arraysize(fngen); i++)
	{
		*fngen[i].fn = SDL_GL_GetProcAddress(fngen[i].fn_str);
		if(*fngen[i].fn == NULL)
		{
			ret = -1;
			SDL_LogVerbose(SDL_LOG_CATEGORY_RENDER,
				"GL function %s not found", fngen[i].fn_str);
		}
	}

	return ret;
}

static GLuint compile_shader(glctx *ctx, GLenum type, GLsizei count,
			     const char *const *strings)
{
    GLuint shader = ctx->fn.glCreateShader(type);
    ctx->fn.glShaderSource(shader, count, strings, NULL);
    ctx->fn.glCompileShader(shader);

    GLint status;
    ctx->fn.glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
 	char buffer[256];
	ctx->fn.glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
	SDL_SetError("Failed to compile %s shader: %s",
			type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			buffer);
    }

    return shader;
}

static void ortho2d(float m[4][4], float left, float right, float bottom,
		    float top)
{
    m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
    m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
    m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
    m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -1.0f;
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
}


static void init_shaders(glctx *ctx)
{
	const char *g_vshader_src =
	"#version 150\n"
	"in vec2 i_pos;\n"
	"in vec2 i_coord;\n"
	"out vec2 o_coord;\n"
	"uniform mat4 u_mvp;\n"
	"void main() {\n"
	"o_coord = i_coord;\n"
	"gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
	"}";

	const char *g_fshader_src =
	"#version 150\n"
	"in vec2 o_coord;\n"
	"uniform sampler2D u_tex;\n"
	"void main() {\n"
	"gl_FragColor = texture2D(u_tex, o_coord);\n"
	"}";

	GLuint vshader = compile_shader(ctx, GL_VERTEX_SHADER, 1, &g_vshader_src);
	GLuint fshader = compile_shader(ctx, GL_FRAGMENT_SHADER, 1, &g_fshader_src);
	GLuint program = ctx->fn.glCreateProgram();

	SDL_assert(program);

	ctx->fn.glAttachShader(program, vshader);
	ctx->fn.glAttachShader(program, fshader);
	ctx->fn.glLinkProgram(program);

	ctx->fn.glDeleteShader(vshader);
	ctx->fn.glDeleteShader(fshader);

	ctx->fn.glValidateProgram(program);

	GLint status;
	ctx->fn.glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status == GL_FALSE)
	{
		char buffer[256];
		ctx->fn.glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
		SDL_SetError("Failed to link shader program: %s", buffer);
	}

	ctx->gl_sh.program = program;
	ctx->gl_sh.i_pos   = ctx->fn.glGetAttribLocation(program,  "i_pos");
	ctx->gl_sh.i_coord = ctx->fn.glGetAttribLocation(program,  "i_coord");
	ctx->gl_sh.u_tex   = ctx->fn.glGetUniformLocation(program, "u_tex");
	ctx->gl_sh.u_mvp   = ctx->fn.glGetUniformLocation(program, "u_mvp");

	ctx->fn.glGenVertexArrays(1, &ctx->gl_sh.vao);
	ctx->fn.glGenBuffers(1, &ctx->gl_sh.vbo);

	ctx->fn.glUseProgram(ctx->gl_sh.program);

	ctx->fn.glUniform1i(ctx->gl_sh.u_tex, 0);

	float m[4][4];
	if(ctx->bottom_left_origin)
		ortho2d(m, -1, 1, 1, -1);
	else
		ortho2d(m, -1, 1, -1, 1);

	ctx->fn.glUniformMatrix4fv(ctx->gl_sh.u_mvp, 1, GL_FALSE, (float *)m);

	ctx->fn.glUseProgram(0);
}

static void refresh_vertex_data(glctx *ctx, int w, int h)
{
	int tex_w, tex_h;

	if(SDL_QueryTexture(ctx->tex, NULL, NULL, &tex_w, &tex_h) != 0)
		return;

	const float bottom = (float)h / tex_h;
	const float right  = (float)w / tex_w;

	const float vertex_data[] = {
		// pos, coord
		-1.0f, -1.0f,  0.0f,  bottom, // left-bottom
		-1.0f,  1.0f,  0.0f,  0.0f,   // left-top
		 1.0f, -1.0f, right,  bottom,// right-bottom
		 1.0f,  1.0f, right,  0.0f,  // right-top
	};

	ctx->fn.glBindVertexArray(ctx->gl_sh.vao);

	ctx->fn.glBindBuffer(GL_ARRAY_BUFFER, ctx->gl_sh.vbo);
	ctx->fn.glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);

	ctx->fn.glEnableVertexAttribArray(ctx->gl_sh.i_pos);
	ctx->fn.glEnableVertexAttribArray(ctx->gl_sh.i_coord);
	ctx->fn.glVertexAttribPointer(ctx->gl_sh.i_pos, 2, GL_FLOAT, GL_FALSE,
				      sizeof(float) * 4, 0);
	ctx->fn.glVertexAttribPointer(ctx->gl_sh.i_coord, 2, GL_FLOAT, GL_FALSE,
				      sizeof(float) * 4, (void *)(2 * sizeof(float)));

	ctx->fn.glBindVertexArray(0);
	ctx->fn.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

glctx *gl_init(SDL_Renderer *rend, SDL_Texture *tex,
			   struct retro_hw_render_callback *lrhw)
{
	SDL_RendererInfo info;
	glctx *ctx;
	int access, major, minor, w, h;
	Uint32 format;

	if(SDL_GetRendererInfo(rend, &info) != 0)
		return NULL;

	if(SDL_strncmp(info.name, "opengl", 6) != 0)
	{
		SDL_SetError("Renderer %s is not compatible with OpenGL",
					 info.name);
		return NULL;
	}

	if(SDL_QueryTexture(tex, &format, &access, &w, &h) != 0)
		return NULL;

	if((access & SDL_TEXTUREACCESS_TARGET) == 0)
	{
		SDL_SetError("Texture does not support render target");
		return NULL;
	}

	if(format != SDL_PIXELFORMAT_RGB888)
	{
		SDL_SetError("Texture was not RGB888");
		return NULL;
	}

	if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) != 0 ||
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor) != 0)
		return NULL;

	if(major < lrhw->version_major ||
			(major == lrhw->version_major && minor < lrhw->version_minor))
	{
		SDL_SetError("The initialised version of OpenGL (%d.%d) does "
					 "not meet the minimum requested (%u.%u)",
					 major, minor,
					 lrhw->version_major, lrhw->version_minor);
		return NULL;
	}

	/* The parameters have passed all checks by this point. */
	if((ctx = SDL_calloc(1, sizeof(glctx))) == NULL)
		return NULL;

	if(gl_init_fn(ctx) != 0)
	{
		SDL_SetError("One or more required OpenGL functions are "
				"unavailable on this platform");
		return NULL;
	}

	ctx->depth = lrhw->depth != 0;
	ctx->stencil = lrhw->stencil != 0;
	ctx->bottom_left_origin = lrhw->bottom_left_origin != 0;
	ctx->context_reset = lrhw->context_reset;
	ctx->context_destroy = lrhw->context_destroy;
	lrhw->get_current_framebuffer = get_current_framebuffer;
	lrhw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;

	init_shaders(ctx);

	SDL_SetRenderTarget(ctx->rend, ctx->tex);

	if(ctx->depth && ctx->stencil)
	{
		GLuint rbo_id;
		ctx->fn.glGenRenderbuffers(1, &rbo_id);
		ctx->fn.glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
		ctx->fn.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

		ctx->fn.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
						  GL_DEPTH_STENCIL_ATTACHMENT,
				    GL_RENDERBUFFER, rbo_id);
	}
	else if(ctx->depth)
	{
		GLuint rbo_id;
		ctx->fn.glGenRenderbuffers(1, &rbo_id);
		ctx->fn.glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
		ctx->fn.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

		ctx->fn.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
						  GL_DEPTH_ATTACHMENT,
				    GL_RENDERBUFFER, rbo_id);
	}
	SDL_RenderClear(ctx->rend);

	refresh_vertex_data(ctx, w, h);

	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "OpenGL %s initialised",
			glGetString(GL_VERSION));
	return ctx;
}

void gl_set_texture(glctx *ctx, SDL_Texture *tex)
{
	if(ctx != NULL)
		ctx->tex = tex;
}

void gl_prerun(glctx *ctx)
{
	if(ctx->reset == 0)
	{
		ctx->context_reset();
		ctx->reset = 1;
	}
	SDL_SetRenderTarget(ctx->rend, ctx->tex);
	SDL_GL_BindTexture(ctx->tex, NULL, NULL);
}

void gl_postrun(glctx *ctx, const SDL_Rect *screen_dim)
{
	if(screen_dim->w == 0 || screen_dim->h == 0)
		return;

	if(screen_dim->w != ctx->w || screen_dim->h != ctx->h)
	{
		ctx->w = screen_dim->w;
		ctx->h = screen_dim->h;

		refresh_vertex_data(ctx, screen_dim->w, screen_dim->h);
	}

	ctx->fn.glUseProgram(ctx->gl_sh.program);
	ctx->fn.glBindVertexArray(ctx->gl_sh.vao);
	ctx->fn.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ctx->fn.glBindVertexArray(0);

	ctx->fn.glUseProgram(0);

	SDL_GL_UnbindTexture(ctx->tex);
	SDL_RenderFlush(ctx->rend); /* TODO: Check if this is required. */
	SDL_SetRenderTarget(ctx->rend, NULL);
}

void gl_deinit(glctx *ctx)
{
	if(ctx != NULL && ctx->context_destroy != NULL)
		ctx->context_destroy();

	if(ctx != NULL)
	{
		SDL_free(ctx);
		ctx = NULL;
	}
}
