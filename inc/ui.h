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

#pragma once
#include <SDL.h>
#include <menu.h>
#include <font.h>

typedef enum {
	ui_overlay_top_left = 0,
	ui_overlay_top_right = 1,
	ui_overlay_bot_left = 2,
	ui_overlay_bot_right = 3
} ui_overlay_corner_e;

typedef enum {
	ui_overlay_timeout = 0,
	ui_overlay_refresh = 1
} ui_overlay_timer_e;

typedef struct ui_overlay_item ui_overlay_item_s;
typedef struct ui_overlay_item ui_overlay_ctx;
ui_overlay_item_s *ui_add_overlay(ui_overlay_ctx **ctx, SDL_Colour text_colour,
		ui_overlay_corner_e corner, char *text, Uint8 disp_count,
		char *(*get_new_str)(void *priv), void *priv);
int ui_overlay_render(ui_overlay_ctx **ctx, SDL_Renderer *rend, font_ctx *font);
void ui_overlay_delete(ui_overlay_ctx **p, ui_overlay_item_s *item);
void ui_overlay_delete_all(ui_overlay_ctx **p);

typedef struct ui_s	ui;
ui *ui_init(SDL_Renderer *rend);
void draw_menu(ui *ui, menu_ctx *menu);
void launch_menu(ui *ui, menu_ctx *menu);
