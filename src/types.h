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
	struct item *item;              // Pointer to our place in output list.
	struct item *window_workspace_list[WORKSPACES]; // pointer to list of workspaces
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
	unsigned int mask, button;
	void (*func)(const Arg *);
	const Arg arg;
	const bool root_only;
} Button;

struct client_geometry {
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
};

struct client {                     // Everything we know about a window.
	xcb_drawable_t id;              // ID of this window.should be xcb window t
	bool set_by_user;                 // X,Y was set by -geom.
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
	uint8_t  depth;                 // pixel depth
	struct client_geometry original_geometry;        // Original size if we're currently maxed.
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
	struct item *window_item;       // Pointer to our place in global windows list.
	struct item *workspace_item;    // Pointer to workspace window list.
	struct item *monitor_item;    // Pointer to monitor window list.
	int ws;                         // In which workspace this window belongs to.
};

struct winconf {                    // Window configuration data.
	int16_t y;
	int16_t x;                      // X and Y.
	uint16_t width;
	uint16_t height;         // Width/Height in pixels.
	uint8_t      stack_mode;
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