#include <stdbool.h>

#include <xcb/xcb_keysyms.h>

#include "events.h"
#include "sanewm.h"
#include "window.h"
#include "list.h"
#include "config.h"
#include "desktop.h"
#include "pointer.h"
#include "monitor.h"
#include "keys.h"

void
destroy_notify(xcb_generic_event_t *ev)
{
	struct client *cl;

	xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) ev;
	if (NULL != focus_window && focus_window->id == e->window)
		focus_window = NULL;

	cl = find_client(& e->window);

	/* Find this window in list of clients and forget about it. */
	if (NULL != cl)
		forget_window(cl->id);

	update_client_list();
}

void
enter_notify(xcb_generic_event_t *ev)
{
	xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;
	struct client *client;
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		num_lock_mask,
		num_lock_mask | XCB_MOD_MASK_LOCK
	};


	/*
	 * If this isn't a normal enter notify, don't bother. We also need
	 * ungrab events, since these will be generated on button and key
	 * grabs and if the user for some reason presses a button on the
	 * root and then moves the pointer to our window and releases the
	 * button, we get an Ungrab Enter_Notify. The other cases means the
	 * pointer is grabbed and that either means someone is using it for
	 * menu selections or that we're moving or resizing. We don't want
	 * to change focus in those cases.
	 */


	if (e->mode == XCB_NOTIFY_MODE_NORMAL ||
	    e->mode == XCB_NOTIFY_MODE_UNGRAB) {
		/* If we're entering the same window we focus now,
		 * then don't bother focusing. */

		if (NULL != focus_window && e->event == focus_window->id)
			return;


		/* Otherwise, set focus to the window we just entered if we
		 * can find it among the windows we know about.
		 * If not, just keep focus in the old window. */

		client = find_client(&e->event);
		if (NULL == client)
			return;

		/* skip this if not is_sloppy
		 * we'll focus on click instead (see button_press function)
		 * thus we have to grab left click button on that window
		 * the grab is removed at the end of the set_focus function,
		 * in the grab_buttons when not in sloppy mode
		 */
		if (!is_sloppy) {
			for (unsigned int m = 0; m < LENGTH(modifiers); ++m) {
				xcb_grab_button(conn,
						0, // owner_events => 0 means
						   // the grab_window won't
						   // receive this event
						client->id,
						XCB_EVENT_MASK_BUTTON_PRESS,
						XCB_GRAB_MODE_ASYNC,
						XCB_GRAB_MODE_ASYNC,
						screen->root, XCB_NONE,
						XCB_BUTTON_INDEX_1,
						modifiers[m]);
			}
			return;
		}

		set_focus(client);
		set_borders(client, true);
	}
}

void
unmap_notify(xcb_generic_event_t *ev)
{
	xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *)ev;
	struct client *client = NULL;
	/*
	 * Find the window in our current workspace list, then forget about it.
	 * Note that we might not know about the window we got the Unmap_Notify
	 * event for.
	 * It might be a window we just unmapped on *another* workspace when
	 * changing workspaces, for instance, or it might be a window with
	 * override redirect set.
	 * This is not an error.
	 * XXX We might need to look in the global window list, after all.
	 * Consider if a window is unmapped on our last workspace while
	 * changing workspaces.
	 * If we do this, we need to keep track of our own windows and
	 * ignore Unmap_Notify on them.
	 */
	client = find_client(& e->window);
	// TODO Make this compatiable with my model
	/* if (NULL == client || client->ws != current_workspace) */
	if (NULL == client || client->ws != current_workspace || client->monitor != focused_monitor)
		return;
	if (focus_window != NULL && client->id == focus_window->id)
		focus_window = NULL;
	if (client->iconic == false)
		forget_client(client);

	update_client_list();
}

