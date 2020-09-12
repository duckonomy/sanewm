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

#include "desktop.h"

#include "sanewm.h"
#include "list.h"
#include "window.h"

void
delete_from_workspace(struct sane_window *window)
{
	if (window->ws < 0)
		return;
	delete_item(&workspace_list[window->ws], window->workspace_item);
	window->workspace_item = NULL;
	window->ws = -1;
}

void
delete_from_workspace_monitor(struct sane_window *window)
{
	if (window->monitor_ws < 0)
		return;
	delete_item(&current_monitor->window_workspace_list[window->ws], window->monitor_workspace_item);
	if (window->monitor_workspace_item)
		window->monitor_workspace_item = NULL;
	window->monitor_ws = -1;
}

void
change_workspace(const Arg *arg)
{
	change_workspace_helper(arg->i);
}

void
change_workspace_monitor(const Arg *arg)
{
	change_workspace_helper_monitor(arg->i);
}


void
next_workspace()
{
	current_workspace == WORKSPACES - 1 ? change_workspace_helper(0) :
		change_workspace_helper(current_workspace + 1);
}


void
next_workspace_monitor()
{
	current_monitor->workspace == WORKSPACES - 1 ? change_workspace_helper_monitor(0) :
		change_workspace_helper(current_monitor->workspace + 1);

}

void
previous_workspace()
{
	current_workspace > 0 ? change_workspace_helper(current_workspace - 1) :
		change_workspace_helper(WORKSPACES - 1);
}

void
previous_workspace_monitor()
{
	current_monitor->workspace > 0 ? change_workspace_helper(current_monitor->workspace - 1) :
		change_workspace_helper_monitor(WORKSPACES - 1);
}

/* Add a window, specified by window, to workspace ws. */
void
add_to_workspace(struct sane_window *window, uint32_t ws)
{
	struct list_item *item = add_item(&workspace_list[ws]);

	if (window == NULL)
		return;

	if (NULL == item)
		return;

	/* Remember our place in the workspace window list. */
	window->workspace_item	= item;
	window->ws		= ws;
	item->data		= window;

	/* Set window hint property so we can survive a crash. Like "fixed" */
	if (!window->fixed)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL,
				    32, 1, &ws);
}

/* Add a window, specified by window, to workspace ws. */
void
add_to_workspace_monitor(struct sane_window *window, uint32_t ws)
{
	struct list_item *item = add_item(&current_monitor->window_workspace_list[ws]);

	if (window == NULL)
		return;

	if (NULL == item)
		return;

	/* Remember our place in the workspace window list. */
	window->monitor_workspace_item	= item;
	window->monitor_ws		= ws;
	item->data			= window;

	/* Set window hint property so we can survive a crash. Like "fixed" */
	if (!window->fixed)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL,
				    32, 1, &ws);
}

/* Change current workspace to ws */
void
change_workspace_helper(const uint32_t ws)
{
	xcb_query_pointer_reply_t *pointer;
	struct sane_window *window;
	struct list_item *item;
	if (ws == current_workspace)
		return;
	xcb_ewmh_set_current_desktop(ewmh, 0, ws);

	/* Go through list of current ws.
	 * Unmap everything that isn't fixed. */
	for (item = workspace_list[current_workspace]; item != NULL;) {
		window = item->data;
		item = item->next;
		set_window_borders(window, false);
		if (!window->fixed){
			xcb_unmap_window(conn, window->id);
		} else {
			// correct order is delete first add later.
			delete_from_workspace(window);
			add_to_workspace(window, ws);
		}
	}
	for (item = workspace_list[ws]; item != NULL; item = item->next) {
		window = item->data;
		if (!window->fixed && !window->iconic)
			xcb_map_window(conn, window->id);
	}
	current_workspace = ws;
	pointer = xcb_query_pointer_reply(conn, xcb_query_pointer(conn,
								  screen->root), 0);
	if (pointer == NULL)
		focus_window(NULL);
	else {
		focus_window(find_window(&pointer->child));
		free(pointer);
	}
}

