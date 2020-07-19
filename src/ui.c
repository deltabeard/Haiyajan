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

ui *ui_init(font_ctx *font, SDL_Renderer *rend)
{
	ui *ui = SDL_malloc(sizeof(ui));
	ui->font = font;
	ui->rend = rend;
	return ui;
}

void draw_menu(ui *ui, menu_ctx *menu)
{
	SDL_Rect bg = { .x = 10, .y = 10, .w = 110, .h = 62 };
	SDL_Rect txtloc = { .x = 17, .y = 13, .w = 2, .h = 2 };

	SDL_SetRenderDrawColor(ui->rend, 51, 138, 224, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(ui->rend, &bg);

	for(unsigned long i = 0; i < menu->items_nmemb; i++)
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
