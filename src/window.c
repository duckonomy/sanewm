#include <string.h>

#include "window.h"
#include "config.h"
#include "types.h"
#include "list.h"
#include "events.h"
#include "desktop.h"

void
fix()
{
	fix_window(focus_window);
}

void
unkillable()
{
	unkillable_window(focus_window);
}

void
raise_current_window(void)
{
	raise_window(focus_window->id);
}

void
add_to_client_list(const xcb_drawable_t id)
{
	xcb_change_property(conn, XCB_PROP_MODE_APPEND , screen->root,
			    ewmh->_NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, 1, &id);
	xcb_change_property(conn, XCB_PROP_MODE_APPEND , screen->root,
			    ewmh->_NET_CLIENT_LIST_STACKING, XCB_ATOM_WINDOW, 32, 1,&id);
}

void
focus_next_helper(bool arg)
{
	struct client *cl = NULL;
	struct item *head = workspace_list[current_workspace];
	struct item *tail, *item = NULL;
	// no windows on current workspace
	if (NULL == head)
		return;
	// if no focus on current workspace, find first valid item on list.
	if (NULL == focus_window || focus_window->ws != current_workspace) {
		for (item = head; item != NULL; item = item->next) {
			cl = item->data;
			if (!cl->iconic)
				break;
		}
	} else {
		// find tail of list and make list circular.
		for (tail = head = item = workspace_list[current_workspace];
		     item != NULL;
		     tail = item, item = item->next);
		head->prev = tail;
		tail->next = head;
		if (arg == SANEWM_FOCUS_NEXT) {
			// start from focus next and find first valid item on circular list.
			head = item = focus_window->workspace_item->next;
			do {
				cl = item->data;
				if (!cl->iconic)
					break;
				item = item->next;
			} while (item != head);
		} else {
			// start from focus previous and find first valid on circular list.
			tail = item = focus_window->workspace_item->prev;
			do {
				cl = item->data;
				if (!cl->iconic)
					break;
				item = item->prev;
			} while (item != tail);
		}
		// restore list.
		workspace_list[current_workspace]->prev->next = NULL;
		workspace_list[current_workspace]->prev = NULL;
	}
	if (!item || !(cl = item->data) || cl->iconic)
		return;
	raise_window(cl->id);
	center_pointer(cl->id, cl);
	set_focus(cl);
}


void
focus_next(const Arg *arg)
{
	focus_next_helper(arg->i > 0);
}

/* Rearrange windows to fit new screen size. */
void
arrange_windows(void)
{
	struct client *client;
	struct item *item;
	/* Go through all windows and resize them appropriately to
	 * fit the screen. */

	for (item = window_list; item != NULL; item = item->next) {
		client = item->data;
		fit_on_screen(client);
	}
}


/* check if the window is unkillable, if yes return true */
bool
get_unkill_state(xcb_drawable_t win)
{
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t *reply;
	uint8_t wsp;

	cookie = xcb_get_property(conn, false, win, ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
				  XCB_GET_PROPERTY_TYPE_ANY, 0,sizeof(uint8_t));

	reply  = xcb_get_property_reply(conn, cookie, NULL);

	if (reply == NULL || xcb_get_property_value_length(reply) == 0) {
		if (reply != NULL)
			free(reply);
		return false;
	}

	wsp = *(uint8_t *) xcb_get_property_value(reply);

	if (reply != NULL)
		free(reply);

	if (wsp == 1)
		return true;
	else
		return false;
}

void
always_on_top()
{
	struct client *cl = NULL;

	if (focus_window == NULL)
		return;

	if (top_win != focus_window->id) {
		if (0 != top_win) cl = find_client(&top_win);

		top_win = focus_window->id;
		if (NULL != cl)
			set_borders(cl, false);

		raise_window(top_win);
	} else
		top_win = 0;

	set_borders(focus_window, true);
}

/* Fix or unfix a window client from all workspaces. If setcolour is */
void
fix_window(struct client *client)
{
	uint32_t ws, ww;

	if (NULL == client)
		return;

	if (client->fixed) {
		client->fixed = false;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1,
				    &current_workspace);
	} else {
		/* Raise the window, if going to another desktop don't
		 * let the fixed window behind. */
		raise_window(client->id);
		client->fixed = true;
		ww = NET_WM_FIXED;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1,
				    &ww);
	}

	set_borders(client, true);
}

/* Make unkillable or killable a window client. If setcolour is */
void
unkillable_window(struct client *client)
{
	if (NULL == client)
		return;

	if (client->unkillable) {
		client->unkillable = false;
		xcb_delete_property(conn, client->id, ewmh->_NET_WM_STATE_DEMANDS_ATTENTION);
	} else {
		raise_window(client->id);
		client->unkillable = true;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_STATE_DEMANDS_ATTENTION, XCB_ATOM_CARDINAL, 8, 1,
				    &client->unkillable);
	}

	set_borders(client, true);
}

