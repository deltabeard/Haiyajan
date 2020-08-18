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

struct ui_s {
	font_ctx *font;
	SDL_Renderer *rend;
};

struct ui_overlay_item {
	struct ui_overlay_item *prev;

	SDL_Colour text_colour;
	ui_overlay_corner_e corner;

	char *text;
	Uint8 free_text;

	Uint8 disp_count;
	char *(*get_new_str)(void *priv);
	void *priv;

	SDL_Texture *tex;
	struct ui_overlay_item *next;
};

ui_overlay_item_s *ui_add_overlay(ui_overlay_ctx **ctx, SDL_Colour text_colour,
		ui_overlay_corner_e corner, char *text, Uint8 disp_count,
		char *(*get_new_str)(void *priv), void *priv,
		Uint8 free_text)
{
	ui_overlay_item_s *list = *ctx;

	/* Obtain the last item in the linked list. */
	while(list != NULL && list->next != NULL)
		list = list->next;

	if(list != NULL)
	{
		/* Create a new overlay. */
		list->next = SDL_malloc(sizeof(ui_overlay_item_s));
		if(list->next == NULL)
			goto err;

		list->next->prev = list;
		list = list->next;
	}
	else
	{
		/* If no overlay exists yet, create the first overlay and set
		 * the context to the first item. */
		*ctx = SDL_malloc(sizeof(ui_overlay_item_s));
		list = *ctx;
		if(list == NULL)
			goto err;

		list->prev = NULL;
	}

	list->text_colour = text_colour;
	list->corner = corner;
	list->text = text;
	list->free_text = free_text;
	list->disp_count = disp_count;
	list->get_new_str = get_new_str;
	list->priv = priv;
	list->tex = NULL;
	list->next = NULL;
	return list;

err:
	return NULL;
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

	free(item);
	return;
}

void ui_overlay_delete_all(ui_overlay_ctx **p)
{
	while(*p != NULL)
		ui_overlay_delete(p, *p);
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

		/* Check if timer has triggered. If zero, the overlay is not
		 * deleted on timeout. */
		if(ctx->disp_count > 0)
		{
			ctx->disp_count--;
			if(ctx->disp_count == 0)
			{
				ui_overlay_delete(p, ctx);
				ctx = next;
				continue;
			}
		}

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

			/* TODO: Add return checks. */
			/* If a new string isn't required, and a texture has
			 * not already been cached, create a texture and cache
			 * it. */
			ctx->tex = SDL_CreateTexture(rend,
					SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_TARGET, txtw, txth);
			SDL_SetTextureBlendMode(ctx->tex, SDL_BLENDMODE_BLEND);
			SDL_SetRenderTarget(rend, ctx->tex);
			SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0x40);
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

void *ui_add_overlay(ui *ui, struct ui_overlay_s *conf)
{
	struct ui_overlay_item *iter = ui->list;

	while(iter != NULL)
		iter = iter->next;

       	iter = SDL_malloc(sizeof(struct ui_overlay_item));
	if(iter == NULL)
		goto out;

	iter->conf = SDL_malloc(sizeof(struct ui_overlay_s));
	if(iter->conf == NULL)
	{
		free(iter);
		iter = NULL;
		goto out;
	}

	memcpy(iter->conf, conf, sizeof(struct ui_overlay_s));

out:
	return iter;
}

void ui_del_overlay(ui *ui, void *id)
{
	struct ui_overlay_item *iter = ui->list;
	struct ui_overlay_item *prev = NULL;

	while(iter != NULL)
	{
		if(iter != id)
		{
			prev = iter;
			iter = iter->next;
			continue;
		}

		if(prev != NULL)
			prev->next = iter->next;

		free(iter->conf->text);
		free(iter->conf);
		free(iter);
		return;
	}

	/* Invalid ID or overlay already timed out. */
	return;
}

