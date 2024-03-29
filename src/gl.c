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

#include <SDL.h>
#include <SDL_opengl.h>
#include <libretro.h>
#include <gl.h>

/**
 * FIXME: Fix OpenGL ES2 in haiyajan.
 */

struct gl_shader {
	GLuint vao;
	GLuint vbo;
	GLuint program;

	GLint i_pos;
	GLint i_coord;
	GLint u_tex;
	GLint u_mvp;
};

struct gl_fn {
	GLuint (*glCreateShader)(GLenum type);
	void (*glCompileShader)(GLuint shader);
	void (*glShaderSource)(GLuint shader, GLsizei count,
			       const GLchar *const *string,
			       const GLint *length);
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
					  GLenum renderbuffertarget,
					  GLuint renderbuffer);
	void (*glBindVertexArray)(GLuint array);
	void (*glBindBuffer)(GLenum target, GLuint buffer);
	void (*glBufferData)(GLenum target, GLsizeiptr size, const void *data,
			     GLenum usage);
	void (*glPixelStorei)(GLenum pname, GLint param);
	void (*glBindTexture)(GLenum target, GLuint texture);
	void (*glGetIntegerv)(GLenum pname, GLint *data);
	const GLubyte *(*glGetString)(GLenum name);
	void (*glEnableVertexAttribArray)(GLuint index);
	void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type,
				      GLboolean normalized, GLsizei stride,
				      const void *pointer);
	void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
};

struct gl_ctx_s {
	/* Set by core. */
	Uint8 depth : 1;
	Uint8 stencil : 1;
	Uint8 bottom_left_origin : 1;
	retro_hw_context_reset_t context_reset;
	retro_hw_context_reset_t context_destroy;

	/* Internal. */
	SDL_Renderer *rend;
	SDL_Texture **tex;
	struct gl_shader gl_sh;
	struct gl_fn fn;
};

static unsigned framebuffer = 1;

uintptr_t get_current_framebuffer(void)
{
	return framebuffer;
}

static int gl_init_fn(gl_ctx *ctx)
{
	struct gl_fn_gen_s {
		const char *fn_str;
		void **fn;
	} const fngen[] = {
		{"glCreateShader",            (void **)&ctx->fn.glCreateShader},
		{"glCompileShader",           (void **)&ctx->fn.glCompileShader},
		{"glShaderSource",            (void **)&ctx->fn.glShaderSource},
		{"glGetShaderiv",             (void **)&ctx->fn.glGetShaderiv},
		{"glGetShaderInfoLog",        (void **)&ctx->fn.glGetShaderInfoLog},
		{"glCreateProgram",           (void **)&ctx->fn.glCreateProgram},
		{"glAttachShader",            (void **)&ctx->fn.glAttachShader},
		{"glLinkProgram",             (void **)&ctx->fn.glLinkProgram},
		{"glDeleteShader",            (void **)&ctx->fn.glDeleteShader},
		{"glValidateProgram",         (void **)&ctx->fn.glValidateProgram},
		{"glGetProgramiv",            (void **)&ctx->fn.glGetProgramiv},
		{"glGetProgramInfoLog",       (void **)&ctx->fn.glGetProgramInfoLog},
		{"glGetAttribLocation",       (void **)&ctx->fn.glGetAttribLocation},
		{"glGetUniformLocation",      (void **)&ctx->fn.glGetUniformLocation},
		{"glGenVertexArrays",         (void **)&ctx->fn.glGenVertexArrays},
		{"glGenBuffers",              (void **)&ctx->fn.glGenBuffers},
		{"glUseProgram",              (void **)&ctx->fn.glUseProgram},
		{"glUniform1i",               (void **)&ctx->fn.glUniform1i},
		{"glUniformMatrix4fv",        (void **)&ctx->fn.glUniformMatrix4fv},
		{"glGenRenderbuffers",        (void **)&ctx->fn.glGenRenderbuffers},
		{"glBindRenderbuffer",        (void **)&ctx->fn.glBindRenderbuffer},
		{"glRenderbufferStorage",     (void **)&ctx->fn.glRenderbufferStorage},
		{"glFramebufferRenderbuffer", (void **)&ctx->fn.glFramebufferRenderbuffer},
		{"glBindVertexArray",         (void **)&ctx->fn.glBindVertexArray},
		{"glBindBuffer",              (void **)&ctx->fn.glBindBuffer},
		{"glBufferData",              (void **)&ctx->fn.glBufferData},
		{"glPixelStorei",             (void **)&ctx->fn.glPixelStorei},
		{"glBindTexture",             (void **)&ctx->fn.glBindTexture},
		{"glGetIntegerv",             (void **)&ctx->fn.glGetIntegerv},
		{"glGetString",               (void **)&ctx->fn.glGetString},
		{"glEnableVertexAttribArray", (void **)&ctx->fn.glEnableVertexAttribArray},
		{"glVertexAttribPointer",     (void **)&ctx->fn.glVertexAttribPointer},
		{"glDrawArrays",              (void **)&ctx->fn.glDrawArrays}
	};
	int ret = 0;
	unsigned i = 0;

	for(i = 0; i < SDL_arraysize(fngen); i++)
	{
		*fngen[i].fn = SDL_GL_GetProcAddress(fngen[i].fn_str);
		if(*fngen[i].fn == NULL)
		{
			ret = -1;
			SDL_LogVerbose(SDL_LOG_CATEGORY_RENDER,
				       "GL function %s not found",
				       fngen[i].fn_str);
		}
	}

	return ret;
}