void
check_name(struct client *client)
{
	xcb_get_property_reply_t *reply ;
	unsigned int reply_len;
	char *wm_name_window;
	unsigned int i;
	uint32_t values[1] = { 0 };

	if (NULL == client)
		return;

	reply = xcb_get_property_reply(conn, xcb_get_property(conn, false,
							      client->id, get_atom(LOOK_INTO),
							      XCB_GET_PROPERTY_TYPE_ANY, 0, 60), NULL);

	if (reply == NULL || xcb_get_property_value_length(reply) == 0) {
		if (NULL != reply)
			free(reply);
		return;
	}

	reply_len = xcb_get_property_value_length(reply);
	wm_name_window = malloc(sizeof(char*) * (reply_len + 1));
	memcpy(wm_name_window, xcb_get_property_value(reply), reply_len);
	wm_name_window[reply_len] = '\0';

	if (NULL != reply)
		free(reply);

	for (i = 0; i < sizeof(ignore_names) / sizeof(__typeof__(*ignore_names)); ++i)
		if (strstr(wm_name_window, ignore_names[i]) != NULL) {
			client->ignore_borders = true;
			xcb_configure_window(conn, client->id,
					     XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
			break;
		}

	free(wm_name_window);
}

/* Set border colour, width and event mask for window. */
struct client *
setup_window(xcb_window_t win)
{
	unsigned int i;
	uint8_t result;
	uint32_t values[2], ws;
	xcb_atom_t a;
	xcb_size_hints_t hints;
	xcb_ewmh_get_atoms_reply_t win_type;
	xcb_window_t prop;
	struct item *item;
	struct client *client;
	xcb_get_property_cookie_t cookie;


	if (xcb_ewmh_get_wm_window_type_reply(ewmh,
					      xcb_ewmh_get_wm_window_type(ewmh, win), &win_type, NULL) == 1) {
		for (i = 0; i < win_type.atoms_len; ++i) {
			a = win_type.atoms[i];
			if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR ||
			    a == ewmh->_NET_WM_WINDOW_TYPE_DOCK ||
			    a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
				xcb_map_window(conn, win);
				return NULL;
			}
		}
		xcb_ewmh_get_atoms_reply_wipe(&win_type);
	}
	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXEL,
				     &conf.empty_color);
	xcb_change_window_attributes_checked(conn, win, XCB_CW_EVENT_MASK,
					     values);

	/* Add this window to the X Save Set. */
	xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);

	/* Remember window and store a few things about it. */
	item = add_item(&window_list);

	if (NULL == item)
		return NULL;

	client = malloc(sizeof(struct client));

	if (NULL == client)
		return NULL;

	item->data = client;
	client->id = win;
	client->x = client->y = client->width = client->height
		= client->min_width = client->min_height = client->base_width
		= client->base_height = 0;

	client->max_width     = screen->width_in_pixels;
	client->max_height    = screen->height_in_pixels;
	client->width_increment     = client->height_increment = 1;
	client->set_by_user     = client->vertical_maxed = client->horizontal_maxed
		= client->maxed = client->unkillable = client->fixed
		= client->ignore_borders = client->iconic = false;

	client->monitor       = NULL;
	client->window_item       = item;

	/* Initialize workspace pointers. */
	client->workspace_item = NULL;
	client->ws = -1;

	/* Get window geometry. */
	get_geometry(&client->id, &client->x, &client->y, &client->width,
		     &client->height, &client->depth);

	/* Get the window's incremental size step, if any.*/
	xcb_icccm_get_wm_normal_hints_reply(conn,
					    xcb_icccm_get_wm_normal_hints_unchecked(conn, win),
					    &hints, NULL);

	/* The user specified the position coordinates.
	 * Remember that so we can use geometry later. */
	if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
		client->set_by_user = true;

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		client->min_width  = hints.min_width;
		client->min_height = hints.min_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
		client->max_width  = hints.max_width;
		client->max_height = hints.max_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC) {
		client->width_increment  = hints.width_inc;
		client->height_increment = hints.height_inc;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
		client->base_width  = hints.base_width;
		client->base_height = hints.base_height;
	}
	cookie = xcb_icccm_get_wm_transient_for_unchecked(conn, win);
	if (cookie.sequence > 0) {
		result = xcb_icccm_get_wm_transient_for_reply(conn, cookie, &prop, NULL);
		if (result) {
			struct client *parent = find_client(&prop);
			if (parent) {
				client->set_by_user = true;
				client->x = parent->x + (parent->width / 2.0) - (client->width / 2.0);
				client->y = parent->y + (parent->height / 2.0) - (client->height / 2.0);
			}
		}
	}

	check_name(client);
	return client;
}


/* Raise window win to top of stack. */
void
raise_window(xcb_drawable_t win)
{
	uint32_t values[] = { XCB_STACK_MODE_ABOVE };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
	xcb_flush(conn);
}


/* Set window client to either top or bottom of stack depending on
 * where it is now. */
void
raise_or_lower(void)
{
	uint32_t values[] = { XCB_STACK_MODE_OPPOSITE };

	if (NULL == focus_window)
		return;

	xcb_configure_window(conn, focus_window->id,XCB_CONFIG_WINDOW_STACK_MODE,
			     values);

	xcb_flush(conn);
}


/* Keep the window inside the screen */
void
move_limit(struct client *client)
{
	int16_t monitor_y, monitor_x, temp = 0;
	uint16_t monitor_height, monitor_width;

	get_monitor_size(1, &monitor_x, &monitor_y,
			 &monitor_width, &monitor_height, client);

	no_border(&temp, client, true);

	/* Is it outside the physical monitor or close to the side? */
	if (client->y-conf.border_width < monitor_y ||
	    client->y < borders[2] + monitor_y)
		client->y = monitor_y;
	else if (client->y + client->height + (conf.border_width * 2) > monitor_y
		 + monitor_height - borders[2])
		client->y = monitor_y + monitor_height - client->height
			- conf.border_width * 2;

	if (client->x < borders[2] + monitor_x)
		client->x = monitor_x;
	else if (client->x + client->width + (conf.border_width * 2)
		 > monitor_x + monitor_width - borders[2])
		client->x = monitor_x + monitor_width - client->width
			- conf.border_width * 2;

	move_window(client->id, client->x, client->y);
	no_border(&temp, client, false);
}

