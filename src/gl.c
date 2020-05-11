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

struct gl_fn
{
	void (*glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
	void (*glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void (*glRenderbufferStorage)(GLenum target, GLenum internalformat,
				      GLsizei width, GLsizei height);
	void (*glFramebufferRenderbuffer)(GLenum target, GLenum attachment,
					  GLenum renderbuffertarget, GLuint renderbuffer);
	void (*glBindVertexArray)(GLuint array);
	void (*glBindBuffer)(GLenum target, GLuint buffer);
	void (*glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
};

struct gl_ctx_s
{
	/* Set by core. */
	unsigned char depth : 1;
	unsigned char stencil : 1;
	unsigned char bottom_left_origin : 1;
	retro_hw_context_reset_t context_reset;
	retro_hw_context_reset_t context_destroy;

	/* Internal. */
	SDL_Renderer *rend;
	SDL_Texture **tex;
	struct gl_fn fn;
};

static void (*internal_glGetIntegerv)(GLenum pname, GLint *data);
static unsigned framebuffer = 1;

uintptr_t get_current_framebuffer(void)
{
	/* The texture will be bound before retro_run() is called. */
	int result;
	internal_glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
	SDL_LogVerbose(SDL_LOG_CATEGORY_RENDER, "Using FBO %d", result);
	return result;
}

static int gl_init_fn(glctx *ctx)
{
	struct gl_fn_gen_s {
		const char *fn_str;
		void **fn;
	} const fngen[] = {
		{ "glGenRenderbuffers",	(void **)&ctx->fn.glGenRenderbuffers },
		{ "glBindRenderbuffer",	(void **)&ctx->fn.glBindRenderbuffer },
		{ "glRenderbufferStorage",(void **)&ctx->fn.glRenderbufferStorage },
		{ "glFramebufferRenderbuffer",(void **)&ctx->fn.glFramebufferRenderbuffer },
		{ "glBindVertexArray",	(void **)&ctx->fn.glBindVertexArray },
		{ "glBindBuffer",	(void **)&ctx->fn.glBindBuffer },
		{ "glBufferData",	(void **)&ctx->fn.glBufferData },
		{ "glGetIntegerv",	(void **)&internal_glGetIntegerv }
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

/**
 * FIXME: Do checks, but initialise context in reset function.
 */
glctx *gl_init(SDL_Renderer *rend, SDL_Texture **tex,
			   struct retro_hw_render_callback *lrhw)
{
	SDL_RendererInfo info;
	glctx *ctx;
	int major, minor;

	if(SDL_GetRendererInfo(rend, &info) != 0)
		return NULL;

	if(SDL_strncmp(info.name, "opengl", 6) != 0)
	{
		SDL_SetError("Renderer %s is not compatible with OpenGL",
					 info.name);
		return NULL;
	}

#if 0
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "OpenGL %s in use",
		    glGetString(GL_VERSION));
#endif

	if((info.flags & SDL_RENDERER_TARGETTEXTURE) == 0)
	{
		SDL_SetError("Renderer does not support texture as a target");
		return NULL;
	}

#if 0
	if(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) != 0 ||
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor) != 0)
		return NULL;

	if(major < (int)lrhw->version_major ||
			(major == (int)lrhw->version_major &&
			minor < (int)lrhw->version_minor))
	{
		SDL_SetError("The initialised version of OpenGL (%d.%d) does "
					 "not meet the minimum requested (%u.%u)",
					 major, minor,
					 lrhw->version_major, lrhw->version_minor);
		return NULL;
	}
#endif

	/* The parameters have passed all checks by this point. */
	if((ctx = SDL_calloc(1, sizeof(glctx))) == NULL)
		return NULL;

	if(gl_init_fn(ctx) != 0)
	{
		SDL_SetError("One or more required OpenGL functions are "
				"unavailable on this platform");
		return NULL;
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_RENDER, "OpenGL functions initialised");

	ctx->depth = lrhw->depth != 0;
	ctx->stencil = lrhw->stencil != 0;
	ctx->bottom_left_origin = lrhw->bottom_left_origin != 0;
	ctx->context_reset = lrhw->context_reset;
	ctx->context_destroy = lrhw->context_destroy;
	ctx->rend = rend;
	ctx->tex = tex;
	lrhw->get_current_framebuffer = get_current_framebuffer;
	lrhw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;

	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "OpenGL initialisation successful");
	return ctx;
}

void gl_reset_context(const glctx *const ctx)
{
	int w, h;

	SDL_QueryTexture(*ctx->tex, NULL, NULL, &w, &h);
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

	ctx->context_reset();
	SDL_SetRenderTarget(ctx->rend, NULL);
}

void gl_prerun(glctx *ctx)
{
	SDL_SetRenderTarget(ctx->rend, *ctx->tex);
	SDL_GL_BindTexture(*ctx->tex, NULL, NULL);
}

void gl_postrun(glctx *ctx)
{
	SDL_GL_UnbindTexture(*ctx->tex);
	SDL_RenderFlush(ctx->rend); /* TODO: Check if this is required. */
	SDL_SetRenderTarget(ctx->rend, NULL);
}

void gl_deinit(glctx *ctx)
{
#if 0
	/* FIXME: Causes Seg fault. */
	if(ctx != NULL && ctx->context_destroy != NULL)
		ctx->context_destroy();
#endif

	if(ctx != NULL)
	{
		SDL_free(ctx);
		ctx = NULL;
	}
}