void
config_notify(xcb_generic_event_t *ev)
{
	xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *)ev;

	if (e->window == screen->root) {
		/*
		 * When using RANDR or Xinerama, the root can change geometry
		 * when the user adds a new screen, tilts their screen 90
		 * degrees or whatnot. We might need to rearrange windows to
		 * be visible.
		 * We might get notified for several reasons, not just if the
		 * geometry changed.
		 * If the geometry is unchanged we do nothing.
		 */

		if (e->width != screen->width_in_pixels ||
		    e->height != screen->height_in_pixels) {
			screen->width_in_pixels = e->width;
			screen->height_in_pixels = e->height;

			if (-1 == randr_base)
				arrange_windows();
		}
	}
}

/* Set position, geometry and attributes of a new window and show it on
 * the screen.*/
void
map_request(xcb_generic_event_t *ev)
{
	xcb_map_request_event_t *e = (xcb_map_request_event_t *) ev;
	struct client *client;
	long data[] = {
		XCB_ICCCM_WM_STATE_NORMAL,
		XCB_NONE
	};


	/* The window is trying to map itself on the current workspace,
	 * but since it's unmapped it probably belongs on another workspace.*/
	if (NULL != find_client(&e->window))
		return;

	client = setup_window(e->window);

	if (NULL == client)
		return;

	/* Add this window to the current workspace. */
	add_to_workspace(client, current_workspace);

	/* If we don't have specific coord map it where the pointer is.*/
	if (!client->set_by_user) {
		if (!get_pointer(&screen->root, &client->x, &client->y))
			client->x = client->y = 0;

		client->x -= client->width / 2;
		client->y -= client->height / 2;
		move_window(client->id, client->x, client->y);
	}

	/* Find the physical output this window will be on if RANDR is active */
	if (-1 != randr_base) {
		/* client->monitor = find_monitor_by_coordinate(client->x, client->y); */
		client->monitor = focused_monitor;

		if (NULL == client->monitor && NULL != monitor_list)
			/* Window coordinates are outside all physical monitors.
			 * Choose the first screen.*/
			client->monitor = monitor_list->data;
	}

	fit_on_screen(client);

	/* Show window on screen. */
	xcb_map_window(conn, client->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);

	center_pointer(e->window, client);
	update_client_list();

	if (!client->maxed)
		set_borders(client, true);
	// always focus new window
	set_focus(client);
}


void
circulate_request(xcb_generic_event_t *ev)
{
	xcb_circulate_request_event_t *e = (xcb_circulate_request_event_t *)ev;

	/*
	 * Subwindow e->window to parent e->event is about to be restacked.
	 * Just do what was requested, e->place is either
	 * XCB_PLACE_ON_TOP or _ON_BOTTOM.
	 */
	xcb_circulate_window(conn, e->window, e->place);
}