void
move_window(xcb_drawable_t win, const int16_t x, const int16_t y)
{                                    // Move window win to root coordinates x,y.
	uint32_t values[2] = {x, y};

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y, values);

	xcb_flush(conn);
}

/* Mark window win as unfocused. */
void set_unfocus(void)
{
	//    xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_NONE,XCB_CURRENT_TIME);
	if (NULL == focus_window || focus_window->id == screen->root)
		return;

	set_borders(focus_window, false);
}

/* Find client with client->id win in global window list or NULL. */
struct client *
find_client(const xcb_drawable_t *win)
{
	struct client *client;
	struct item *item;

	for (item = window_list; item != NULL; item = item->next) {
		client = item->data;

		if (*win == client->id){
			return client;
		}
	}

	return NULL;
}

void
set_focus(struct client *client)// Set focus on window client.
{
	long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };

	/* If client is NULL, we focus on whatever the pointer is on.
	 * This is a pathological case, but it will make the poor user able
	 * to focus on windows anyway, even though this windowmanager might
	 * be buggy. */
	if (NULL == client) {
		focus_window = NULL;
		xcb_set_input_focus(conn, XCB_NONE,
				    XCB_INPUT_FOCUS_POINTER_ROOT, XCB_CURRENT_TIME);
		xcb_window_t not_win = 0;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
				    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1,
				    &not_win);

		xcb_flush(conn);
		return;
	}

	/* Don't bother focusing on the root window or on the same window
	 * that already has focus. */
	if (client->id == screen->root)
		return;

	if (NULL != focus_window)
		set_unfocus(); /* Unset last focus. */

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, client->id,
			    XCB_CURRENT_TIME); /* Set new input focus. */
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
			    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &client->id);

	/* Remember the new window as the current focused window. */
	focus_window = client;

	grab_buttons(client);
	set_borders(client, true);
}


/* Resize with limit. */
void
resize_limit(struct client *client)
{
	int16_t monitor_x, monitor_y,temp=0;
	uint16_t monitor_width, monitor_height;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, client);
	no_border(&temp, client, true);

	/* Is it smaller than it wants to  be? */
	if (0 != client->min_height && client->height < client->min_height)
		client->height = client->min_height;
	if (0 != client->min_width && client->width < client->min_width)
		client->width = client->min_width;
	if (client->x + client->width + conf.border_width > monitor_x + monitor_width)
		client->width = monitor_width - ((client->x - monitor_x)
						 + conf.border_width * 2);
	if (client->y + client->height + conf.border_width > monitor_y + monitor_height)
		client->height = monitor_height - ((client->y - monitor_y)
						   + conf.border_width * 2);

	resize(client->id, client->width, client->height);
	no_border(&temp, client, false);
}


