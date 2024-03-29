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

/**
 * Overlay system.
 * Automatically arranges and renders overlays, or notifications, on to the
 * renderer.
 *
 * Process of using overlay system:
 *  - Create an overlay with ui_add_overlay()
 *  - Render the overlay to the set render target with ui_overlay_render()
 *  - Optional: Manually delete an overlay with ui_overlay_delete()
 *  - Optional: Free all overlays with ui_overlay_delete_all()
 */

typedef enum {
	ui_overlay_top_left = 0,
	ui_overlay_top_right = 1,
	ui_overlay_bot_left = 2,
	ui_overlay_bot_right = 3
} ui_overlay_corner_e;

/* Private structures. */
typedef struct ui_overlay_item ui_overlay_item_s;
typedef struct ui_overlay_item ui_overlay_ctx;

/**
 * Add a new overlay.
 *
 * \param ui_overlay_ctx Private overlay context pointer. Must be initialised to
 * 			NULL.
 * \param text_colour	The text colour.
 * \param corner	The corner in which to display the overlay. The overlay
 * 			will be displayed from the corner selected to the
 * 			vertical center of the screen.
 * \param text		Static text to render, or NULL if dynamic text is to be
 * 			acquired from the get_new_str function.
 * 			String must be null terminated.
 * \param timeout_ms	Number of milliseconds to show the overlay for. Set to
 * 			0 for no automatic deletion.
 * \param get_new_str	The function to call to obtain new text on each render.
 * 			If get_new_str returns NULL, the overlay is deleted.
 * 			This function pointer must be NULL for static text.
 * \param priv		Private pointer to supply to the get_new_str function.
 * 			Unused when get_new_str is NULL.
 * \param free_text	Set to non-zero to call SDL_free automatically on text
 * 			when overlay is deleted.
 * \return		Context for specific overlay. NULL on failure.
 */
ui_overlay_item_s *ui_add_overlay(ui_overlay_ctx **ctx, SDL_Colour text_colour,
		ui_overlay_corner_e corner, char *text, Uint32 timeout_ms,
		char *(*get_new_str)(void *priv), void *priv,
		Uint8 free_text);

/**
 * Render the overlays added to the list to the current renderer.
 *
 * \param ctx	Overlay context.
 * \param rend	SDL Renderer.
 * \param font	Font context.
 * \return	0 on success, else failure.
 */
int ui_overlay_render(ui_overlay_ctx **ctx, SDL_Renderer *rend, font_ctx *font);

/**
 * Delete a specific overlay.
 *
 * \param p	Overlay context.
 * \param item	Specific overlay to delete.
 */
void ui_overlay_delete(ui_overlay_ctx **p, ui_overlay_item_s *item);

/**
 * Delete all overlays.
 * \param p	Overlay context.
 */
void ui_overlay_delete_all(ui_overlay_ctx **p);

typedef struct ui_s	ui;
ui *ui_init(SDL_Renderer *rend);
void draw_menu(ui *ui, menu_ctx *menu);
void launch_menu(ui *ui, menu_ctx *menu);
