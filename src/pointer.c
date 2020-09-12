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

#include <string.h>

#include "pointer.h"
#include "types.h"
#include "sanewm.h"
#include "config.h"

xcb_cursor_t
create_font_cursor(xcb_connection_t *conn, uint16_t glyph)
{
	static xcb_font_t cursor_font;

	cursor_font = xcb_generate_id (conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");
	xcb_cursor_t cursor = xcb_generate_id (conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font, glyph,
				glyph + 1, 0x3232, 0x3232, 0x3232, 0xeeee, 0xeeee, 0xeeec);

	return cursor;
}


/* Function to make the cursor move with the keyboard */
void
cursor_move(const Arg *arg)
{
	uint16_t speed; uint8_t cases = arg->i % 4;
	arg->i < 4 ? (speed = movements[3]) : (speed = movements[2]);

	if (cases == SANEWM_CURSOR_UP)
		xcb_warp_pointer(conn, XCB_NONE, XCB_NONE,
				 0, 0, 0, 0, 0, -speed);
	else if (cases == SANEWM_CURSOR_DOWN)
		xcb_warp_pointer(conn, XCB_NONE, XCB_NONE,
				 0, 0, 0, 0, 0, speed);
	else if (cases == SANEWM_CURSOR_RIGHT)
		xcb_warp_pointer(conn, XCB_NONE, XCB_NONE,
				 0, 0, 0, 0, speed, 0);
	else if (cases == SANEWM_CURSOR_LEFT)
		xcb_warp_pointer(conn, XCB_NONE, XCB_NONE,
				 0, 0, 0, 0, -speed, 0);

	xcb_flush(conn);
}


bool
get_pointer(const xcb_drawable_t *win, int16_t *x, int16_t *y)
{
	xcb_query_pointer_reply_t *pointer;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, *win), 0);
	if (NULL == pointer)
		return false;
	*x = pointer->win_x;
	*y = pointer->win_y;

	free(pointer);
	return true;
}


void
mouse_resize(struct sane_window *window, const int16_t rel_x, const int16_t rel_y)
{
	if (current_window->id == screen->root ||
	    current_window->maxed)
		return;

	window->width  = abs(rel_x);
	window->height = abs(rel_y);

	if (resize_by_line) {
		window->width -= (window->width - window->base_width)
			% window->width_increment;
		window->height -= (window->height - window->base_height)
			% window->height_increment;
	}

	resize_window_limit(window);
	window->vertical_maxed = false;
	window->horizontal_maxed  = false;
}

/* /\* Move window win as a result of pointer motion to coordinates rel_x,rel_y. *\/ */
/* void */
/* mouse_move(const int16_t rel_x, const int16_t rel_y) */
/* { */
/*	if (current_window == NULL || */
/*	    current_window->ws != current_workspace) */
/*		return; */

/*	current_window->x = rel_x; */
/*	current_window->y = rel_y; */

/*	if (borders[2] > 0) */
/*		snap_window(current_window); */

/*	move_window_limit(current_window); */
/* } */

/* Move window win as a result of pointer motion to coordinates rel_x,rel_y. */
void
mouse_move_monitor(const int16_t rel_x, const int16_t rel_y)
{
	if (current_window == NULL ||
	    current_window->monitor_ws != current_monitor->workspace)
		return;

	current_window->x = rel_x;
	current_window->y = rel_y;

	if (borders[2] > 0)
		snap_window_monitor(current_window);

	move_window_limit(current_window);
}


void
move_pointer_back(const int16_t start_x, const int16_t start_y,
		  const struct sane_window *window)
{
	if (start_x > (0 - conf.border_width - 1) &&
	    start_x < (window->width + conf.border_width + 1) &&
	    start_y > (0 - conf.border_width - 1) &&
	    start_y < (window->height + conf.border_width + 1))
		xcb_warp_pointer(conn, XCB_NONE, window->id, 0, 0, 0, 0, start_x, start_y);
}


void
center_pointer(xcb_drawable_t win, struct sane_window *window)
{
	int16_t cur_x, cur_y;

	cur_x = cur_y = 0;

	switch (CURSOR_POSITION) {
	case BOTTOM_RIGHT:
		cur_x += window->width;
	case BOTTOM_LEFT:
		cur_y += window->height; break;
	case TOP_RIGHT:
		cur_x += window->width;
	case TOP_LEFT:
		break;
	default:
		cur_x = window->width / 2;
		cur_y = window->height / 2;
	}

	xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0, cur_x, cur_y);
}


/* set the given window to listen to button events (presses / releases) */
void
grab_buttons(struct sane_window *c)
{
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		num_lock_mask,
		num_lock_mask | XCB_MOD_MASK_LOCK
	};

	for (unsigned int b = 0; b < LENGTH(buttons); ++b)
		if (!buttons[b].root_only) {
			for (unsigned int m = 0; m < LENGTH(modifiers); ++m)
				xcb_grab_button(conn, 1, c->id,
						XCB_EVENT_MASK_BUTTON_PRESS,
						XCB_GRAB_MODE_ASYNC,
						XCB_GRAB_MODE_ASYNC,
						screen->root, XCB_NONE,
						buttons[b].button,
						buttons[b].mask | modifiers[m]);
		}

	/* ungrab the left click, otherwise we can't use it
	 * we've previously grabbed the left click in the enter_notify function
	 * when not in sloppy mode
	 * though the name is counter-intuitive to the method
	 */
	for (unsigned int m = 0; m < LENGTH(modifiers); ++m) {
		xcb_ungrab_button(conn,
				  XCB_BUTTON_INDEX_1,
				  c->id,
				  modifiers[m]);
	}
}
