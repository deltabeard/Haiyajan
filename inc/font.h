/**
 * Font rendering library for SDL2.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <SDL.h>

#define FONT_CHAR_WIDTH		9
#define FONT_CHAR_HEIGHT	15

/**
 * Basic process of using SDL2_Picofont to draw text onto a renderer:
 *
 * ctx = FontStartup(renderer);
 * FontPrintToRenderer(ctx, "Text to draw", NULL);
 * FontExit(ctx);
 *
 * If your platform supports it, you can set your render target to a texture,
 * and then call FontPrintToRenderer() to print text to the texture instead.
 * Use the function FontDrawSize() to ensure that your texture is large enough.
 */

/**
 * Context required to store the generated texture with the given renderer.
 */
typedef struct font_ctx_s font_ctx;

/**
 * Initialises font context with given renderer. Must be called first.
 * The given renderer must remain valid until after FontExit() is called.
 *
 * \param renderer	Renderer of the window.
 * \return Font context, else error. Use SDL_GetError().
 */
font_ctx *FontStartup(SDL_Renderer *renderer);

/**
 * Prints a string to the SDL2 renderer.
 * Use SDL_SetRenderDrawColor() to change the text colour.
 * Use SDL_SetRenderTarget() to render to a texture instead.
 *
 * \param ctx	Font library context.
 * \param text	Text to print.
 * \param dstscale	Location and scale of text. (0,0) x1 if NULL.
 * \return	0 on success, else error. Use SDL_GetError().
 */
int FontPrintToRenderer(font_ctx *const ctx, const char *text,
			const SDL_Rect *dstscale);

/**
 * Expected size of texture/renderer required given the string length, assuming
 * a scale of 1.
 *
 * \param text_len	Length of text.
 * \param w		Pointer to store expected width.
 * \param h		Pointer to store expected height.
 */
void FontDrawSize(const unsigned text_len, unsigned *w, unsigned *h);

/**
 * Deletes font context.
 */
void FontExit(font_ctx *ctx);
