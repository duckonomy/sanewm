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
	fix_window(current_window);
}

void
unkillable()
{
	unkillable_window(current_window);
}

void
raise_current_window(void)
{
	raise_window(current_window->id);
}

void
add_to_window_list(const xcb_drawable_t id)
{
	xcb_change_property(conn, XCB_PROP_MODE_APPEND , screen->root,
			    ewmh->_NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, 1, &id);
	xcb_change_property(conn, XCB_PROP_MODE_APPEND , screen->root,
			    ewmh->_NET_CLIENT_LIST_STACKING, XCB_ATOM_WINDOW, 32, 1,&id);
}

void
focus_next_helper(bool arg)
{
	struct sane_window *window = NULL;
	struct list_item *head = workspace_list[current_workspace];
	struct list_item *tail, *item = NULL;
	// no windows on current workspace
	if (NULL == head)
		return;
	// if no focus on current workspace, find first valid item on list.
	if (NULL == current_window || current_window->ws != current_workspace) {
		for (item = head; item != NULL; item = item->next) {
			window = item->data;
			if (!window->iconic)
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
			head = item = current_window->workspace_item->next;
			do {
				window = item->data;
				if (!window->iconic)
					break;
				item = item->next;
			} while (item != head);
		} else {
			// start from focus previous and find first valid on circular list.
			tail = item = current_window->workspace_item->prev;
			do {
				window = item->data;
				if (!window->iconic)
					break;
				item = item->prev;
			} while (item != tail);
		}
		// restore list.
		workspace_list[current_workspace]->prev->next = NULL;
		workspace_list[current_workspace]->prev = NULL;
	}
	if (!item || !(window = item->data) || window->iconic)
		return;
	raise_window(window->id);
	center_pointer(window->id, window);
	focus_window(window);
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
	struct sane_window *window;
	struct list_item *item;
	/* Go through all windows and resize them appropriately to
	 * fit the screen. */

	for (item = window_list; item != NULL; item = item->next) {
		window = item->data;
		fit_on_screen(window);
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
	struct sane_window *window = NULL;

	if (current_window == NULL)
		return;

	if (top_win != current_window->id) {
		if (0 != top_win) window = find_window(&top_win);

		top_win = current_window->id;
		if (NULL != window)
			set_borders(window, false);

		raise_window(top_win);
	} else
		top_win = 0;

	set_borders(current_window, true);
}

/* Fix or unfix a window client from all workspaces. If setcolour is */
void
fix_window(struct sane_window *window)
{
	uint32_t ws, ww;

	if (NULL == window)
		return;

	if (window->fixed) {
		window->fixed = false;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1,
				    &current_workspace);
	} else {
		/* Raise the window, if going to another desktop don't
		 * let the fixed window behind. */
		raise_window(window->id);
		window->fixed = true;
		ww = NET_WM_FIXED;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
				    ewmh->_NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1,
				    &ww);
	}

	set_borders(window, true);
}

/* Make unkillable or killable a window client. If setcolour is */
void
unkillable_window(struct sane_window *window)
{
	if (NULL == window)
		return;

	if (window->unkillable) {
		window->unkillable = false;
		xcb_delete_property(conn, window->id, ewmh->_NET_WM_STATE_DEMANDS_ATTENTION);
	} else {
		raise_window(window->id);
		window->unkillable = true;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
				    ewmh->_NET_WM_STATE_DEMANDS_ATTENTION, XCB_ATOM_CARDINAL, 8, 1,
				    &window->unkillable);
	}

	set_borders(window, true);
}