void
handle_keypress(xcb_generic_event_t *e)
{
	xcb_key_press_event_t *ev       = (xcb_key_press_event_t *)e;
	xcb_keysym_t           keysym   = xcb_get_keysym(ev->detail);

	for (unsigned int i = 0; i < LENGTH(keys); ++i) {
		if (keysym == keys[i].keysym && CLEAN_MASK(keys[i].mod)
		    == CLEAN_MASK(ev->state) && keys[i].func) {
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}


/* Helper function to configure a window. */
void
configure_window(xcb_window_t win, uint16_t mask, const struct winconf *wc)
{
	uint32_t values[7];
	int8_t i = -1;

	if (mask & XCB_CONFIG_WINDOW_X) {
		mask |= XCB_CONFIG_WINDOW_X;
		++i;
		values[i] = wc->x;
	}

	if (mask & XCB_CONFIG_WINDOW_Y) {
		mask |= XCB_CONFIG_WINDOW_Y;
		++i;
		values[i] = wc->y;
	}

	if (mask & XCB_CONFIG_WINDOW_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		++i;
		values[i] = wc->width;
	}

	if (mask & XCB_CONFIG_WINDOW_HEIGHT) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		++i;
		values[i] = wc->height;
	}

	if (mask & XCB_CONFIG_WINDOW_SIBLING) {
		mask |= XCB_CONFIG_WINDOW_SIBLING;
		++i;
		values[i] = wc->sibling;
	}

	if (mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		++i;
		values[i] = wc->stack_mode;
	}

	if (i == -1)
		return;

	xcb_configure_window(conn, win, mask, values);
	xcb_flush(conn);
}

void
configure_request(xcb_generic_event_t *ev)
{
	xcb_configure_request_event_t *e = (xcb_configure_request_event_t *)ev;
	struct client *client;
	struct winconf wc;
	int16_t monitor_x, monitor_y;
	uint16_t monitor_width, monitor_height;
	uint32_t values[1];

	if ((client = find_client(&e->window))) { /* Find the client. */
		get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, client);

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			if (!client->maxed && !client->horizontal_maxed)
				client->width = e->width;

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			if (!client->maxed && !client->vertical_maxed)
				client->height = e->height;


		if (e->value_mask & XCB_CONFIG_WINDOW_X)
			if (!client->maxed && !client->horizontal_maxed)
				client->x = e->x;

		if (e->value_mask & XCB_CONFIG_WINDOW_Y)
			if (!client->maxed && !client->vertical_maxed)
				client->y = e->y;

		/* XXX Do we really need to pass on sibling and stack mode
		 * configuration? Do we want to? */
		if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
			values[0] = e->sibling;
			xcb_configure_window(conn, e->window,
					     XCB_CONFIG_WINDOW_SIBLING,values);
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
			values[0] = e->stack_mode;
			xcb_configure_window(conn, e->window,
					     XCB_CONFIG_WINDOW_STACK_MODE,values);
		}

		/* Check if window fits on screen after resizing. */
		if (!client->maxed) {
			resize_limit(client);
			move_limit(client);
			fit_on_screen(client);
		}

		set_borders(client, true);
	} else {
		/* Unmapped window, pass all options except border width. */
		wc.x = e->x;
		wc.y = e->y;
		wc.width = e->width;
		wc.height = e->height;
		wc.sibling = e->sibling;
		wc.stack_mode = e->stack_mode;

		configure_window(e->window, e->value_mask, &wc);
	}
}



void
mouse_motion(const Arg *arg)
{
	int16_t mx, my, winx, winy, winw, winh;
	xcb_query_pointer_reply_t *pointer;
	xcb_grab_pointer_reply_t  *grab_reply;
	xcb_motion_notify_event_t *ev = NULL;
	xcb_generic_event_t       *e  = NULL;
	bool ungrab;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, screen->root), 0);

	if (!pointer || focus_window->maxed) {
		free(pointer);
		return;
	}

	mx   = pointer->root_x;
	my   = pointer->root_y;
	winx = focus_window->x;
	winy = focus_window->y;
	winw = focus_window->width;
	winh = focus_window->height;

	xcb_cursor_t cursor;
	struct client example;
	raise_current_window();

	if (arg->i == SANEWM_MOVE)
		cursor = create_font_cursor (conn, 52); /* fleur */
	else {
		cursor  = create_font_cursor (conn, 120); /* sizing */
		example = create_back_window();
		xcb_map_window(conn,example.id);
	}

	grab_reply = xcb_grab_pointer_reply(conn, xcb_grab_pointer(conn, 0,
								   screen->root, BUTTON_MASK | XCB_EVENT_MASK_BUTTON_MOTION
								   | XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC,
								   XCB_GRAB_MODE_ASYNC, XCB_NONE, cursor, XCB_CURRENT_TIME)
					    , NULL);

	if (grab_reply->status != XCB_GRAB_STATUS_SUCCESS) {
		free(grab_reply);

		if (arg->i == SANEWM_RESIZE)
			xcb_unmap_window(conn,example.id);
		return;
	}

	free(grab_reply);
	ungrab = false;

	do {
		if (NULL != e)
			free(e);

		while (!(e = xcb_wait_for_event(conn)))
			xcb_flush(conn);

		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_REQUEST:
		case XCB_MAP_REQUEST:
			events[e->response_type & ~0x80](e);
			break;
		case XCB_MOTION_NOTIFY:
			ev = (xcb_motion_notify_event_t*)e;
			if (arg->i == SANEWM_MOVE)
				mouse_move(winx + ev->root_x - mx,
					   winy + ev->root_y-my);
			else
				mouse_resize(&example, winw + ev->root_x - mx,
					     winh + ev->root_y - my);

			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			if (arg->i == SANEWM_RESIZE) {
				ev = (xcb_motion_notify_event_t*)e;

				mouse_resize(focus_window, winw + ev->root_x - mx,
					     winh + ev->root_y - my);

				set_borders(focus_window, true);
			}

			ungrab = true;
			break;
		}
	} while (!ungrab && focus_window != NULL);

	free(pointer);
	free(e);
	xcb_free_cursor(conn,cursor);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);

	if (arg->i == SANEWM_RESIZE)
		xcb_unmap_window(conn,example.id);

	xcb_flush(conn);
}

