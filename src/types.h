/* Copyright (c) 2017-2020 Ian Park
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SANEWM_TYPES_H
#define SANEWM_TYPES_H

#include <inttypes.h>
#include <stdbool.h>

#include <xcb/randr.h>

#include "definitions.h"

///---Types---///
struct monitor {
	xcb_randr_output_t id;
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;                // Width/Height in pixels.
	struct list_item *item;              // Pointer to our place in output list.
	struct list_item *window_workspace_list[WORKSPACES]; // pointer to list of workspaces
};

typedef union {
	const char** com;
	const uint32_t i;
} Arg;

typedef struct {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(const Arg *);
	const Arg arg;
} key;

typedef struct {
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *);
	const Arg arg;
	const bool root_only;
} Button;

struct window_geometry {
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
};

struct sane_window {                     // Everything we know about a window.
	xcb_drawable_t id;              // ID of this window.should be xcb window t
	bool set_by_user;                 // X,Y was set by -geom.
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
	uint8_t  depth;                 // pixel depth
	struct window_geometry original_geometry;        // Original size if we're currently maxed.
	uint16_t max_width;
	uint16_t max_height;
	uint16_t min_width;
	uint16_t min_height;
	uint16_t width_increment;
	uint16_t height_increment;
	uint16_t base_width;
	uint16_t base_height;
	bool fixed;
	bool unkillable;
	bool vertical_maxed;
	bool horizontal_maxed;
	bool maxed;
	bool vertical_horizontal;
	bool ignore_borders;
	bool iconic;
	struct monitor *monitor;        // The physical output this window is on.
	struct list_item *window_item;       // Pointer to our place in global windows list.
	struct list_item *workspace_item;    // Pointer to workspace window list.
	struct list_item *monitor_item;    // Pointer to monitor window list.
	int ws;                         // In which workspace this window belongs to.
};

struct winconf {                    // Window configuration data.
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
	uint8_t stack_mode;
	xcb_window_t sibling;
};

struct conf {
	int8_t border_width;             // Do we draw borders for non-focused window? If so, how large?
	int8_t outer_border;            // The size of the outer border
	uint32_t focus_color;
	uint32_t unfocus_color;
	uint32_t fixed_color;
	uint32_t unkill_color;
	uint32_t empty_color;
	uint32_t fixed_unkill_color;
	uint32_t outer_border_color;
	bool inverted_colors;
	bool enable_compton;
};

#endif