void
check_name(struct sane_window *window)
{
	xcb_get_property_reply_t *reply ;
	unsigned int reply_len;
	char *wm_name_window;
	unsigned int i;
	uint32_t values[1] = { 0 };

	if (NULL == window)
		return;

	reply = xcb_get_property_reply(conn, xcb_get_property(conn, false,
							      window->id, get_atom(LOOK_INTO),
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
			window->ignore_borders = true;
			xcb_configure_window(conn, window->id,
					     XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
			break;
		}

	free(wm_name_window);
}

/* Set border colour, width and event mask for window. */
struct sane_window *
setup_window(xcb_window_t win)
{
	unsigned int i;
	uint8_t result;
	uint32_t values[2], ws;
	xcb_atom_t a;
	xcb_size_hints_t hints;
	xcb_ewmh_get_atoms_reply_t win_type;
	xcb_window_t prop;
	struct list_item *item;
	struct sane_window *window;
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

	window = malloc(sizeof(struct sane_window));

	if (NULL == window)
		return NULL;

	item->data = window;
	window->id = win;
	window->x = window->y = window->width = window->height
		= window->min_width = window->min_height = window->base_width
		= window->base_height = 0;

	window->max_width     = screen->width_in_pixels;
	window->max_height    = screen->height_in_pixels;
	window->width_increment     = window->height_increment = 1;
	window->set_by_user     = window->vertical_maxed = window->horizontal_maxed
		= window->maxed = window->unkillable = window->fixed
		= window->ignore_borders = window->iconic = false;

	window->monitor       = NULL;
	window->window_item       = item;

	/* Initialize workspace pointers. */
	window->workspace_item = NULL;
	window->ws = -1;

	/* Get window geometry. */
	get_geometry(&window->id, &window->x, &window->y, &window->width,
		     &window->height, &window->depth);

	/* Get the window's incremental size step, if any.*/
	xcb_icccm_get_wm_normal_hints_reply(conn,
					    xcb_icccm_get_wm_normal_hints_unchecked(conn, win),
					    &hints, NULL);

	/* The user specified the position coordinates.
	 * Remember that so we can use geometry later. */
	if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
		window->set_by_user = true;

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		window->min_width  = hints.min_width;
		window->min_height = hints.min_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
		window->max_width  = hints.max_width;
		window->max_height = hints.max_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC) {
		window->width_increment  = hints.width_inc;
		window->height_increment = hints.height_inc;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
		window->base_width  = hints.base_width;
		window->base_height = hints.base_height;
	}
	cookie = xcb_icccm_get_wm_transient_for_unchecked(conn, win);
	if (cookie.sequence > 0) {
		result = xcb_icccm_get_wm_transient_for_reply(conn, cookie, &prop, NULL);
		if (result) {
			struct sane_window *parent = find_window(&prop);
			if (parent) {
				window->set_by_user = true;
				window->x = parent->x + (parent->width / 2.0) - (window->width / 2.0);
				window->y = parent->y + (parent->height / 2.0) - (window->height / 2.0);
			}
		}
	}

	check_name(window);
	return window;
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

	if (NULL == current_window)
		return;

	xcb_configure_window(conn, current_window->id,XCB_CONFIG_WINDOW_STACK_MODE,
			     values);

	xcb_flush(conn);
}


/* Keep the window inside the screen */
void
move_limit(struct sane_window *window)
{
	int16_t monitor_y, monitor_x, temp = 0;
	uint16_t monitor_height, monitor_width;

	get_monitor_size(1, &monitor_x, &monitor_y,
			 &monitor_width, &monitor_height, window);

	no_border(&temp, window, true);

	/* Is it outside the physical monitor or close to the side? */
	if (window->y-conf.border_width < monitor_y ||
	    window->y < borders[2] + monitor_y)
		window->y = monitor_y;
	else if (window->y + window->height + (conf.border_width * 2) > monitor_y
		 + monitor_height - borders[2])
		window->y = monitor_y + monitor_height - window->height
			- conf.border_width * 2;

	if (window->x < borders[2] + monitor_x)
		window->x = monitor_x;
	else if (window->x + window->width + (conf.border_width * 2)
		 > monitor_x + monitor_width - borders[2])
		window->x = monitor_x + monitor_width - window->width
			- conf.border_width * 2;

	move_window(window->id, window->x, window->y);
	no_border(&temp, window, false);
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
	if (NULL == current_window || current_window->id == screen->root)
		return;

	set_borders(current_window, false);
}

/* Find window with window->id win in global window list or NULL. */
struct sane_window *
find_window(const xcb_drawable_t *win)
{
	struct sane_window *window;
	struct list_item *item;

	for (item = window_list; item != NULL; item = item->next) {
		window = item->data;

		if (*win == window->id){
			return window;
		}
	}

	return NULL;
}

void
focus_window(struct sane_window *window)
{
	long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };

	/* If window is NULL, we focus on whatever the pointer is on.
	 * This is a pathological case, but it will make the poor user able
	 * to focus on windows anyway, even though this windowmanager might
	 * be buggy. */
	if (NULL == window) {
		current_window = NULL;
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
	if (window->id == screen->root)
		return;

	if (NULL != current_window)
		set_unfocus(); /* Unset last focus. */

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window->id,
			    XCB_CURRENT_TIME); /* Set new input focus. */
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
			    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &window->id);

	/* Remember the new window as the current focused window. */
	current_window = window;

	grab_buttons(window);
	set_borders(window, true);
}


