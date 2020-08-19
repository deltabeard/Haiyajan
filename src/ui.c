/**
 * User interface for use with Haiyajan.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <menu.h>
#include <font.h>
#include <ui.h>
#include <SDL.h>

#define UI_OVERLAY_BG_ALPHA	0x40

struct ui_s {
	font_ctx *font;
	SDL_Renderer *rend;
};

static SDL_SpinLock overlay_lock = { 0 };
struct ui_overlay_tip {
	ui_overlay_ctx **p;
	ui_overlay_item_s *item;
};
static Uint32 ui_overlay_timeout(Uint32 interval, void *param);

struct ui_overlay_item {
	struct ui_overlay_item *prev;

	SDL_Colour text_colour;
	ui_overlay_corner_e corner;

	char *text;
	Uint8 free_text;

	char *(*get_new_str)(void *priv);
	void *priv;

	SDL_Texture *tex;
	struct ui_overlay_item *next;
};

ui_overlay_item_s *ui_add_overlay(ui_overlay_ctx **ctx, SDL_Colour text_colour,
		ui_overlay_corner_e corner, char *text, Uint32 timeout_ms,
		char *(*get_new_str)(void *priv), void *priv,
		Uint8 free_text)
{
	ui_overlay_item_s *list;
	ui_overlay_item_s *new = SDL_malloc(sizeof(ui_overlay_item_s));
	struct ui_overlay_tip *tim = SDL_malloc(sizeof(struct ui_overlay_tip));

	/* Allocate new item. */
	if(new == NULL || tim == NULL)
		return NULL;

	SDL_AtomicLock(&overlay_lock);
	list = *ctx;

	/* Obtain the last item in the linked list. */
	while(list != NULL && list->next != NULL)
		list = list->next;

	if(list != NULL)
	{
		/* Create a new overlay. */
		list->next = new;
		if(list->next == NULL)
			goto err;

		list->next->prev = list;
		list = list->next;
	}
	else
	{
		/* If no overlay exists yet, create the first overlay and set
		 * the context to the first item. */
		*ctx = new;
		list = *ctx;
		if(list == NULL)
			goto err;

		list->prev = NULL;
	}

	list->text_colour = text_colour;
	list->corner = corner;
	list->text = text;
	list->free_text = free_text;
	list->get_new_str = get_new_str;
	list->priv = priv;
	list->tex = NULL;
	list->next = NULL;

	if(timeout_ms != 0)
	{
		tim->p = ctx;
		tim->item = list;
		SDL_AddTimer(timeout_ms, ui_overlay_timeout, tim);
	}

out:
	SDL_AtomicUnlock(&overlay_lock);
	return list;

err:
	list = NULL;
	goto out;
}

void ui_overlay_delete(ui_overlay_ctx **p, ui_overlay_item_s *item)
{
	if(item->free_text)
		SDL_free(item->text);

	/* If this is the tip of the linked list, then set the next item as the
	 * tip. If there is no next item, then the tip is set to NULL. */
	if(*p == item)
		*p = item->next;

	if(item->next != NULL)
		item->next->prev = item->prev;

	if(item->prev != NULL)
		item->prev->next = item->next;

	SDL_free(item);
	return;
}

void ui_overlay_delete_all(ui_overlay_ctx **p)
{
	while(*p != NULL)
		ui_overlay_delete(p, *p);
}

Uint32 ui_overlay_timeout(Uint32 interval, void *param)
{
	struct ui_overlay_tip *tim = param;
	(void) interval;

	SDL_AtomicLock(&overlay_lock);
	ui_overlay_delete(tim->p, tim->item);
	SDL_AtomicUnlock(&overlay_lock);
	SDL_free(param);
	return 0;
}

