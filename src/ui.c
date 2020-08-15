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

struct ui_overlay_item {
	struct ui_overlay_s *conf;
	SDL_Texture *tex;
	struct ui_overlay_item *next;
};

struct ui_s {
	font_ctx *font;
	SDL_Renderer *rend;
	Uint32 last_ticks;
	struct ui_overlay_item *list;
};

ui *ui_init(SDL_Renderer *rend)
{
	ui *ui = SDL_calloc(1, sizeof(struct ui_s));
	if(ui == NULL)
		return NULL;

	ui->font = FontStartup(rend);
	if(ui->font == NULL)
		return NULL;

	ui->last_ticks = SDL_GetTicks();
	ui->rend = rend;
	return ui;
}

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