/* Resize with limit. */
void
resize_limit(struct sane_window *window)
{
	int16_t monitor_x, monitor_y,temp=0;
	uint16_t monitor_width, monitor_height;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, window);
	no_border(&temp, window, true);

	/* Is it smaller than it wants to  be? */
	if (0 != window->min_height && window->height < window->min_height)
		window->height = window->min_height;
	if (0 != window->min_width && window->width < window->min_width)
		window->width = window->min_width;
	if (window->x + window->width + conf.border_width > monitor_x + monitor_width)
		window->width = monitor_width - ((window->x - monitor_x)
						 + conf.border_width * 2);
	if (window->y + window->height + conf.border_width > monitor_y + monitor_height)
		window->height = monitor_height - ((window->y - monitor_y)
						   + conf.border_width * 2);

	resize(window->id, window->width, window->height);
	no_border(&temp, window, false);
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

/* Resize window window in direction. */
void
resize_step(const Arg *arg)
{
	uint8_t stepx,stepy,cases = arg->i % 4;

	if (NULL == current_window || current_window->maxed)
		return;

	arg->i < 4 ? (stepx = stepy = movements[1])
		: (stepx = stepy = movements[0]);

	if (current_window->width_increment > 7 && current_window->height_increment > 7) {
		/* we were given a resize hint by the window so use it */
		stepx = current_window->width_increment;
		stepy = current_window->height_increment;
	}

	if (cases == SANEWM_RESIZE_LEFT)
		current_window->width = current_window->width - stepx;
	else if (cases == SANEWM_RESIZE_DOWN)
		current_window->height = current_window->height + stepy;
	else if (cases == SANEWM_RESIZE_UP)
		current_window->height = current_window->height - stepy;
	else if (cases == SANEWM_RESIZE_RIGHT)
		current_window->width = current_window->width + stepx;

	if (current_window->vertical_maxed)
		current_window->vertical_maxed = false;
	if (current_window->horizontal_maxed)
		current_window->horizontal_maxed  = false;

	resize_limit(current_window);
	center_pointer(current_window->id, current_window);
	raise_current_window();
	set_borders(current_window, true);
}

/* Resize window and keep it's aspect ratio (exponentially grow),
 * and round the result (+0.5) */
void
resize_step_aspect(const Arg *arg)
{
	if (NULL == current_window || current_window->maxed)
		return;

	if (arg->i == SANEWM_RESIZE_KEEP_ASPECT_SHRINK) {
		current_window->width = (current_window->width / resize_keep_aspect_ratio)
			+ 0.5;
		current_window->height = (current_window->height / resize_keep_aspect_ratio)
			+ 0.5;
	} else {
		current_window->height = (current_window->height * resize_keep_aspect_ratio)
			+ 0.5;
		current_window->width = (current_window->width * resize_keep_aspect_ratio)
			+ 0.5;
	}

	if (current_window->vertical_maxed)
		current_window->vertical_maxed = false;
	if (current_window->horizontal_maxed)
		current_window->horizontal_maxed  = false;

	resize_limit(current_window);
	center_pointer(current_window->id, current_window);
	raise_current_window();
	set_borders(current_window, true);
}

/* Try to snap to other windows and monitor border */
void
snap_window(struct sane_window *window)
{
	struct list_item *item;
	struct sane_window *win;
	int16_t monitor_x, monitor_y;
	uint16_t monitor_width, monitor_height;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, current_window);

	for (item = workspace_list[current_workspace]; item != NULL; item = item->next) {
		win = item->data;

		if (window != win) {
			if (abs((win->x +win->width) - window->x
				+ conf.border_width) < borders[2])
				if (window->y + window->height > win->y &&
				    window->y < win->y +
				    win->height)
					window->x = (win->x + win->width) +
						(2 * conf.border_width);

			if (abs((win->y + win->height) -
				window->y + conf.border_width) < borders[2])
				if (window->x + window->width >win->x &&
				    window->x < win->x + win->width)
					window->y = (win->y + win->height) +
						(2 * conf.border_width);

			if (abs((window->x + window->width) - win->x +
				conf.border_width) < borders[2])
				if (window->y + window->height > win->y &&
				    window->y < win->y + win->height)
					window->x = (win->x - window->width)
						- (2 * conf.border_width);

			if (abs((window->y + window->height) -
				win->y +
				conf.border_width)
			    < borders[2])
				if (window->x +
				    window->width >win->x &&
				    window->x < win->x +
				    win->width)
					window->y = (win->y -
						     window->height) -
						(2 * conf.border_width);
		}
	}
}