void
move_resize(xcb_drawable_t win, const uint16_t x, const uint16_t y,
	    const uint16_t width, const uint16_t height)
{
	uint32_t values[4] = { x, y, width, height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Resize window win to width,height. */
void
resize(xcb_drawable_t win, const uint16_t width, const uint16_t height)
{
	uint32_t values[2] = { width , height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Resize window client in direction. */
void
resize_step(const Arg *arg)
{
	uint8_t stepx,stepy,cases = arg->i % 4;

	if (NULL == focus_window || focus_window->maxed)
		return;

	arg->i < 4 ? (stepx = stepy = movements[1])
		: (stepx = stepy = movements[0]);

	if (focus_window->width_increment > 7 && focus_window->height_increment > 7) {
		/* we were given a resize hint by the window so use it */
		stepx = focus_window->width_increment;
		stepy = focus_window->height_increment;
	}

	if (cases == SANEWM_RESIZE_LEFT)
		focus_window->width = focus_window->width - stepx;
	else if (cases == SANEWM_RESIZE_DOWN)
		focus_window->height = focus_window->height + stepy;
	else if (cases == SANEWM_RESIZE_UP)
		focus_window->height = focus_window->height - stepy;
	else if (cases == SANEWM_RESIZE_RIGHT)
		focus_window->width = focus_window->width + stepx;

	if (focus_window->vertical_maxed)
		focus_window->vertical_maxed = false;
	if (focus_window->horizontal_maxed)
		focus_window->horizontal_maxed  = false;

	resize_limit(focus_window);
	center_pointer(focus_window->id, focus_window);
	raise_current_window();
	set_borders(focus_window, true);
}

/* Resize window and keep it's aspect ratio (exponentially grow),
 * and round the result (+0.5) */
void
resize_step_aspect(const Arg *arg)
{
	if (NULL == focus_window || focus_window->maxed)
		return;

	if (arg->i == SANEWM_RESIZE_KEEP_ASPECT_SHRINK) {
		focus_window->width = (focus_window->width / resize_keep_aspect_ratio)
			+ 0.5;
		focus_window->height = (focus_window->height / resize_keep_aspect_ratio)
			+ 0.5;
	} else {
		focus_window->height = (focus_window->height * resize_keep_aspect_ratio)
			+ 0.5;
		focus_window->width = (focus_window->width * resize_keep_aspect_ratio)
			+ 0.5;
	}

	if (focus_window->vertical_maxed)
		focus_window->vertical_maxed = false;
	if (focus_window->horizontal_maxed)
		focus_window->horizontal_maxed  = false;

	resize_limit(focus_window);
	center_pointer(focus_window->id, focus_window);
	raise_current_window();
	set_borders(focus_window, true);
}

/* Try to snap to other windows and monitor border */
void
snap_window(struct client *client)
{
	struct item *item;
	struct client *win;
	int16_t monitor_x, monitor_y;
	uint16_t monitor_width, monitor_height;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, focus_window);

	for (item = workspace_list[current_workspace]; item != NULL; item = item->next) {
		win = item->data;

		if (client != win) {
			if (abs((win->x +win->width) - client->x
				+ conf.border_width) < borders[2])
				if (client->y + client->height > win->y &&
				    client->y < win->y +
				    win->height)
					client->x = (win->x + win->width) +
						(2 * conf.border_width);

			if (abs((win->y + win->height) -
				client->y + conf.border_width) < borders[2])
				if (client->x + client->width >win->x &&
				    client->x < win->x + win->width)
					client->y = (win->y + win->height) +
						(2 * conf.border_width);

			if (abs((client->x + client->width) - win->x +
				conf.border_width) < borders[2])
				if (client->y + client->height > win->y &&
				    client->y < win->y + win->height)
					client->x = (win->x - client->width)
						- (2 * conf.border_width);

			if (abs((client->y + client->height) -
				win->y +
				conf.border_width)
			    < borders[2])
				if (client->x +
				    client->width >win->x &&
				    client->x < win->x +
				    win->width)
					client->y = (win->y -
						     client->height) -
						(2 * conf.border_width);
		}
	}
}


void
move_step(const Arg *arg)
{
	int16_t start_x, start_y;
	uint8_t step, cases=arg->i;

	if (NULL == focus_window || focus_window->maxed)
		return;

	/* Save pointer position so we can warp pointer here later. */
	if (!get_pointer(&focus_window->id, &start_x, &start_y))
		return;

	cases = cases % 4;
	arg->i < 4 ? (step = movements[1]) : (step = movements[0]);

	if (cases == SANEWM_MOVE_LEFT)
		focus_window->x = focus_window->x - step;
	else if (cases == SANEWM_MOVE_DOWN)
		focus_window->y = focus_window->y + step;
	else if (cases == SANEWM_MOVE_UP)
		focus_window->y = focus_window->y - step;
	else if (cases == SANEWM_MOVE_RIGHT)
		focus_window->x = focus_window->x + step;

	raise_current_window();
	move_limit(focus_window);
	move_pointer_back(start_x,start_y,focus_window);
	xcb_flush(conn);
}

void
set_borders(struct client *client, const bool isitfocused)
{
	uint32_t values[1];  /* this is the color maintainer */
	uint16_t half = 0;
	bool inv = conf.inverted_colors;

	if (client->maxed || client->ignore_borders)
		return;

	/* Set border width. */
	values[0] = conf.border_width;
	xcb_configure_window(conn, client->id,
			     XCB_CONFIG_WINDOW_BORDER_WIDTH, values);

	if (top_win != 0 &&client->id == top_win)
		inv = !inv;

	half = conf.outer_border;

	xcb_rectangle_t rect_inner[] = {
		{
			client->width,
			0,
			conf.border_width - half,client->height
			+ conf.border_width - half
		},
		{
			client->width + conf.border_width + half,
			0,
			conf.border_width - half,
			client->height + conf.border_width - half
		},
		{
			0,
			client->height,
			client->width + conf.border_width
			- half,conf.border_width - half
		},
		{
			0,
			client->height + conf.border_width + half,client->width
			+ conf.border_width - half,conf.border_width
			- half
		},
		{
			client->width + conf.border_width
			+ half,conf.border_width + client->height
			+ half,conf.border_width,
			conf.border_width
		}
	};

	xcb_rectangle_t rect_outer[] = {
		{
			client->width + conf.border_width - half,
			0,
			half,
			client->height + conf.border_width * 2
		},
		{
			client->width + conf.border_width,
			0,
			half,
			client->height + conf.border_width * 2
		},
		{
			0,
			client->height + conf.border_width - half,client->width
			+ conf.border_width * 2,
			half
		},
		{
			0,
			client->height + conf.border_width,
			client->width + conf.border_width * 2,
			half
		},
		{
			1, 1, 1, 1
		}
	};

	xcb_pixmap_t pmap = xcb_generate_id(conn);
	xcb_create_pixmap(conn, client->depth, pmap, client->id,
			  client->width + (conf.border_width * 2),
			  client->height + (conf.border_width * 2));

	xcb_gcontext_t gc = xcb_generate_id(conn);
	xcb_create_gc(conn, gc, pmap, 0, NULL);

	if (inv) {
		xcb_rectangle_t fake_rect[5];

		for (uint8_t i = 0; i < 5; ++i) {
			fake_rect[i]  = rect_outer[i];
			rect_outer[i] = rect_inner[i];
			rect_inner[i] = fake_rect[i];
		}
	}

	values[0]  = conf.outer_border_color;

	if (client->unkillable || client->fixed) {
		if (client->unkillable && client->fixed)
			values[0]  = conf.fixed_unkill_color;
		else
			if (client->fixed)
				values[0]  = conf.fixed_color;
			else
				values[0]  = conf.unkill_color;
	}

	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &values[0]);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, rect_outer);

	values[0]   = conf.focus_color;

	if (!isitfocused)
		values[0] = conf.unfocus_color;

	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &values[0]);
	xcb_poly_fill_rectangle(conn, pmap, gc, 5, rect_inner);
	values[0] = pmap;
	xcb_change_window_attributes(conn,client->id, XCB_CW_BORDER_PIXMAP,
				     &values[0]);

	/* free the memory we allocated for the pixmap */
	xcb_free_pixmap(conn,pmap);
	xcb_free_gc(conn,gc);
	xcb_flush(conn);
}

