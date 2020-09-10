#include "desktop.h"

#include "sanewm.h"
#include "list.h"
#include "window.h"

void
delete_from_workspace(struct client *client)
{
	if (client->ws < 0)
		return;
	/* delete_item(&workspace_list[client->ws], client->workspace_item); */
	delete_item(&focused_monitor->window_workspace_list[client->ws], client->workspace_item);
	client->workspace_item = NULL;
	client->ws = -1;
}

// TODO Not necessary
/* void */
/* delete_from_monitor_workspace(struct client *client) */
/* { */
/*	if (client->ws < 0) */
/*		return; */
/*	/\* delete_item(&workspace_list[client->ws], client->workspace_item); *\/ */
/*	delete_item(&focused_monitor->window_workspace_list[client->ws], client->workspace_item); */
/*	client->workspace_item = NULL; */
/*	client->ws = -1; */
/* } */

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

/* Add a window, specified by client, to workspace ws. */
void
add_to_workspace(struct client *client, uint32_t ws)
{
	/* struct item *item = add_item(&workspace_list[ws]); */
	struct item *item = add_item(&focused_monitor->window_workspace_list[ws]);

	if (client == NULL)
		return;

	if (NULL == item)
		return;

	/* Remember our place in the workspace window list. */
	client->workspace_item = item;
	client->ws = ws;
	item->data         = client;

	/* Set window hint property so we can survive a crash. Like "fixed" */
	if (!client->fixed)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL,
				    32, 1, &ws);
}

// TODO This might not be necessary because above function already uses "focused workspace"
void
add_to_monitor_workspace(struct client *client, struct monitor *monitor, uint32_t ws)
{
	/* struct item *item = add_item(&workspace_list[ws]); */
	struct item *item = add_item(&monitor->window_workspace_list[ws]);

	if (client == NULL)
		return;

	if (NULL == item)
		return;

	// NOTE HERE Item is the item in the workspace list
	// TODO This actually solves the item problem
	/* Remember our place in the workspace window list so it's circular....*/
	client->workspace_item = item;
	// TODO This is problematic (i.e. the workspace variable)
	// Should have this Maybe map this with monitor;
	// PROG Kinda fixed by identifiying the monitor
	client->ws = ws;
	client->monitor = monitor;
	item->data = client;

	// TODO Does this work with ws?
	/* Set window hint property so we can survive a crash. Like "fixed" */
	if (!client->fixed)
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL,
				    32, 1, &ws);
}

/* Change current workspace to ws */
void
change_workspace_helper(const uint32_t ws)
{
	xcb_query_pointer_reply_t *pointer;
	struct client *client;
	struct item *item;
	if (ws == current_workspace)
		return;
	xcb_ewmh_set_current_desktop(ewmh, 0, ws);

	/* Go through list of current ws.
	 * Unmap everything that isn't fixed. */
	/* TODO Make this asynchronous if possible */
	/* TODO Make this loop through focuse monitor as well */
	/* for (item = workspace_list[current_workspace]; item != NULL;) { */
	for (item = focused_monitor->window_workspace_list[current_workspace]; item != NULL;) {
		client = item->data;
		item = item->next;
		set_borders(client, false);
		if (!client->fixed){
			xcb_unmap_window(conn, client->id);
		} else {
			// correct order is delete first add later.
			delete_from_workspace(client);
			add_to_workspace(client, ws);
		}
	}
	/* for (item = workspace_list[ws]; item != NULL; item = item->next) { */
	for (item = focused_monitor->window_workspace_list[ws]; item != NULL;) {
		client = item->data;
		if (!client->fixed && !client->iconic)
			xcb_map_window(conn, client->id);
	}
	current_workspace = ws;
	pointer = xcb_query_pointer_reply(conn, xcb_query_pointer(conn,
								  screen->root), 0);
	if (pointer == NULL)
		set_focus(NULL);
	else {
		set_focus(find_client(&pointer->child));
		free(pointer);
	}
}


void
send_to_workspace(const Arg *arg)
{
	if (NULL == focus_window ||
	    focus_window->fixed ||
	    arg->i == current_workspace)
		return;
	// correct order is delete first add later.
	delete_from_workspace(focus_window);
	add_to_workspace(focus_window, arg->i);
	xcb_unmap_window(conn, focus_window->id);
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
