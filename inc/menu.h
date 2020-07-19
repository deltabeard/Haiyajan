/**
 * Copyright (C) 2020 by Mahyar Koshkouei <mk@deltabeard.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once
#include <stdlib.h>

#define MENU_SELECT_ITEM(menu, sel)				\
	do{							\
		if((sel - 1) < menu->items_nmemb)		\
			menu->item_selected = sel;		\
	}while(0)

enum ui_menu_type_e {
	/* Show a small menu. */
	UI_MENU_TYPE_SMALL_MENU = 0,

	/* This menu uses as much space as possible on the screen.
	 * Should be used for file selection. */
	UI_MENU_TYPE_LARGE_MENU,

	/* This menu has two entries: "ON" and "OFF". The menu text *must* be
	 * "ON" and "OFF", and only those two entries. */
	UI_MENU_TYPE_ON_OFF
};
typedef enum ui_menu_type_e ui_menu_type;

struct menu_ctx_s
{
	struct menu_ctx_s *parent;
	const char *title;
	const char *help;
	unsigned long item_selected;
	unsigned long items_nmemb;
	const struct menu_item_s *items;
	ui_menu_type type;
};
typedef struct menu_ctx_s menu_ctx;

enum menu_instruction_e {
	/* Go back to the previous item.
	 * Could be used when user presses UP. */
	MENU_INSTR_PREV_ITEM,

	/* Go to next item in menu.
	 * Could be used when user presses DOWN. */
	MENU_INSTR_NEXT_ITEM,

	/* Go to parent menu if one exists.
	 * Could be used when user presses BACKSPACE. */
	MENU_INSTR_PARENT_MENU,

	/* Execute item operation.
	 * Could be used when user presses ENTER. */
	MENU_INSTR_EXEC_ITEM
};
typedef enum menu_instruction_e menu_instruction;

struct menu_item_s
{
	const char *name;
	const char *help;
	enum menu_op_e
	{
		/* Opens a sub menu. */
		MENU_SUB_MENU,

		/* Executes a function. */
		MENU_EXEC_FUNC,

		/* Sets the value of an integer. */
		MENU_SET_VAL
	} op;

	union param_u
	{
		/* Pointer to sub menu. */
		menu_ctx *sub_menu;

		/* Pointer to function to execute if item selected. */
		struct exec_func_s
		{
			void *ctx;
			void (*func)(void *ctx);
		} exec_func;

		/* Pointer to integer to set. */
		struct set_val_s
		{
			int val;
			int *set;
		} set_val;
	} param;
};
typedef struct menu_item_s menu_item;

/**
 * Add an item to a given menu. First item shown first in menu.
 */
void menu_init(menu_ctx *menu, menu_ctx *parent, const char *title,
		const char *help, unsigned long items_nmemb, menu_item *items);
void menu_set_items(menu_ctx *menu, unsigned long nmemb, menu_item *items);
menu_ctx *menu_instruct(menu_ctx *ctx, menu_instruction instr);