void
unmaximize_window_size(struct client *client)
{
	uint32_t values[5], mask = 0;

	if (NULL == client)
		return;

	client->x = client->original_geometry.x;
	client->y = client->original_geometry.y;
	client->width = client->original_geometry.width;
	client->height = client->original_geometry.height;

	/* Restore geometry. */
	values[0] = client->x;
	values[1] = client->y;
	values[2] = client->width;
	values[3] = client->height;

	client->maxed = client->horizontal_maxed = 0;
	move_resize(client->id, client->x, client->y,
		    client->width, client->height);

	center_pointer(client->id, client);
	set_borders(client, true);
}

void
maximize(const Arg *arg)
{
	maximize_window(focus_window, 1);
}

void
fullscreen(const Arg *arg)
{
	maximize_window(focus_window, 0);
}


void
unmaximize_window(struct client *client){
	unmaximize_window_size(client);
	client->maxed = false;
	set_borders(client, true);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, NULL);
}

void
maximize_window(struct client *client, uint8_t with_offsets){
	uint32_t values[4];
	int16_t monitor_x, monitor_y;
	uint16_t monitor_width, monitor_height;

	if (NULL == focus_window)
		return;

	/* Check if maximized already. If so, revert to stored geometry. */
	if (focus_window->maxed) {
		unmaximize_window(focus_window);
		return;
	}

	get_monitor_size(with_offsets, &monitor_x, &monitor_y, &monitor_width, &monitor_height, client);
	maximize_helper(client, monitor_x, monitor_y, monitor_width, monitor_height);
	raise_current_window();
	if (!with_offsets) {
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
				    ewmh->_NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &ewmh->_NET_WM_STATE_FULLSCREEN);
	}
	xcb_flush(conn);
}

void
maximize_vertical_horizontal(const Arg *arg)
{
	uint32_t values[2];
	int16_t monitor_y, monitor_x, temp = 0;
	uint16_t monitor_height, monitor_width;

	if (NULL == focus_window)
		return;

	if (focus_window->vertical_maxed || focus_window->horizontal_maxed) {
		unmaximize_window_size(focus_window);
		focus_window->vertical_maxed = focus_window->horizontal_maxed = false;
		fit_on_screen(focus_window);
		set_borders(focus_window, true);
		return;
	}

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, focus_window);
	save_original_geometry(focus_window);
	no_border(&temp, focus_window,true);

	if (arg->i == SANEWM_MAXIMIZE_VERTICALLY) {
		focus_window->y = monitor_y;
		/* Compute new height considering height increments
		 * and screen height. */
		focus_window->height = monitor_height - (conf.border_width * 2);

		/* Move to top of screen and resize. */
		values[0] = focus_window->y;
		values[1] = focus_window->height;

		xcb_configure_window(conn, focus_window->id, XCB_CONFIG_WINDOW_Y
				     | XCB_CONFIG_WINDOW_HEIGHT, values);

		focus_window->vertical_maxed = true;
	} else if (arg->i == SANEWM_MAXIMIZE_HORIZONTALLY) {
		focus_window->x = monitor_x;
		focus_window->width = monitor_width - (conf.border_width * 2);
		values[0] = focus_window->x;
		values[1] = focus_window->width;

		xcb_configure_window(conn, focus_window->id, XCB_CONFIG_WINDOW_X
				     | XCB_CONFIG_WINDOW_WIDTH, values);

		focus_window->horizontal_maxed = true;
	}

	no_border(&temp, focus_window,false);
	raise_current_window();
	center_pointer(focus_window->id, focus_window);
	set_borders(focus_window,true);
}

void
max_half(const Arg *arg)
{
	uint32_t values[4];
	int16_t monitor_x, monitor_y, temp=0;
	uint16_t monitor_width, monitor_height;

	if (NULL == focus_window || focus_window->maxed)
		return;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, focus_window);
	no_border(&temp, focus_window, true);

	if (arg->i>4) {
		if (arg->i>6) {
			/* in folding mode */
			if (arg->i == SANEWM_MAX_HALF_FOLD_VERTICAL)
				focus_window->height = focus_window->height / 2
					- (conf.border_width);
			else
				focus_window->height = focus_window->height * 2
					+ (2*conf.border_width);
		} else {
			focus_window->y      =  monitor_y;
			focus_window->height =  monitor_height - (conf.border_width * 2);
			focus_window->width  = ((float)(monitor_width) / 2)
				- (conf.border_width * 2);

			if (arg->i == SANEWM_MAX_HALF_VERTICAL_LEFT)
				focus_window->x = monitor_x;
			else
				focus_window->x = monitor_x + monitor_width
					- (focus_window->width
					   + conf.border_width * 2);
		}
	} else {
		if (arg->i < 2) {
			/* in folding mode */
			if (arg->i == SANEWM_MAX_HALF_FOLD_HORIZONTAL)
				focus_window->width = focus_window->width / 2
					- conf.border_width;
			else
				focus_window->width = focus_window->width * 2
					+ (2 * conf.border_width); //unfold
		} else {
			focus_window->x     =  monitor_x;
			focus_window->width =  monitor_width - (conf.border_width * 2);
			focus_window->height = ((float)(monitor_height) / 2)
				- (conf.border_width * 2);

			if (arg->i == SANEWM_MAX_HALF_HORIZONTAL_TOP)
				focus_window->y = monitor_y;
			else
				focus_window->y = monitor_y + monitor_height
					- (focus_window->height
					   + conf.border_width * 2);
		}
	}

	move_resize(focus_window->id, focus_window->x, focus_window->y,
		    focus_window->width, focus_window->height);

	focus_window->vertical_horizontal = true;
	no_border(&temp, focus_window, false);
	raise_current_window();
	fit_on_screen(focus_window);
	center_pointer(focus_window->id, focus_window);
	set_borders(focus_window, true);
}