SDL_Texture *ui_render_overlays(ui *ui, SDL_Texture *tex)
{
	const SDL_Colour c[4] = {
		/* White */
		{ 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE },
		/* Green */
		{ 0x00, 0xA5, 0x68, SDL_ALPHA_OPAQUE },
		/* Red */
		{ 0xC5, 0x00, 0x30, SDL_ALPHA_OPAQUE },
		/* Yellow */
		{ 0xFF, 0xD4, 0x00, SDL_ALPHA_OPAQUE }
	};
	SDL_Rect r[4] = {
		{ 0 },
		{ 0 },
		{ 0 },
		{ 0 }
	};
	int w, h;
	struct ui_overlay_item *iter = ui->list;
	Uint32 delta = 1;

	/* TODO: add checks. */
	if(tex == NULL)
	{
		SDL_GetRendererOutputSize(ui->rend, &w, &h);
		tex = SDL_CreateTexture(ui->rend, SDL_PIXELFORMAT_RGBA32,
				SDL_TEXTUREACCESS_TARGET, w, h);
	}
	else
	{
		SDL_QueryTexture(tex, NULL, NULL, &w, &h);
		SDL_SetRenderTarget(ui->rend, tex);
		SDL_SetRenderDrawColor(ui->rend, 0, 0, 0, 0);
		SDL_RenderClear(ui->rend);
	}

	/* Resolve origin coordinates for the four corners. */
	r[UI_OVERLAY_TOP_RIGHT].x = w;
	r[UI_OVERLAY_BOTTOM_RIGHT].x = w;
	r[UI_OVERLAY_BOTTOM_RIGHT].y = h;
	r[UI_OVERLAY_TOP_RIGHT].y = h;

	if(SDL_GetTicks() > ui->last_ticks)
		delta = SDL_GetTicks() - ui->last_ticks;

	while(iter != NULL)
	{
		SDL_Rect *dst = &r[iter->conf->corner];

		/* Check if overlay has timed out and must either be deleted
		 * or refreshed. */
		if(iter->conf->timer_func == 1)
		{
			iter->conf->timer.refresh.ms -= delta;
			if(iter->conf->timer.refresh.ms <= 0)
			{
				free(iter->conf->text);
				iter->conf->text = iter->conf->timer.refresh.ui_overlay_get_str(iter->conf->timer.refresh.priv);
				SDL_DestroyTexture(iter->tex);
				iter->tex = NULL;
			}
		}
		else
		{
			iter->conf->timer.timeout_ms -= delta;
			if(iter->conf->timer.timeout_ms <= 0)
			{
				struct ui_overlay_item *temp = iter->next;
				ui_del_overlay(ui, iter);
				iter = temp;
				continue;
			}
		}

		/* Recreate overlay. */
		if(iter->tex == NULL)
		{
			unsigned ov_sz_w, ov_sz_h;
			FontDrawSize(strlen(iter->conf->text),
					&ov_sz_w, &ov_sz_h);

			iter->tex = SDL_CreateTexture(ui->rend,
					SDL_PIXELFORMAT_RGBA32,
					SDL_TEXTUREACCESS_TARGET, ov_sz_w,
					ov_sz_h);
			SDL_SetRenderTarget(ui->rend, iter->tex);

			SDL_SetRenderDrawColor(ui->rend, c[iter->conf->colour].r,
					c[iter->conf->colour].g,
					c[iter->conf->colour].b,
					c[iter->conf->colour].a);

			FontPrintToRenderer(ui->font, iter->conf->text, NULL);
			dst->w = ov_sz_w;
			dst->h = ov_sz_h;
		}

		SDL_QueryTexture(iter->tex, NULL, NULL, &dst->w, &dst->h);

		/* Render overlays. */
		SDL_SetRenderTarget(ui->rend, tex);
		switch(iter->conf->corner)
		{
			case UI_OVERLAY_TOP_LEFT:
				SDL_RenderCopy(ui->rend, iter->tex, NULL, dst);
				dst->x += dst->h;
				break;

			case UI_OVERLAY_TOP_RIGHT:
				break;

			case UI_OVERLAY_BOTTOM_RIGHT:
				break;

			case UI_OVERLAY_BOTTOM_LEFT:
				break;
		}

		iter = iter->next;
	}

	ui->last_ticks = SDL_GetTicks();
	return tex;
}
#endif

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