void
move_step(const Arg *arg)
{
	int16_t start_x, start_y;
	uint8_t step, cases=arg->i;

	if (NULL == current_window || current_window->maxed)
		return;

	/* Save pointer position so we can warp pointer here later. */
	if (!get_pointer(&current_window->id, &start_x, &start_y))
		return;

	cases = cases % 4;
	arg->i < 4 ? (step = movements[1]) : (step = movements[0]);

	if (cases == SANEWM_MOVE_LEFT)
		current_window->x = current_window->x - step;
	else if (cases == SANEWM_MOVE_DOWN)
		current_window->y = current_window->y + step;
	else if (cases == SANEWM_MOVE_UP)
		current_window->y = current_window->y - step;
	else if (cases == SANEWM_MOVE_RIGHT)
		current_window->x = current_window->x + step;

	raise_current_window();
	move_limit(current_window);
	move_pointer_back(start_x,start_y,current_window);
	xcb_flush(conn);
}

void
set_borders(struct sane_window *window, const bool isitfocused)
{
	uint32_t values[1];  /* this is the color maintainer */
	uint16_t half = 0;
	bool inv = conf.inverted_colors;

	if (window->maxed || window->ignore_borders)
		return;

	/* Set border width. */
	values[0] = conf.border_width;
	xcb_configure_window(conn, window->id,
			     XCB_CONFIG_WINDOW_BORDER_WIDTH, values);

	if (top_win != 0 &&window->id == top_win)
		inv = !inv;

	half = conf.outer_border;

	xcb_rectangle_t rect_inner[] = {
		{
			window->width,
			0,
			conf.border_width - half, window->height +
			conf.border_width - half
		},
		{
			window->width + conf.border_width + half,
			0,
			conf.border_width - half,
			window->height + conf.border_width - half
		},
		{
			0,
			window->height,
			window->width + conf.border_width -
			half, conf.border_width - half
		},
		{
			0,
			window->height + conf.border_width + half,window->width +
			conf.border_width - half,conf.border_width -
			half
		},
		{
			window->width + conf.border_width +
			half,conf.border_width + window->height +
			half,conf.border_width,
			conf.border_width
		}
	};

	xcb_rectangle_t rect_outer[] = {
		{
			window->width + conf.border_width - half,
			0,
			half,
			window->height + conf.border_width * 2
		},
		{
			window->width + conf.border_width,
			0,
			half,
			window->height + conf.border_width * 2
		},
		{
			0,
			window->height + conf.border_width - half,window->width +
			conf.border_width * 2,
			half
		},
		{
			0,
			window->height + conf.border_width,
			window->width + conf.border_width * 2,
			half
		},
		{
			1, 1, 1, 1
		}
	};

	xcb_pixmap_t pmap = xcb_generate_id(conn);
	xcb_create_pixmap(conn, window->depth, pmap, window->id,
			  window->width + (conf.border_width * 2),
			  window->height + (conf.border_width * 2));

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

	if (window->unkillable || window->fixed) {
		if (window->unkillable && window->fixed)
			values[0]  = conf.fixed_unkill_color;
		else
			if (window->fixed)
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
	xcb_change_window_attributes(conn, window->id, XCB_CW_BORDER_PIXMAP,
				     &values[0]);

	/* free the memory we allocated for the pixmap */
	xcb_free_pixmap(conn,pmap);
	xcb_free_gc(conn,gc);
	xcb_flush(conn);
}

void
unmaximize_window_size(struct sane_window *window)
{
	uint32_t values[5], mask = 0;

	if (NULL == window)
		return;

	window->x = window->original_geometry.x;
	window->y = window->original_geometry.y;
	window->width = window->original_geometry.width;
	window->height = window->original_geometry.height;

	/* Restore geometry. */
	values[0] = window->x;
	values[1] = window->y;
	values[2] = window->width;
	values[3] = window->height;

	window->maxed = window->horizontal_maxed = 0;
	move_resize(window->id, window->x, window->y,
		    window->width, window->height);

	center_pointer(window->id, window);
	set_borders(window, true);
}

void
maximize(const Arg *arg)
{
	maximize_window(current_window, 1);
}

void
fullscreen(const Arg *arg)
{
	maximize_window(current_window, 0);
}


void
unmaximize_window(struct sane_window *window){
	unmaximize_window_size(window);
	window->maxed = false;
	set_borders(window, true);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id,
			    ewmh->_NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, NULL);
}

void
maximize_window(struct sane_window *window, uint8_t with_offsets){
	uint32_t values[4];
	int16_t monitor_x, monitor_y;
	uint16_t monitor_width, monitor_height;

	if (NULL == current_window)
		return;

	/* Check if maximized already. If so, revert to stored geometry. */
	if (current_window->maxed) {
		unmaximize_window(current_window);
		return;
	}

	get_monitor_size(with_offsets, &monitor_x, &monitor_y, &monitor_width, &monitor_height, window);
	maximize_helper(window, monitor_x, monitor_y, monitor_width, monitor_height);
	raise_current_window();
	// TODO Get rid of ewmh fullscreen
	/* if (!with_offsets) { */
	/*	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window->id, */
	/*			    ewmh->_NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &ewmh->_NET_WM_STATE_FULLSCREEN); */
	/* } */
	xcb_flush(conn);
}

