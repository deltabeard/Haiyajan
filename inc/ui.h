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

typedef struct ui_s ui;
ui *ui_init(font_ctx *font, SDL_Renderer *rend);
void draw_menu(ui *ui, menu_ctx *menu);
void launch_menu(ui *ui, menu_ctx *menu);