/* TODO FIXME PROG Change current workspace on current monitor */
void
change_workspace_helper_monitor(const uint32_t ws)
{
	xcb_query_pointer_reply_t *pointer;
	struct sane_window *window;
	struct list_item *item;
	if (ws == current_monitor->workspace)
		return;

	// Change with the extended setting or translate somewhere
	xcb_ewmh_set_current_desktop(ewmh, 0, ws);

	/* Go through list of current ws.
	 * Unmap everything that isn't fixed. */
	/* for (item = workspace_list[current_workspace]; item != NULL;) { */
	for (item = current_monitor->window_workspace_list[current_monitor->workspace]; item != NULL;) {
		window = item->data;
		item = item->next;
		set_window_borders(window, false);
		if (!window->fixed){
			xcb_unmap_window(conn, window->id);
		} else {
			// correct order is delete first, add later.
			delete_from_workspace_monitor(window);
			add_to_workspace_monitor(window, ws);
		}
	}
	for (item = current_monitor->window_workspace_list[ws]; item != NULL; item = item->next) {
		window = item->data;
		// No iconic anymore? -> maybe iconized ones in notification
		if (!window->fixed && !window->iconic)
			xcb_map_window(conn, window->id);
	}
	current_monitor->workspace = ws;
	pointer = xcb_query_pointer_reply(conn, xcb_query_pointer(conn,
								  screen->root), 0);
	if (pointer == NULL)
		focus_window(NULL);
	else {
		focus_window(find_window(&pointer->child));
		free(pointer);
	}
}

void
send_to_workspace(const Arg *arg)
{
	if (NULL == current_window ||
	    current_window->fixed ||
	    arg->i == current_workspace)
		return;
	// correct order is delete first add later.
	delete_from_workspace(current_window);
	add_to_workspace(current_window, arg->i);
	xcb_unmap_window(conn, current_window->id);
	xcb_flush(conn);
}

void
send_to_workspace_monitor(const Arg *arg)
{
	if (NULL == current_window ||
	    current_window->fixed ||
	    arg->i == current_monitor->workspace)
		return;
	// correct order is delete first add later.
	delete_from_workspace_monitor(current_window);
	add_to_workspace_monitor(current_window, arg->i);
	xcb_unmap_window(conn, current_window->id);
	xcb_flush(conn);
}

void
send_to_next_workspace(const Arg *arg)
{
	Arg arg2 = { .i = 0 };
	Arg arg3 = { .i = current_workspace + 1 };
	current_workspace == WORKSPACES - 1 ? send_to_workspace(&arg2) : send_to_workspace(&arg3);
}

void
send_to_next_workspace_monitor(const Arg *arg)
{
	Arg arg2 = { .i = 0 };
	Arg arg3 = { .i = current_monitor->workspace + 1 };
	current_monitor->workspace == WORKSPACES - 1 ? send_to_workspace_monitor(&arg2) : send_to_workspace_monitor(&arg3);
}

void
send_to_previous_workspace(const Arg *arg)
{
	Arg arg2 = {.i = current_workspace - 1};
	Arg arg3 = {.i = WORKSPACES - 1};
	current_workspace > 0 ? send_to_workspace_monitor(&arg2) : send_to_workspace_monitor(&arg3);
}

void
send_to_previous_workspace_monitor(const Arg *arg)
{
	Arg arg2 = {.i = current_monitor->workspace - 1};
	Arg arg3 = {.i = WORKSPACES - 1};
	current_monitor->workspace > 0 ? send_to_workspace_monitor(&arg2) : send_to_workspace_monitor(&arg3);
}

uint32_t
get_wm_desktop(xcb_drawable_t win)
{
	xcb_get_property_cookie_t cookie = xcb_get_property(conn, false, win, ewmh->_NET_WM_DESKTOP,
							    XCB_GET_PROPERTY_TYPE_ANY, 0, sizeof(uint32_t));
	xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
	if (NULL == reply || 0 == xcb_get_property_value_length(reply)) { /* 0 if we didn't find it. */
		if (NULL != reply)
			free(reply);
		return SANEWM_NOWS;
	}
	uint32_t wsp = *(uint32_t *)xcb_get_property_value(reply);
	if (NULL != reply)
		free(reply);
	return wsp;
}