void
maximize_vertical_horizontal(const Arg *arg)
{
	uint32_t values[2];
	int16_t monitor_y, monitor_x, temp = 0;
	uint16_t monitor_height, monitor_width;

	if (NULL == current_window)
		return;

	if (current_window->vertical_maxed || current_window->horizontal_maxed) {
		unmaximize_window_size(current_window);
		current_window->vertical_maxed = current_window->horizontal_maxed = false;
		fit_on_screen(current_window);
		set_borders(current_window, true);
		return;
	}

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, current_window);
	save_original_geometry(current_window);
	no_border(&temp, current_window,true);

	if (arg->i == SANEWM_MAXIMIZE_VERTICALLY) {
		current_window->y = monitor_y;
		/* Compute new height considering height increments
		 * and screen height. */
		current_window->height = monitor_height - (conf.border_width * 2);

		/* Move to top of screen and resize. */
		values[0] = current_window->y;
		values[1] = current_window->height;

		xcb_configure_window(conn, current_window->id, XCB_CONFIG_WINDOW_Y
				     | XCB_CONFIG_WINDOW_HEIGHT, values);

		current_window->vertical_maxed = true;
	} else if (arg->i == SANEWM_MAXIMIZE_HORIZONTALLY) {
		current_window->x = monitor_x;
		current_window->width = monitor_width - (conf.border_width * 2);
		values[0] = current_window->x;
		values[1] = current_window->width;

		xcb_configure_window(conn, current_window->id, XCB_CONFIG_WINDOW_X
				     | XCB_CONFIG_WINDOW_WIDTH, values);

		current_window->horizontal_maxed = true;
	}

	no_border(&temp, current_window,false);
	raise_current_window();
	center_pointer(current_window->id, current_window);
	set_borders(current_window,true);
}

void
max_half(const Arg *arg)
{
	uint32_t values[4];
	int16_t monitor_x, monitor_y, temp=0;
	uint16_t monitor_width, monitor_height;

	if (NULL == current_window || current_window->maxed)
		return;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, current_window);
	no_border(&temp, current_window, true);

	if (arg->i>4) {
		if (arg->i>6) {
			/* in folding mode */
			if (arg->i == SANEWM_MAX_HALF_FOLD_VERTICAL)
				current_window->height = current_window->height / 2
					- (conf.border_width);
			else
				current_window->height = current_window->height * 2
					+ (2*conf.border_width);
		} else {
			current_window->y      =  monitor_y;
			current_window->height =  monitor_height - (conf.border_width * 2);
			current_window->width  = ((float)(monitor_width) / 2)
				- (conf.border_width * 2);

			if (arg->i == SANEWM_MAX_HALF_VERTICAL_LEFT)
				current_window->x = monitor_x;
			else
				current_window->x = monitor_x + monitor_width
					- (current_window->width
					   + conf.border_width * 2);
		}
	} else {
		if (arg->i < 2) {
			/* in folding mode */
			if (arg->i == SANEWM_MAX_HALF_FOLD_HORIZONTAL)
				current_window->width = current_window->width / 2
					- conf.border_width;
			else
				current_window->width = current_window->width * 2
					+ (2 * conf.border_width); //unfold
		} else {
			current_window->x     =  monitor_x;
			current_window->width =  monitor_width - (conf.border_width * 2);
			current_window->height = ((float)(monitor_height) / 2)
				- (conf.border_width * 2);

			if (arg->i == SANEWM_MAX_HALF_HORIZONTAL_TOP)
				current_window->y = monitor_y;
			else
				current_window->y = monitor_y + monitor_height
					- (current_window->height
					   + conf.border_width * 2);
		}
	}

	move_resize(current_window->id, current_window->x, current_window->y,
		    current_window->width, current_window->height);

	current_window->vertical_horizontal = true;
	no_border(&temp, current_window, false);
	raise_current_window();
	fit_on_screen(current_window);
	center_pointer(current_window->id, current_window);
	set_borders(current_window, true);
}