void
max_half_half(const Arg *arg)
{
	uint32_t values[4];
	int16_t monitor_x, monitor_y, temp=0;
	uint16_t monitor_width, monitor_height;

	if (NULL == focus_window || focus_window->maxed)
		return;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, focus_window);
	no_border(&temp, focus_window, true);

	focus_window->x =  monitor_x;
	focus_window->y =  monitor_y;

	focus_window->width  = ((float)(monitor_width) / 2)
		- (conf.border_width * 2);
	focus_window->height = ((float)(monitor_height) / 2)
		- (conf.border_width * 2);

	if (arg->i == SANEWM_MAX_HALF_HALF_BOTTOM_LEFT)
		focus_window->y = monitor_y + monitor_height
			- (focus_window->height
			   + conf.border_width * 2);
	else if (arg->i == SANEWM_MAX_HALF_HALF_TOP_RIGHT)
		focus_window->x = monitor_x + monitor_width
			- (focus_window->width
			   + conf.border_width * 2);
	else if (arg->i == SANEWM_MAX_HALF_HALF_BOTTOM_RIGHT) {
		focus_window->x = monitor_x + monitor_width
			- (focus_window->width
			   + conf.border_width * 2);
		focus_window->y = monitor_y + monitor_height
			- (focus_window->height
			   + conf.border_width * 2);
	} else if (arg->i == SANEWM_MAX_HALF_HALF_CENTER) {
		focus_window->x  += monitor_width - (focus_window->width
						     + conf.border_width * 2) + monitor_x;
		focus_window->y  += monitor_height - (focus_window->height
						      + conf.border_width * 2) + monitor_y;
		focus_window->y  = focus_window->y / 2;
		focus_window->x  = focus_window->x / 2;
	}

	move_resize(focus_window->id, focus_window->x, focus_window->y,
		    focus_window->width, focus_window->height);

	focus_window->vertical_horizontal = true;
	no_border(&temp, focus_window, false);
	raise_current_window();
	fit_on_screen(focus_window);
	center_pointer(focus_window->id, focus_window);
	set_borders(focus_window, true);
}


void
hide(void)
{
	if (focus_window == NULL)
		return;

	long data[] = {
		XCB_ICCCM_WM_STATE_ICONIC,
		ewmh->_NET_WM_STATE_HIDDEN,
		XCB_NONE
	};

	/* Unmap window and declare iconic. Unmapping will generate an
	 * Unmap_Notify event so we can forget about the window later. */
	focus_window->iconic = true;

	xcb_unmap_window(conn, focus_window->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, focus_window->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 3, data);

	xcb_flush(conn);
}


bool
get_geometry(const xcb_drawable_t *win, int16_t *x, int16_t *y, uint16_t *width,
	     uint16_t *height, uint8_t *depth)
{
	xcb_get_geometry_reply_t *geom =
		xcb_get_geometry_reply(conn, xcb_get_geometry(conn, *win), NULL);

	if (NULL == geom)
		return false;

	*x = geom->x;
	*y = geom->y;
	*width = geom->width;
	*height = geom->height;
	*depth = geom->depth;

	free(geom);
	return true;
}

void
teleport(const Arg *arg)
{
	int16_t point_x, point_y, monitor_x, monitor_y, temp = 0;
	uint16_t monitor_width, monitor_height;

	if (NULL == focus_window ||
	    NULL == workspace_list[current_workspace] ||
	    focus_window->maxed)
		return;

	if (!get_pointer(&focus_window->id, &point_x, &point_y))
		return;
	uint16_t tmp_x = focus_window->x;
	uint16_t tmp_y = focus_window->y;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height,focus_window);
	no_border(&temp, focus_window, true);
	focus_window->x = monitor_x; focus_window->y = monitor_y;

	if (arg->i == SANEWM_TELEPORT_CENTER) { /* center */
		focus_window->x  += monitor_width - (focus_window->width
						     + conf.border_width * 2) + monitor_x;
		focus_window->y  += monitor_height - (focus_window->height
						      + conf.border_width * 2) + monitor_y;
		focus_window->y  = focus_window->y / 2;
		focus_window->x  = focus_window->x / 2;
	} else {
		/* top-left */
		if (arg->i > 3) {
			/* bottom-left */
			if (arg->i == SANEWM_TELEPORT_BOTTOM_LEFT)
				focus_window->y += monitor_height - (focus_window->height
								     + conf.border_width * 2);
			/* center y */
			else if (arg->i == SANEWM_TELEPORT_CENTER_Y) {
				focus_window->x  = tmp_x;
				focus_window->y += monitor_height - (focus_window->height
								     + conf.border_width * 2)+ monitor_y;
				focus_window->y  = focus_window->y / 2;
			}
		} else {
			/* top-right */
			if (arg->i < 2)
				/* center x */
				if (arg->i == SANEWM_TELEPORT_CENTER_X) {
					focus_window->y  = tmp_y;
					focus_window->x += monitor_width
						- (focus_window->width
						   + conf.border_width * 2)
						+ monitor_x;
					focus_window->x  = focus_window->x / 2;
				} else
					focus_window->x += monitor_width
						- (focus_window->width
						   + conf.border_width * 2);
			else {
				/* bottom-right */
				focus_window->x += monitor_width
					- (focus_window->width
					   + conf.border_width * 2);
				focus_window->y += monitor_height
					- (focus_window->height
					   + conf.border_width * 2);
			}
		}
	}

	move_window(focus_window->id, focus_window->x, focus_window->y);
	move_pointer_back(point_x,point_y, focus_window);
	no_border(&temp, focus_window, false);
	raise_current_window();
	xcb_flush(conn);
}

