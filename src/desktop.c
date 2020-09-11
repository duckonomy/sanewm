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
change_workspace(const Arg *arg)
{
	change_workspace_helper(arg->i);
}

void
next_workspace()
{
	current_workspace == WORKSPACES - 1 ? change_workspace_helper(0)
		:change_workspace_helper(current_workspace + 1);
}

void
previous_workspace()
{
	current_workspace > 0 ? change_workspace_helper(current_workspace - 1)
		: change_workspace_helper(WORKSPACES - 1);
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
send_to_next_workspace(const Arg *arg)
{
	Arg arg2 = { .i=0 };
	Arg arg3 = { .i=current_workspace + 1 };
	current_workspace == WORKSPACES - 1 ? send_to_workspace(&arg2):send_to_workspace(&arg3);
}

void
send_to_previous_workspace(const Arg *arg)
{
	Arg arg2 = {.i=current_workspace - 1};
	Arg arg3 = {.i=WORKSPACES-1};
	current_workspace > 0 ? send_to_workspace(&arg2) : send_to_workspace(&arg3);
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