void
max_half_half(const Arg *arg)
{
	uint32_t values[4];
	int16_t monitor_x, monitor_y, temp=0;
	uint16_t monitor_width, monitor_height;

	if (NULL == current_window || current_window->maxed)
		return;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, current_window);
	no_border(&temp, current_window, true);

	current_window->x =  monitor_x;
	current_window->y =  monitor_y;

	current_window->width  = ((float)(monitor_width) / 2)
		- (conf.border_width * 2);
	current_window->height = ((float)(monitor_height) / 2)
		- (conf.border_width * 2);

	if (arg->i == SANEWM_MAX_HALF_HALF_BOTTOM_LEFT)
		current_window->y = monitor_y + monitor_height
			- (current_window->height
			   + conf.border_width * 2);
	else if (arg->i == SANEWM_MAX_HALF_HALF_TOP_RIGHT)
		current_window->x = monitor_x + monitor_width
			- (current_window->width
			   + conf.border_width * 2);
	else if (arg->i == SANEWM_MAX_HALF_HALF_BOTTOM_RIGHT) {
		current_window->x = monitor_x + monitor_width
			- (current_window->width
			   + conf.border_width * 2);
		current_window->y = monitor_y + monitor_height
			- (current_window->height
			   + conf.border_width * 2);
	} else if (arg->i == SANEWM_MAX_HALF_HALF_CENTER) {
		current_window->x  += monitor_width - (current_window->width
						     + conf.border_width * 2) + monitor_x;
		current_window->y  += monitor_height - (current_window->height
						      + conf.border_width * 2) + monitor_y;
		current_window->y  = current_window->y / 2;
		current_window->x  = current_window->x / 2;
	}

	move_resize(current_window->id, current_window->x, current_window->y,
		    current_window->width, current_window->height);

	current_window->vertical_horizontal = true;
	no_border(&temp, current_window, false);
	raise_current_window();
	fit_on_screen(current_window);
	center_pointer(current_window->id, current_window);
	set_borders(current_window, true);
}


/* void */
/* hide(void) */
/* { */
/*	if (current_window == NULL) */
/*		return; */

/*	long data[] = { */
/*		XCB_ICCCM_WM_STATE_ICONIC, */
/*		ewmh->_NET_WM_STATE_HIDDEN, */
/*		XCB_NONE */
/*	}; */

/*	/\* Unmap window and declare iconic. Unmapping will generate an */
/*	 * Unmap_Notify event so we can forget about the window later. *\/ */
/*	current_window->iconic = true; */

/*	xcb_unmap_window(conn, current_window->id); */
/*	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, current_window->id, */
/*			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 3, data); */

/*	xcb_flush(conn); */
/* } */


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

	if (NULL == current_window ||
	    NULL == workspace_list[current_workspace] ||
	    current_window->maxed)
		return;

	if (!get_pointer(&current_window->id, &point_x, &point_y))
		return;
	uint16_t tmp_x = current_window->x;
	uint16_t tmp_y = current_window->y;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height,current_window);
	no_border(&temp, current_window, true);
	current_window->x = monitor_x; current_window->y = monitor_y;

	if (arg->i == SANEWM_TELEPORT_CENTER) { /* center */
		current_window->x  += monitor_width - (current_window->width
						     + conf.border_width * 2) + monitor_x;
		current_window->y  += monitor_height - (current_window->height
						      + conf.border_width * 2) + monitor_y;
		current_window->y  = current_window->y / 2;
		current_window->x  = current_window->x / 2;
	} else {
		/* top-left */
		if (arg->i > 3) {
			/* bottom-left */
			if (arg->i == SANEWM_TELEPORT_BOTTOM_LEFT)
				current_window->y += monitor_height - (current_window->height
								     + conf.border_width * 2);
			/* center y */
			else if (arg->i == SANEWM_TELEPORT_CENTER_Y) {
				current_window->x  = tmp_x;
				current_window->y += monitor_height - (current_window->height
								     + conf.border_width * 2)+ monitor_y;
				current_window->y  = current_window->y / 2;
			}
		} else {
			/* top-right */
			if (arg->i < 2)
				/* center x */
				if (arg->i == SANEWM_TELEPORT_CENTER_X) {
					current_window->y  = tmp_y;
					current_window->x += monitor_width
						- (current_window->width
						   + conf.border_width * 2)
						+ monitor_x;
					current_window->x  = current_window->x / 2;
				} else
					current_window->x += monitor_width
						- (current_window->width
						   + conf.border_width * 2);
			else {
				/* bottom-right */
				current_window->x += monitor_width
					- (current_window->width
					   + conf.border_width * 2);
				current_window->y += monitor_height
					- (current_window->height
					   + conf.border_width * 2);
			}
		}
	}

	move_window(current_window->id, current_window->x, current_window->y);
	move_pointer_back(point_x,point_y, current_window);
	no_border(&temp, current_window, false);
	raise_current_window();
	xcb_flush(conn);
}