static GLuint compile_shader(struct gl_fn *fn, GLenum type, GLsizei count,
			     const char *const *strings)
{
	GLint status;
	
	GLuint shader = fn->glCreateShader(type);
	fn->glShaderSource(shader, count, strings, NULL);
	fn->glCompileShader(shader);
	fn->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE)
	{
		char buffer[256];
		fn->glGetShaderInfoLog(shader, sizeof(buffer), NULL,
					   buffer);
		SDL_SetError("Failed to compile %s shader: %s",
			     type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			     buffer);
	}

	return shader;
}

static void init_shaders(struct gl_fn *fn, struct gl_shader *sh)
{
	const char *const g_vshader_src =
		"attribute vec4 vPosition;"
		"void main() {"
		"  gl_Position = vPosition;"
		"}";

	const char *const g_fshader_src =
		"precision mediump float;"
		"uniform vec4 vColor;"
		"void main() {"
		"  gl_FragColor = vColor;"
		"}";
	GLuint vshader = compile_shader(fn, GL_VERTEX_SHADER, 1,
					&g_vshader_src);
	GLuint fshader = compile_shader(fn, GL_FRAGMENT_SHADER, 1,
					&g_fshader_src);
	GLuint program = fn->glCreateProgram();
	GLint status;

	SDL_assert(program);

	fn->glAttachShader(program, vshader);
	fn->glAttachShader(program, fshader);
	fn->glLinkProgram(program);

	fn->glDeleteShader(vshader);
	fn->glDeleteShader(fshader);

	fn->glValidateProgram(program);
	fn->glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status == GL_FALSE)
	{
		char buffer[256];
		fn->glGetProgramInfoLog(program, sizeof(buffer), NULL,
					    buffer);
		SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
			    "Failed to link shader program: %s", buffer);
	}

	sh->program = program;
	sh->i_pos = fn->glGetAttribLocation(program, "i_pos");
	sh->i_coord = fn->glGetAttribLocation(program, "i_coord");
	sh->u_tex = fn->glGetUniformLocation(program, "u_tex");
	sh->u_mvp = fn->glGetUniformLocation(program, "u_mvp");

	fn->glGenVertexArrays(1, &sh->vao);
	fn->glGenBuffers(1, &sh->vbo);
	fn->glUseProgram(sh->program);
	fn->glUniform1i(sh->u_tex, 0);
	fn->glUseProgram(0);
}