int ui_overlay_render(ui_overlay_ctx **p, SDL_Renderer *rend, font_ctx *font)
{
	int w, h;
	int ret = 0;
	Uint8 corner_use[4] = { 0 };
	ui_overlay_ctx *ctx = *p;

	SDL_RenderGetLogicalSize(rend, &w, &h);

	while(ctx != NULL)
	{
		ui_overlay_item_s *next = ctx->next;
		SDL_Colour c = ctx->text_colour;
		unsigned txth, txtw;
		const unsigned margin = 2;
		const unsigned padding = 2;
		SDL_Rect dst;

		/* Get new string if requested. */
		if(ctx->get_new_str != NULL)
			ctx->text = ctx->get_new_str(ctx->priv);

		/* If text is NULL, then delete overlay. */
		if(ctx->text == NULL)
		{
			ui_overlay_delete(p, ctx);
			ctx = next;
			continue;
		}

		FontDrawSize(strlen(ctx->text), &txtw, &txth);
		txtw += margin;
		txth += margin;

		if(ctx->tex == NULL)
		{
			SDL_Texture *prev_targ = SDL_GetRenderTarget(rend);
			SDL_Rect txt_dst = { margin, margin, 1, 1 };

			/* If a new string isn't required, and a texture has
			 * not already been cached, create a texture and cache
			 * it. */
			ctx->tex = SDL_CreateTexture(rend,
					SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_TARGET, txtw, txth);
			if(ctx->tex == NULL)
				continue;

			SDL_SetTextureBlendMode(ctx->tex, SDL_BLENDMODE_BLEND);
			SDL_SetRenderTarget(rend, ctx->tex);
			SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00,
					UI_OVERLAY_BG_ALPHA);
			SDL_RenderClear(rend);

			SDL_SetRenderDrawColor(rend, c.r, c.g, c.b, c.a);
			FontPrintToRenderer(font, ctx->text, &txt_dst);
			SDL_SetRenderTarget(rend, prev_targ);
		}

		/* Render text */
		dst.w = txtw;
		dst.h = txth;

		/* Add one pixel space between overlays. */
		if(corner_use[ctx->corner] != 0)
			txth += padding;

		switch(ctx->corner)
		{
		case ui_overlay_top_left:
			dst.x = 0;
			dst.y = txth * corner_use[ctx->corner];
			break;

		case ui_overlay_top_right:
			dst.x = w - txtw;
			dst.y = txth * corner_use[ctx->corner];
			break;

		case ui_overlay_bot_left:
			dst.x = 0;
			dst.y = h - (txth * (corner_use[ctx->corner] + 1));
			break;

		case ui_overlay_bot_right:
			dst.x = w - txtw;
			dst.y = h - (txth * (corner_use[ctx->corner] + 1));
			break;
		}

		corner_use[ctx->corner]++;
		SDL_RenderCopy(rend, ctx->tex, NULL, &dst);

		if(ctx->get_new_str != NULL)
		{
			SDL_DestroyTexture(ctx->tex);
			ctx->tex = NULL;
		}

		ctx = next;
	}

	return ret;
}

#if 0

ui *ui_init(SDL_Renderer *rend)
{
	ui *ui = SDL_calloc(1, sizeof(struct ui_s));
	if(ui == NULL)
		return NULL;

	ui->font = FontStartup(rend);
	if(ui->font == NULL)
	{
		free(ui);
		return NULL;
	}

	ui->rend = rend;
	return ui;
}

void draw_menu(ui *ui, menu_ctx *menu)
{
	SDL_Rect bg = { 10, 10, 110, 62 };
	SDL_Rect txtloc = { 17, 13, 2, 2 };
	unsigned i;

	SDL_SetRenderDrawColor(ui->rend, 51, 138, 224, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(ui->rend, &bg);

	for(i = 0; i < menu->items_nmemb; i++)
	{
		/* Set font colour. */
		if(i == menu->item_selected)
		{
			SDL_Rect hlloc = txtloc;
			hlloc.x -= 3;
			hlloc.w = bg.w - 3;
			hlloc.h = txtloc.h * FONT_CHAR_HEIGHT;

			SDL_RenderDrawRect(ui->rend, &bg);
			SDL_SetRenderDrawColor(ui->rend, 255, 255, 94, SDL_ALPHA_OPAQUE);

			SDL_SetRenderDrawColor(ui->rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		}
		else
		{
			SDL_SetRenderDrawColor(ui->rend, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
		}
	}

}

void launch_menu(ui *ui, menu_ctx *menu)
{
	(void) ui;
	(void) menu;
	return;
}

#endif
