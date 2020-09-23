/**
 * Tool assisted input.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 *
 * Portions of this file originate from the sdlarch project, the license of
 * which is reproduced below. Note that this file is relicensed to GNU AGPLv3.
 */

#pragma once

 /**
  * Haiyajan Tool Assisted Input file format .htai
  * Design to be as universal as possible, and for future extensions without
  * breaking compatibility.
  *
  * Structure:
  * u8 magic[10]
  * u8 version
  * u8 file_version
  * char rom_target[32]
  * char emu_target[32]
  * char emu_version[32]
  * char tai_desc[32]
  * char tai_author[32]
  * u64 frame_num
  * u8 command;
  * while command != HTAI_CMD_END
  *	command_data;
  * 	command;
  * end
  *
  * magic
  * 	Unique set of bytes used to identify the file.
  *	These bytes must be:
  *		0xAB, 'h', 't', 'a', 'i', 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
  *
  * version
  *	File format version. This specification is for version 1.
  *
  * file_version
  *	Version information of the file itself.
  *
  * rom_target
  *	Name of the target ROM name. Should only be used as a weak sign of
  *	compatibility with the loaded game.
  *	String may not be null terminated if its length is 32 characters.
  *
  * emu_target
  *	Name of the target emulator name. Should only be used as a weak sign of
  *	compatibility with the emulator.
  *	String may not be null terminated if its length is 32 characters.
  *
  * emu_version
  *	Emulator version the file was created with.
  *	String may not be null terminated if its length is 32 characters.
  *
  * tai_desc
  *	Descriptive of the file.
  *	String may not be null terminated if its length is 32 characters.
  *
  * tai_author
  *	Author of the file.
  *	String may not be null terminated if its length is 32 characters.
  *
  * frame_num
  *  The frame number to execute command. Following frame numbers must either
  *  be the same or higher.
  *
  * command
  *	Command to execute. Must be one of:
  *		HTAI_CMD_END = 0
  *		HTAI_CMD_QUIT
  *		HTAI_CMD_KEYBOARD
  *		HTAI_CMD_TEXTEDIT
  *		HTAI_CMD_TEXTINPUT
  *		HTAI_CMD_MOUSE_MOTION
  *		HTAI_CMD_MOUSE_BUTTON
  *		HTAI_CMD_MOUSE_WHEEL
  *		HTAI_CMD_CONTROLLER_AXIS
  *		HTAI_CMD_CONTROLLER_BUTTON
  *		HTAI_CMD_CONTROLLER_DEVICE
  *		HTAI_CMD_TOUCH_FINGER
  *
  * command_data
  *	HTAI_CMD_END:
  *		N/A. No more input remaining. The program stays running.
  *	HTAI_CMD_QUIT:
  *		N/A. The program should immediately quit.
  *	HTAI_CMD_KEYBOARD:
  *		u8 state
  *		u8 unused
  *		u16_t keymod
  *		u32_t scancode
  *		u32_t keycode
  *	HTAI_CMD_TEXTEDIT:
  *		char[32] text
  *		s32_t start
  *		s32_t length
  *	HTAI_CMD_TEXTINPUT:
  *		char[32] text
  *		s32_t start
  *		s32_t length
  *	HTAI_CMD_MOUSE_MOTION:
  *		u32_t state
  *		s32 x
  *		s32 y
  *	HTAI_CMD_MOUSE_BUTTON:
  *		u8 button
  *		u8 state
  *		u8 clicks
  *		u8 unused
  *		s32 x
  *		s32 y
  *	HTAI_CMD_MOUSE_WHEEL:
  *		s32 x
  *		s32 y
  *		u32 direction
  *	HTAI_CMD_CONTROLLER_AXIS:
  *		u8 axis
  *		u8 unused
  *		s16 value
  *	HTAI_CMD_CONTROLLER_BUTTON:
  *		u8 button
  *		u8 state
  *		u16 unused
  *	HTAI_CMD_CONTROLLER_DEVICE:
  *		u8 button
  *		u8 state
  *		u16 unused
  *	HTAI_CMD_TOUCH_FINGER:
  *		u8 type
  *		u8 unused[3]
  *		f32 x
  *		f32 y
  *		f32 pressure
  */

#include <SDL.h>

typedef struct tai_s tai;

/**
 * Initialise tool assisted input context.
 *
 * \param out		File to read or write to.
 * \param record	Whether to record a new file, or play back an existing file.
 * 			If record is SDL_TRUE, then out must point to a writeable
 * 			buffer.
 * \return		Tool assisted input context.
 */
tai *tai_init(SDL_RWops *f, SDL_bool record);

/**
 * Process event by either reading from HTAI file and generating an SDL_Event,
 * or by storing the given SDL_Event to a new HTAI file.
 *
 * \param ctx		Tool assisted input context.
 * \param e		SDL_Event to record. Ignored when reading existing file.
 */
int tai_process_event(tai *ctx, SDL_Event *e);

void tai_next_frame(tai *ctx);

/**
 * Close and free the tool assisted input context. Finishes the recording
 * context. The SDL_RWops context will be closed.
 */
int tai_exit(tai *ctx);
