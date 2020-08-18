/**
 * Unofficial libretro externsions.
 *
 * Copyright (C) 2020 by Mahyar Koshkouei <mk@deltabeard.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

struct license_info_s {
	enum license_type_e {
		LICENSE_TYPE_OTHER = 0,
		LICENSE_TYPE_PUBLIC_DOMAIN,
		LICENSE_TYPE_PERMISSIVE,
		LICENSE_TYPE_COPYLEFT,
		LICENSE_TYPE_NONCOMMERCIAL,
		LICENSE_TYPE_PROPRIETARY
	} license_type;

	/**
	 * A full name used to represent the license.
	 * Such as "Mozilla Public License 2.0".
	 * If this string is NULL, then the core does not specify a full name
	 * for its license.
	 * The core guarantees that this string is valid until retro_deinit()
	 * is called.
	 */
	const char *license_fullname;

	/**
	 * A short name used to represent the license.
	 * A list of identifiers may be found at https://spdx.org/licenses/
	 * It is expected that the number of characters within this string is 32
	 * or less. The front end may choose to truncate this string if it is
	 * longer than 32 characters.
	 * If this string is NULL, then the core does not specify a license.
	 * The core guarantees that this string is valid until retro_deinit()
	 * is called.
	 */
	const char *license_spdx;

	/**
	 * A *HTTPS* URL to the license terms and conditions.
	 * If possible, the URL should be the official publication of the
	 * license terms, not just a copy found at your own repository.
	 * The URL must be HTTPS. Using HTTP, or other protocol here is
	 * forbidden.
	 * If license_type is not LICENSE_TYPE_OTHER, and license_fullname or
	 * license_spdx are not NULL, then license_url must be a valid URL.
	 * Otherwise, license_url may be NULL.
	 * The core guarantees that this string is valid until retro_deinit()
	 * is called.
	 */
	const char *license_url;

	/**
	 * A *HTTPS* URL to the project code repository. If you project code
	 * repository is not HTTPS, fix your website.
	 * The core guarantees that this string is valid until retro_deinit()
	 * is called.
	 */
	const char *project_repo;
};

/**
 * Obtain license information from the core.
 * May be called by the frontend at any time.
 */
const struct license_info_s *re_core_get_license_info(void);

/**
 * If a core does not use framerate for timing purposes, it might not be able to
 * detect when the front end paused gameplay.
 * This function may be used for the front end to specifically pause and play
 * the libretro core as required. This function may be called by the front end
 * any time between the first call to retro_run() and before retro_deinit().
 *
 * \param pause	Set to 1 to pause the core, or 0 to play.
 */
void re_core_set_pause(int pause);