static void refresh_vertex_data(const gl_ctx *ctx, int w, int h)
{
	int tex_w, tex_h;

	if (SDL_QueryTexture(*ctx->tex, NULL, NULL, &tex_w, &tex_h) != 0)
		return;

	{
		const float bottom = (float)h / tex_h;
		const float right = (float)w / tex_w;

		const float vertex_data[] = {
			// pos, coord
			-1.0f, -1.0f, 0.0f, bottom, // left-bottom
			-1.0f, 1.0f, 0.0f, 0.0f,   // left-top
			1.0f, -1.0f, right, bottom,// right-bottom
			1.0f, 1.0f, right, 0.0f,  // right-top
		};

		ctx->fn.glBindVertexArray(ctx->gl_sh.vao);

		ctx->fn.glBindBuffer(GL_ARRAY_BUFFER, ctx->gl_sh.vbo);
		ctx->fn.glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data,
			GL_STREAM_DRAW);

		ctx->fn.glBindVertexArray(0);
		ctx->fn.glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

gl_ctx *gl_prepare(SDL_Renderer *rend)
{
	gl_ctx *ctx = SDL_calloc(1, sizeof(gl_ctx));

	if(ctx == NULL)
		return NULL;

	ctx->rend = rend;

	return ctx;
}

/**
 * FIXME: Do checks, but initialise context in reset function.
 */
int gl_init(gl_ctx *ctx, SDL_Texture **tex,
	    struct retro_hw_render_callback *lrhw)
{
	SDL_RendererInfo info;
	int major, minor;

	if(ctx == NULL)
	{
		SDL_SetError("GL context was not initialised");
		return SDL_FALSE;
	}

	if(SDL_GetRendererInfo(ctx->rend, &info) != 0)
	{
		SDL_SetError("Unable to obtain renderer information");
		return SDL_FALSE;
	}

	if(SDL_strncmp(info.name, "opengl", 6) != 0)
	{
		SDL_SetError("Renderer %s is not compatible with OpenGL",
			     info.name);
		return SDL_FALSE;
	}

	if((info.flags & SDL_RENDERER_TARGETTEXTURE) == 0)
	{
		SDL_SetError("Renderer does not support texture as a target");
		return SDL_FALSE;
	}

	if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) != 0 ||
	   SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor) != 0)
		return SDL_FALSE;

	SDL_LogVerbose(SDL_LOG_CATEGORY_RENDER, "SDL detected OpenGL %d.%d",
		       major, minor);

	if(major < (int)lrhw->version_major ||
	   (major == (int)lrhw->version_major &&
	    minor < (int)lrhw->version_minor))
	{
		SDL_SetError("The initialised version of OpenGL (%d.%d) does "
			     "not meet the minimum requested (%u.%u)",
			     major, minor,
			     lrhw->version_major, lrhw->version_minor);
		return SDL_FALSE;
	}

	if(gl_init_fn(ctx) != 0)
	{
		SDL_SetError("One or more required OpenGL functions are "
			     "unavailable on this platform");
		return SDL_FALSE;
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "OpenGL %s in use",
		    ctx->fn.glGetString(GL_VERSION));

	/* The parameters have passed all checks by this point. */
	ctx->tex = tex;

	{
		int result;
		ctx->fn.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
		framebuffer = result < 1 ? 1 : result;
	}

	ctx->depth = lrhw->depth != 0;
	ctx->stencil = lrhw->stencil != 0;
	ctx->bottom_left_origin = lrhw->bottom_left_origin != 0;
	ctx->context_reset = lrhw->context_reset;
	ctx->context_destroy = lrhw->context_destroy;
	lrhw->get_current_framebuffer = get_current_framebuffer;
	lrhw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;

	init_shaders(&ctx->fn, &ctx->gl_sh);
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
		    "OpenGL initialisation successful");
	return SDL_TRUE;
}

void gl_reset_context(const gl_ctx *const ctx)
{
	int access, w, h;

	SDL_QueryTexture(*ctx->tex, NULL, &access, &w, &h);
	SDL_SetRenderTarget(ctx->rend, *ctx->tex);

	if(ctx->depth && ctx->stencil)
	{
		GLuint rbo_id;
		ctx->fn.glGenRenderbuffers(1, &rbo_id);
		ctx->fn.glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
		ctx->fn.glRenderbufferStorage(GL_RENDERBUFFER,
					      GL_DEPTH24_STENCIL8, w, h);

		ctx->fn.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
						  GL_DEPTH_STENCIL_ATTACHMENT,
						  GL_RENDERBUFFER, rbo_id);
	}
	else if(ctx->depth)
	{
		GLuint rbo_id;
		ctx->fn.glGenRenderbuffers(1, &rbo_id);
		ctx->fn.glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
		ctx->fn.glRenderbufferStorage(GL_RENDERBUFFER,
					      GL_DEPTH_COMPONENT24, w, h);

		ctx->fn.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
						  GL_DEPTH_ATTACHMENT,
						  GL_RENDERBUFFER, rbo_id);
	}

	SDL_RenderClear(ctx->rend);
	refresh_vertex_data(ctx, w, h);
	SDL_SetRenderTarget(ctx->rend, NULL);
	ctx->context_reset();
}

void gl_prerun(gl_ctx *ctx)
{
	SDL_SetRenderTarget(ctx->rend, *ctx->tex);
	SDL_GL_BindTexture(*ctx->tex, NULL, NULL);

	ctx->fn.glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	ctx->fn.glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void gl_postrun(gl_ctx *ctx)
{
	ctx->fn.glUseProgram(ctx->gl_sh.program);
	ctx->fn.glBindVertexArray(ctx->gl_sh.vao);
	ctx->fn.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ctx->fn.glBindVertexArray(0);

	ctx->fn.glUseProgram(0);

	SDL_GL_UnbindTexture(*ctx->tex);
	SDL_SetRenderTarget(ctx->rend, NULL);
}

void gl_deinit(gl_ctx *ctx)
{
#if 0
	/* Causes segmentation fault currently. */
	if(ctx != NULL && ctx->context_destroy != NULL)
		ctx->context_destroy();
#endif

	if(ctx != NULL)
	{
		SDL_free(ctx);
		ctx = NULL;
	}
}