void
delete_window()
{
	bool use_delete = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	xcb_get_property_cookie_t cookie;

	if (NULL == current_window || current_window->unkillable == true)
		return;

	/* Check if WM_DELETE is supported.  */
	cookie = xcb_icccm_get_wm_protocols_unchecked(conn, current_window->id,
						      ewmh->WM_PROTOCOLS);

	if (current_window->id == top_win)
		top_win = 0;

	if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL) == 1) {
		for (uint32_t i = 0; i < protocols.atoms_len; ++i)
			if (protocols.atoms[i] == ATOM[wm_delete_window]) {
				xcb_client_message_event_t ev = {
					.response_type = XCB_CLIENT_MESSAGE,
					.format = 32,
					.sequence = 0,
					.window = current_window->id,
					.type = ewmh->WM_PROTOCOLS,
					.data.data32 = {
						ATOM[wm_delete_window],
						XCB_CURRENT_TIME
					}
				};

				xcb_send_event(conn, false, current_window->id,
					       XCB_EVENT_MASK_NO_EVENT,
					       (char *)&ev);

				use_delete = true;
				break;
			}

		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	if (!use_delete)
		xcb_kill_client(conn, current_window->id);
}


/* Forget everything about window. */
void
forget_window(struct sane_window *window)
{
	uint32_t ws;

	if (NULL == window)
		return;
	if (window->id == top_win)
		top_win = 0;
	/* Delete window from the workspace list it belongs to. */
	delete_from_workspace(window);

	/* Remove from global window list. */
	free_item(&window_list, NULL, window->window_item);
}

/* Forget everything about a window with window->id win. */
void
forget_window_id(xcb_window_t win)
{
	struct sane_window *window;
	struct list_item *item;

	for (item = window_list; item != NULL; item = item->next) {
		/* Find this window in the global window list. */
		window = item->data;
		if (win == window->id) {
			/* Forget it and free allocated data, it might
			 * already be freed by handling an Unmap_Notify. */
			forget_window(window);
			return;
		}
	}
}

void
no_border(int16_t *temp, struct sane_window *window, bool set_unset)
{
	if (window->ignore_borders) {
		if (set_unset) {
			*temp = conf.border_width;
			conf.border_width = 0;
		} else
			conf.border_width = *temp;
	}
}

void
maximize_helper(struct sane_window *window, uint16_t monitor_x, uint16_t monitor_y,
		uint16_t monitor_width, uint16_t monitor_height)
{
	uint32_t values[4];

	values[0] = 0;
	save_original_geometry(window);
	xcb_configure_window(conn, window->id, XCB_CONFIG_WINDOW_BORDER_WIDTH,
			     values);

	window->x = monitor_x;
	window->y = monitor_y;
	window->width = monitor_width;
	window->height = monitor_height;

	move_resize(window->id, window->x, window->y, window->width,
		    window->height);
	window->maxed = true;
}