void
delete_window()
{
	bool use_delete = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	xcb_get_property_cookie_t cookie;

	if (NULL == focus_window || focus_window->unkillable == true)
		return;

	/* Check if WM_DELETE is supported.  */
	cookie = xcb_icccm_get_wm_protocols_unchecked(conn, focus_window->id,
						      ewmh->WM_PROTOCOLS);

	if (focus_window->id == top_win)
		top_win = 0;

	if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL) == 1) {
		for (uint32_t i = 0; i < protocols.atoms_len; ++i)
			if (protocols.atoms[i] == ATOM[wm_delete_window]) {
				xcb_client_message_event_t ev = {
					.response_type = XCB_CLIENT_MESSAGE,
					.format = 32,
					.sequence = 0,
					.window = focus_window->id,
					.type = ewmh->WM_PROTOCOLS,
					.data.data32 = {
						ATOM[wm_delete_window],
						XCB_CURRENT_TIME
					}
				};

				xcb_send_event(conn, false, focus_window->id,
					       XCB_EVENT_MASK_NO_EVENT,
					       (char *)&ev);

				use_delete = true;
				break;
			}

		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	if (!use_delete)
		xcb_kill_client(conn, focus_window->id);
}


/* Forget everything about client client. */
void
forget_client(struct client *client)
{
	uint32_t ws;

	if (NULL == client)
		return;
	if (client->id == top_win)
		top_win = 0;
	/* Delete client from the workspace list it belongs to. */
	delete_from_workspace(client);

	/* Remove from global window list. */
	free_item(&window_list, NULL, client->window_item);
}

/* Forget everything about a client with client->id win. */
void
forget_window(xcb_window_t win)
{
	struct client *client;
	struct item *item;

	for (item = window_list; item != NULL; item = item->next) {
		/* Find this window in the global window list. */
		client = item->data;
		if (win == client->id) {
			/* Forget it and free allocated data, it might
			 * already be freed by handling an Unmap_Notify. */
			forget_client(client);
			return;
		}
	}
}

void
no_border(int16_t *temp, struct client *client, bool set_unset)
{
	if (client->ignore_borders) {
		if (set_unset) {
			*temp = conf.border_width;
			conf.border_width = 0;
		} else
			conf.border_width = *temp;
	}
}

void
maximize_helper(struct client *client, uint16_t monitor_x, uint16_t monitor_y,
		uint16_t monitor_width, uint16_t monitor_height)
{
	uint32_t values[4];

	values[0] = 0;
	save_original_geometry(client);
	xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_BORDER_WIDTH,
			     values);

	client->x = monitor_x;
	client->y = monitor_y;
	client->width = monitor_width;
	client->height = monitor_height;

	move_resize(client->id, client->x, client->y, client->width,
		    client->height);
	client->maxed = true;
}

/* Fit client on physical screen, moving and resizing as necessary. */
void
fit_on_screen(struct client *client)
{
	int16_t monitor_x, monitor_y,temp=0;
	uint16_t monitor_width, monitor_height;
	bool willmove, willresize;

	willmove = willresize = client->vertical_maxed = client->horizontal_maxed = false;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, client);

	if (client->maxed) {
		client->maxed = false;
		set_borders(client, false);
	} else {
		/* not maxed but look as if it was maxed, then make it maxed */
		if (client->width == monitor_width && client->height == monitor_height)
			willresize = true;
		else {
			get_monitor_size(0, &monitor_x, &monitor_y, &monitor_width, &monitor_height,
					 client);
			if (client->width == monitor_width && client->height
			    == monitor_height)
				willresize = true;
		}
		if (willresize) {
			client->x = monitor_x;
			client->y = monitor_y;
			client->width -= conf.border_width * 2;
			client->height -= conf.border_width * 2;
			maximize_helper(client, monitor_x, monitor_y, monitor_width,
					monitor_height);
			return;
		} else {
			get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width,
					 &monitor_height,client);
		}
	}

	if (client->x > monitor_x + monitor_width ||
	    client->y > monitor_y + monitor_height ||
	    client->x < monitor_x ||
	    client->y < monitor_y) {
		willmove = true;
		/* Is it outside the physical monitor? */
		if (client->x > monitor_x + monitor_width)
			client->x = monitor_x + monitor_width
				- client->width - conf.border_width*2;
		if (client->y > monitor_y + monitor_height)
			client->y = monitor_y + monitor_height - client->height
				- conf.border_width * 2;
		if (client->x < monitor_x)
			client->x = monitor_x;
		if (client->y < monitor_y)
			client->y = monitor_y;
	}

	/* Is it smaller than it wants to  be? */
	if (0 != client->min_height && client->height < client->min_height) {
		client->height = client->min_height;

		willresize = true;
	}

	if (0 != client->min_width && client->width < client->min_width) {
		client->width = client->min_width;
		willresize = true;
	}

	no_border(&temp, client, true);
	/* If the window is larger than our screen, just place it in
	 * the corner and resize. */
	if (client->width + conf.border_width * 2 > monitor_width) {
		client->x = monitor_x;
		client->width = monitor_width - conf.border_width * 2;
		willmove = willresize = true;
	} else
		if (client->x + client->width + conf.border_width * 2
		    > monitor_x + monitor_width) {
			client->x = monitor_x + monitor_width - (client->width
								 + conf.border_width * 2);
			willmove = true;
		}
	if (client->height + conf.border_width * 2 > monitor_height) {
		client->y = monitor_y;
		client->height = monitor_height - conf.border_width * 2;
		willmove = willresize = true;
	} else
		if (client->y + client->height + conf.border_width * 2 > monitor_y
		    + monitor_height) {
			client->y = monitor_y + monitor_height - (client->height
								  + conf.border_width * 2);
			willmove = true;
		}

	if (willmove)
		move_window(client->id, client->x, client->y);

	if (willresize)
		resize(client->id, client->width, client->height);

	no_border(&temp, client, false);
}

