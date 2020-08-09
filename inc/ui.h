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

struct ui_overlay_s {
	/* Colour of text. */
#define UI_OVERLAY_WHITE	0
#define UI_OVERLAY_GREEN	1
#define UI_OVERLAY_RED		2
#define UI_OVERLAY_YELLOW	3
	unsigned colour : 2;

#define UI_OVERLAY_TOP_LEFT	0
#define UI_OVERLAY_TOP_RIGHT	1
#define UI_OVERLAY_BOTTOM_RIGHT	2
#define UI_OVERLAY_BOTTOM_LEFT	3
	unsigned corner : 2;

#define UI_OVERLAY_SIZE_SMALL	0
#define UI_OVERLAY_SIZE_LARGE	1
	unsigned size : 1;

	/* Remove overlay on timeout. */
#define UI_OVERLAY_TIMER_TIMEOUT	0
#define UI_OVERLAY_TIMER_REFRESH	1
	unsigned timer_func : 1;

	union {
		/* Time till overlay is to be deleted. */
		Sint32 timeout_ms;

		/* Refresh Time till overlay is to have its text updated. */
		struct {
			Sint32 ms;
			void *priv;
			char *(*ui_overlay_get_str)(void *priv);
		} refresh;
	} timer;

	char *text;
};

typedef struct ui_s ui;
ui *ui_init(SDL_Renderer *rend);
void draw_menu(ui *ui, menu_ctx *menu);
void launch_menu(ui *ui, menu_ctx *menu);