/* Fit window on physical screen, moving and resizing as necessary. */
void
fit_on_screen(struct sane_window *window)
{
	int16_t monitor_x, monitor_y,temp=0;
	uint16_t monitor_width, monitor_height;
	bool will_move, will_resize;

	will_move = will_resize = window->vertical_maxed = window->horizontal_maxed = false;

	get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width, &monitor_height, window);

	if (window->maxed) {
		window->maxed = false;
		set_borders(window, false);
	} else {
		/* not maxed but look as if it was maxed, then make it maxed */
		if (window->width == monitor_width && window->height == monitor_height)
			will_resize = true;
		else {
			get_monitor_size(0, &monitor_x, &monitor_y, &monitor_width, &monitor_height,
					 window);
			if (window->width == monitor_width && window->height ==
			    monitor_height)
				will_resize = true;
		}
		if (will_resize) {
			window->x = monitor_x;
			window->y = monitor_y;
			window->width -= conf.border_width * 2;
			window->height -= conf.border_width * 2;
			maximize_helper(window, monitor_x, monitor_y, monitor_width,
					monitor_height);
			return;
		} else {
			get_monitor_size(1, &monitor_x, &monitor_y, &monitor_width,
					 &monitor_height, window);
		}
	}

	if (window->x > monitor_x + monitor_width ||
	    window->y > monitor_y + monitor_height ||
	    window->x < monitor_x ||
	    window->y < monitor_y) {
		will_move = true;
		/* Is it outside the physical monitor? */
		if (window->x > monitor_x + monitor_width)
			window->x = monitor_x + monitor_width
				- window->width - conf.border_width*2;
		if (window->y > monitor_y + monitor_height)
			window->y = monitor_y + monitor_height - window->height
				- conf.border_width * 2;
		if (window->x < monitor_x)
			window->x = monitor_x;
		if (window->y < monitor_y)
			window->y = monitor_y;
	}

	/* Is it smaller than it wants to  be? */
	if (0 != window->min_height && window->height < window->min_height) {
		window->height = window->min_height;

		will_resize = true;
	}

	if (0 != window->min_width && window->width < window->min_width) {
		window->width = window->min_width;
		will_resize = true;
	}

	no_border(&temp, window, true);
	/* If the window is larger than our screen, just place it in
	 * the corner and resize. */
	if (window->width + conf.border_width * 2 > monitor_width) {
		window->x = monitor_x;
		window->width = monitor_width - conf.border_width * 2;
		will_move = will_resize = true;
	} else
		if (window->x + window->width + conf.border_width * 2
		    > monitor_x + monitor_width) {
			window->x = monitor_x + monitor_width - (window->width
								 + conf.border_width * 2);
			will_move = true;
		}
	if (window->height + conf.border_width * 2 > monitor_height) {
		window->y = monitor_y;
		window->height = monitor_height - conf.border_width * 2;
		will_move = will_resize = true;
	} else
		if (window->y + window->height + conf.border_width * 2 > monitor_y
		    + monitor_height) {
			window->y = monitor_y + monitor_height - (window->height
								  + conf.border_width * 2);
			will_move = true;
		}

	if (will_move)
		move_window(window->id, window->x, window->y);

	if (will_resize)
		resize(window->id, window->width, window->height);

	no_border(&temp, window, false);
}

void
save_original_geometry(struct sane_window *window)
{
	window->original_geometry.x      = window->x;
	window->original_geometry.y      = window->y;
	window->original_geometry.width  = window->width;
	window->original_geometry.height = window->height;
}

void
update_window_list(void)
{
	uint32_t len, i;
	xcb_window_t *children;
	struct sane_window *window;

	/* can only be called after the first window has been spawn */
	xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn,
							     xcb_query_tree(conn, screen->root), 0);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST_STACKING);

	if (reply == NULL) {
		add_to_window_list(0);
		return;
	}

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	for (i = 0; i < len; ++i) {
		window = find_window(&children[i]);
		if (window != NULL)
			add_to_window_list(window->id);
	}

	free(reply);
}

/* Walk through all existing windows and set them up. returns true on success */
bool
setup_screen(void)
{
	xcb_get_window_attributes_reply_t *attr;
	struct sane_window *window;
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
			window = setup_window(children[i]);

			if (NULL != window) {
				/* Find the physical output this window will be on if
				 * RANDR is active. */
				if (-1 != randr_base)
					window->monitor = find_monitor_by_coordinate(window->x,
										     window->y);
				/* Fit window on physical screen. */
				fit_on_screen(window);
				set_borders(window, false);

				/* Check if this window has a workspace set already
				 * as a WM hint. */
				ws = get_wm_desktop(children[i]);

				if (get_unkill_state(children[i]))
					unkillable_window(window);

				if (ws == NET_WM_FIXED) {
					/* Add to current workspace. */
					add_to_workspace(window, current_workspace);
					/* Add to all other workspaces. */
					fix_window(window);
				} else {
					if (SANEWM_NOWS != ws && ws < WORKSPACES) {
						add_to_workspace(window, ws);
						if (ws != current_workspace)
							/* If it's not our current works
							 * pace, hide it. */
							xcb_unmap_window(conn,
									 window->id);
					} else {
						add_to_workspace(window, current_workspace);
						add_to_window_list(children[i]);
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
struct sane_window
create_back_window(void)
{
	struct sane_window temp_win;
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
			  current_window->x,
			  current_window->y,
			  /* width, height */
			  current_window->width,
			  current_window->height,
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

	temp_win.x                    = current_window->x;
	temp_win.y                    = current_window->y;
	temp_win.width                = current_window->width;
	temp_win.unkillable           = current_window->unkillable;
	temp_win.fixed                = current_window->fixed;
	temp_win.height               = current_window->height;
	temp_win.width_increment      = current_window->width_increment;
	temp_win.height_increment     = current_window->height_increment;
	temp_win.base_width           = current_window->base_width;
	temp_win.base_height          = current_window->base_height;
	temp_win.monitor              = current_window->monitor;
	temp_win.min_height           = current_window->min_height;
	temp_win.min_width            = current_window->min_height;
	temp_win.ignore_borders       = current_window->ignore_borders;

	return temp_win;
}