void
save_original_geometry(struct client *client)
{
	client->original_geometry.x      = client->x;
	client->original_geometry.y      = client->y;
	client->original_geometry.width  = client->width;
	client->original_geometry.height = client->height;
}

void
update_client_list(void)
{
	uint32_t len, i;
	xcb_window_t *children;
	struct client *cl;

	/* can only be called after the first window has been spawn */
	xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn,
							     xcb_query_tree(conn, screen->root), 0);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST_STACKING);

	if (reply == NULL) {
		add_to_client_list(0);
		return;
	}

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	for (i = 0; i < len; ++i) {
		cl = find_client(&children[i]);
		if (cl != NULL)
			add_to_client_list(cl->id);
	}

	free(reply);
}

/* Walk through all existing windows and set them up. returns true on success */
bool
setup_screen(void)
{
	xcb_get_window_attributes_reply_t *attr;
	struct client *client;
	uint32_t ws;
	uint32_t len;
	xcb_window_t *children;
	uint32_t i;

	/* Get all children. */
	xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn,
							     xcb_query_tree(conn, screen->root), 0);

	if (NULL == reply)
		return false;

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	/* Set up all windows on this root. */
	for (i = 0; i < len; ++i) {
		attr = xcb_get_window_attributes_reply(conn,
						       xcb_get_window_attributes(conn, children[i]),
						       NULL);

		if (!attr)
			continue;

		/* Don't set up or even bother windows in override redirect mode.
		 * This mode means they wouldn't have been reported to us with a
		 * MapRequest if we had been running, so in the normal case we wouldn't
		 * have seen them. Only handle visible windows. */
		if (!attr->override_redirect &&
		    attr->map_state ==
		    XCB_MAP_STATE_VIEWABLE) {
			client = setup_window(children[i]);

			if (NULL != client) {
				/* Find the physical output this window will be on if
				 * RANDR is active. */
				if (-1 != randr_base)
					client->monitor = find_monitor_by_coordinate(client->x,
										     client->y);
				/* Fit window on physical screen. */
				fit_on_screen(client);
				set_borders(client, false);

				/* Check if this window has a workspace set already
				 * as a WM hint. */
				ws = get_wm_desktop(children[i]);

				if (get_unkill_state(children[i]))
					unkillable_window(client);

				if (ws == NET_WM_FIXED) {
					/* Add to current workspace. */
					add_to_workspace(client, current_workspace);
					/* Add to all other workspaces. */
					fix_window(client);
				} else {
					if (SANEWM_NOWS != ws && ws < WORKSPACES) {
						add_to_workspace(client, ws);
						if (ws != current_workspace)
							/* If it's not our current works
							 * pace, hide it. */
							xcb_unmap_window(conn,
									 client->id);
					} else {
						add_to_workspace(client, current_workspace);
						add_to_client_list(children[i]);
					}
				}
			}
		}

		if (NULL != attr)
			free(attr);

	}
	change_workspace_helper(0);

	xcb_randr_get_output_primary_reply_t *gpo =
		xcb_randr_get_output_primary_reply(conn, xcb_randr_get_output_primary(conn, screen->root), NULL);

	primary_output_monitor = gpo->output;
	focused_monitor = find_monitor(primary_output_monitor);

	if (NULL != reply)
		free(reply);

	if (NULL != gpo)
		free(gpo);

	return true;
}
struct client
create_back_window(void)
{
	struct client temp_win;
	uint32_t values[1] = { conf.focus_color };

	temp_win.id = xcb_generate_id(conn);
	xcb_create_window(conn,
			  /* depth */
			  XCB_COPY_FROM_PARENT,
			  /* window Id */
			  temp_win.id,
			  /* parent window */
			  screen->root,
			  /* x, y */
			  focus_window->x,
			  focus_window->y,
			  /* width, height */
			  focus_window->width,
			  focus_window->height,
			  /* border width */
			  borders[3],
			  /* class */
			  XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  /* visual */
			  screen->root_visual,
			  XCB_CW_BORDER_PIXEL,
			  values);

	if (conf.enable_compton) {
		values[0] = 1;
		xcb_change_window_attributes(conn, temp_win.id,
					     XCB_BACK_PIXMAP_PARENT_RELATIVE, values);
	} else {
		values[0] = conf.unfocus_color;
		xcb_change_window_attributes(conn, temp_win.id,
					     XCB_CW_BACK_PIXEL, values);
	}

	temp_win.x                    = focus_window->x;
	temp_win.y                    = focus_window->y;
	temp_win.width                = focus_window->width;
	temp_win.unkillable           = focus_window->unkillable;
	temp_win.fixed                = focus_window->fixed;
	temp_win.height               = focus_window->height;
	temp_win.width_increment      = focus_window->width_increment;
	temp_win.height_increment     = focus_window->height_increment;
	temp_win.base_width           = focus_window->base_width;
	temp_win.base_height          = focus_window->base_height;
	temp_win.monitor              = focus_window->monitor;
	temp_win.min_height           = focus_window->min_height;
	temp_win.min_width            = focus_window->min_height;
	temp_win.ignore_borders       = focus_window->ignore_borders;

	return temp_win;
}