void
button_press(xcb_generic_event_t *ev)
{
	xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;
	struct client *client;
	unsigned int i;


	if (!is_sloppy &&
	    e->detail == XCB_BUTTON_INDEX_1 &&
	    CLEAN_MASK(e->state) == 0) {
		// skip if already focused
		if (NULL != focus_window && e->event == focus_window->id)
			return;
		client = find_client(&e->event);
		if (NULL != client) {
			set_focus(client);
			raise_window(client->id);
			set_borders(client, true);
		}
		return;
	}

	for (i = 0; i < LENGTH(buttons); ++i)
		if (buttons[i].func &&
		    buttons[i].button == e->detail &&
		    CLEAN_MASK(buttons[i].mask)
		    == CLEAN_MASK(e->state)){
			if ((focus_window == NULL) &&
			    buttons[i].func == mouse_motion)
				return;
			if (buttons[i].root_only) {
				if (e->event == e->root && e->child == 0)
					buttons[i].func(&(buttons[i].arg));
			}
			else {
				buttons[i].func(&(buttons[i].arg));
			}
		}
}

void
client_message(xcb_generic_event_t *ev)
{
	xcb_client_message_event_t *e = (xcb_client_message_event_t *)ev;
	struct client *cl;

	if ((e->type == ATOM[wm_change_state] &&
	     e->format == 32 &&
	     e->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC) ||
	    e->type == ewmh->_NET_ACTIVE_WINDOW) {
		cl = find_client(&e->window);

		if (NULL == cl)
			return;

		if (false == cl->iconic) {
			if (e->type == ewmh->_NET_ACTIVE_WINDOW) {
				set_focus(cl);
				raise_window(cl->id);
			} else {
				hide();
			}

			return;
		}

		cl->iconic = false;
		xcb_map_window(conn, cl->id);
		set_focus(cl);
	}
	else if (e->type == ewmh->_NET_CURRENT_DESKTOP)
		change_workspace_helper(e->data.data32[0]);
	else if (e->type == ewmh->_NET_WM_STATE && e->format == 32) {
		cl = find_client(&e->window);
		if (NULL == cl)
			return;
		if (e->data.data32[1] == ewmh->_NET_WM_STATE_FULLSCREEN ||
		    e->data.data32[2] == ewmh->_NET_WM_STATE_FULLSCREEN) {
			switch (e->data.data32[0]) {
			case XCB_EWMH_WM_STATE_REMOVE:
				unmaximize_window(cl);
				break;
			case XCB_EWMH_WM_STATE_ADD:
				maximize_window(cl, false);
				break;
			case XCB_EWMH_WM_STATE_TOGGLE:
				if (cl->maxed)
					unmaximize_window(cl);
				else
					maximize_window(cl, false);
				break;

			default:
				break;
			}
		}
	} else if (e->type == ewmh->_NET_WM_DESKTOP && e->format == 32) {
		cl = find_client(&e->window);
		if (NULL == cl)
			return;
		/*
		 * e->data.data32[1] Source indication
		 * 0: backward compat
		 * 1: normal
		 * 2: pager/bars
		 *
		 * e->data.data32[0] new workspace
		 */
		delete_from_workspace(cl);
		add_to_workspace(cl, e->data.data32[0]);
		xcb_unmap_window(conn, cl->id);
		xcb_flush(conn);
	}
}
