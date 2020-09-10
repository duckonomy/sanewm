#include "monitor.h"

#include "sanewm.h"
#include "list.h"
#include "config.h"
#include "pointer.h"

void
delete_monitor(struct monitor *mon)
{
	free_item(&monitor_list, NULL, mon->item);
}

/* get screen of display */
xcb_screen_t *
xcb_screen_of_display(xcb_connection_t *con, int screen)
{
	xcb_screen_iterator_t iter;
	iter = xcb_setup_roots_iterator(xcb_get_setup(con));
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return iter.data;

	return NULL;
}

void
get_monitor_size(int8_t with_offsets, int16_t *monitor_x, int16_t *monitor_y,
		 uint16_t *monitor_width, uint16_t *monitor_height,
		 const struct client *client)
{
	if (NULL == client || NULL == client->monitor) {
		/* Window isn't attached to any monitor, so we use
		 * the root window size. */
		*monitor_x      = *monitor_y = 0;
		*monitor_width  = screen->width_in_pixels;
		*monitor_height = screen->height_in_pixels;
	} else {
		*monitor_x      = client->monitor->x;
		*monitor_y      = client->monitor->y;
		*monitor_width  = client->monitor->width;
		*monitor_height = client->monitor->height;
	}

	if (with_offsets) {
		*monitor_x      += offsets[0];
		*monitor_y      += offsets[1];
		*monitor_width  -= offsets[2];
		*monitor_height -= offsets[3];
	}
}



/* Set up RANDR extension. Get the extension base and subscribe to events */
int
setup_randr(void)
{
	int base;
	const xcb_query_extension_reply_t *extension
		= xcb_get_extension_data(conn, &xcb_randr_id);

	if (!extension->present)
		return -1;
	else
		getrandr();

	base = extension->first_event;
	xcb_randr_select_input(conn, screen->root,
			       XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE |
			       XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
			       XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
			       XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

	return base;
}

/* Get RANDR resources and figure out how many outputs there are. */
void
getrandr(void)
{
	int len;
	xcb_randr_get_screen_resources_current_cookie_t rcookie
		= xcb_randr_get_screen_resources_current(conn, screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res
		= xcb_randr_get_screen_resources_current_reply(conn, rcookie,
							       NULL);

	if (NULL == res)
		return;

	xcb_timestamp_t timestamp = res->config_timestamp;
	len= xcb_randr_get_screen_resources_current_outputs_length(res);
	xcb_randr_output_t *outputs
		= xcb_randr_get_screen_resources_current_outputs(res);

	/* Request information for all outputs. */
	get_outputs(outputs, len, timestamp);
	free(res);
}

/* Walk through all the RANDR outputs (number of outputs == len) there */
void
get_outputs(xcb_randr_output_t *outputs, const int len,
	    xcb_timestamp_t timestamp)
{
	int i;
	int name_len;
	char *name;

	/* was at time timestamp. */
	xcb_randr_get_crtc_info_cookie_t icookie;
	xcb_randr_get_crtc_info_reply_t *crtc = NULL;
	xcb_randr_get_output_info_reply_t *output;
	struct monitor *mon, *clone_monitor;
	struct item *item;
	xcb_randr_get_output_info_cookie_t ocookie[len];

	for (i = 0; i < len; ++i)
		ocookie[i] = xcb_randr_get_output_info(conn, outputs[i],
						       timestamp);

	/* Loop through all outputs. */
	for (i = 0; i < len; ++i) {
		if ((output = xcb_randr_get_output_info_reply(conn, ocookie[i],
							      NULL)) == NULL)
			continue;

		//name_len = MIN(16, xcb_randr_get_output_info_name_length(output));
		//name = malloc(name_len+1);
		//snprintf(name, name_len+1, "%.*s", name_len,
		//		xcb_randr_get_output_info_name(output));

		if (XCB_NONE != output->crtc) {
			icookie = xcb_randr_get_crtc_info(conn, output->crtc,
							  timestamp);
			crtc = xcb_randr_get_crtc_info_reply(conn, icookie, NULL);

			if (NULL == crtc)
				return;

			/* Check if it's a clone. */
			// TODO maybe they are not cloned, one might be bigger
			// than the other after closing the lid
			clone_monitor = find_clones(outputs[i], crtc->x, crtc->y);

			if (NULL != clone_monitor)
				continue;

			/* Do we know this monitor already? */
			if (NULL == (mon = find_monitor(outputs[i])))
				add_monitor(outputs[i], crtc->x, crtc->y,
					    crtc->width,crtc->height);
			else
				/* We know this monitor. Update information.
				 * If it's smaller than before, rearrange windows. */
				if (crtc->x != mon->x ||
				    crtc->y != mon->y ||
				    crtc->width != mon->width ||
				    crtc->height != mon->height) {
					if (crtc->x != mon->x)
						mon->x = crtc->x;
					if (crtc->y != mon->y)
						mon->y = crtc->y;
					if (crtc->width != mon->width)
						mon->width = crtc->width;
					if (crtc->height != mon->height)
						mon->height = crtc->height;

					// TODO when lid closed, one screen
					arrange_by_monitor(mon);
				}
			free(crtc);
		} else {
			/* Check if it was used before. If it was, do something. */
			if ((mon = find_monitor(outputs[i]))) {
				struct client *client;
				for (item = window_list; item != NULL; item = item->next) {
					/* Check all windows on this monitor
					 * and move them to the next or to the
					 * first monitor if there is no next. */
					client = item->data;

					if (client->monitor == mon) {
						if (NULL == client->monitor->item->next)
							if (NULL == monitor_list)
								client->monitor = NULL;
							else
								client->monitor = monitor_list->data;
						else
							client->monitor = client->monitor->item->next->data;
						fit_on_screen(client);
					}
				}

				/* It's not active anymore. Forget about it. */
				delete_monitor(mon);
			}
		}
		if (NULL != output)
			free(output);

		free(name);
	} /* for */
}

void
arrange_by_monitor(struct monitor *monitor)
{
	struct client *client;
	struct item *item;

	for (item = window_list; item != NULL; item = item->next) {
		client = item->data;

		if (client->monitor == monitor)
			fit_on_screen(client);
	}
}

struct monitor *
find_monitor(xcb_randr_output_t id)
{
	struct monitor *mon;
	struct item *item;

	for (item = monitor_list; item != NULL; item = item->next) {
		mon = item->data;

		if (id == mon->id)
			return mon;
	}

	return NULL;
}

struct monitor *
find_clones(xcb_randr_output_t id, const int16_t x, const int16_t y)
{
	struct monitor *clone_monitor;
	struct item *item;

	for (item = monitor_list; item != NULL; item = item->next) {
		clone_monitor = item->data;

		/* Check for same position. */
		if (id != clone_monitor->id && clone_monitor->x == x && clone_monitor->y == y)
			return clone_monitor;
	}

	return NULL;
}

struct monitor *
find_monitor_by_coordinate(const int16_t x, const int16_t y)
{
	struct monitor *mon;
	struct item *item;

	for (item = monitor_list; item != NULL; item = item->next) {
		mon = item->data;

		if (x >= mon->x && x <= mon->x + mon->width && y >= mon->y && y
		    <= mon->y+mon->height)
			return mon;
	}

	return NULL;
}

struct monitor *
add_monitor(xcb_randr_output_t id, const int16_t x, const int16_t y,
	    const uint16_t width,const uint16_t height)
{
	struct item *item;
	struct monitor *mon = malloc(sizeof(struct monitor));

	if (NULL == (item = add_item(&monitor_list)))
		return NULL;

	if (NULL == mon)
		return NULL;

	item->data  = mon;
	mon->id     = id;
	mon->item   = item;
	mon->x      = x;
	mon->y      = y;
	mon->width  = width;
	mon->height = height;

	return mon;
}

/* TODO Change both of these to get actual circular behavior rather than just two conditionals */
/* TODO Also when you kill a window and then try executing (it probably changes its pointer->root) */
/* Actually need a underlying window (fake) for detecting focus with mouse change... */
/* Causes some random death when hibernating */
void
switch_screen(const Arg *arg)
{
	int16_t cur_x;
	int16_t cur_y;

	struct item *item, *head, *tail;

	tail = head = focused_monitor->item;

	item = focused_monitor->item;
	while (item != NULL) {
		tail = item;
		item = item->next;
	}

	item = focused_monitor->item;
	while (item != NULL) {
		head = item;
		item = item->prev;
	}

	// FIXME for some reason (maybe a running process) this breaks lol
	/* tail->next = head; */
	/* head->prev = tail; */

	if (arg->i == SANEWM_NEXT_SCREEN) {

		item = focused_monitor->item;
		if (item->next == NULL)
			item = head;
		else
			item = item->next;
	} else {
		item = focused_monitor->item;
		if (item->prev == NULL)
			item = tail;
		else
			item = item->prev;

	}

	// The opposite would also be NULL
	if (item == NULL)
		return;

	struct monitor *other_monitor = item->data;

	focused_monitor = other_monitor;

	other_monitor = NULL;

	cur_x = focused_monitor->x + focused_monitor->width / 2;
	cur_y = focused_monitor->y + focused_monitor->height / 2;

	xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, cur_x, cur_y);
}

/* Should also switch focused monitors  (it is definative) */
void
change_screen(const Arg *arg)
{
	struct item *item, *head, *tail;
	float x_percentage, y_percentage;

	if (NULL == focus_window || NULL == focus_window->monitor)
		return;

	//
	/* struct item *head = focus_window->monitor->item; */
	/* struct item *tail, *item = NULL; */

	/* if (arg->i == SANEWM_NEXT_SCREEN) */
	/*	item = focus_window->monitor->item->next; */



	tail = head = focused_monitor->item;

	item = focused_monitor->item;
	while (item != NULL) {
		tail = item;
		item = item->next;
	}

	item = focused_monitor->item;
	while (item != NULL) {
		head = item;
		item = item->prev;
	}

	// FIXME for some reason (maybe a running process) this breaks lol
	/* tail->next = head; */
	/* head->prev = tail; */

	if (arg->i == SANEWM_NEXT_SCREEN) {

		item = focused_monitor->item;
		if (item->next == NULL)
			item = head;
		else
			item = item->next;
	} else {
		item = focused_monitor->item;
		if (item->prev == NULL)
			item = tail;
		else
			item = item->prev;

	}

	/* if (arg->i == SANEWM_NEXT_SCREEN) { */
	/*	if (focus_window->monitor->item->next != NULL) { */
	/*		item = focus_window->monitor->item->next; */
	/*	} else { */
	/*		item = focus_window->monitor->item->prev; */
	/*	} */
	/* } else { */
	/*	if (focus_window->monitor->item->prev != NULL) { */
	/*		item = focus_window->monitor->item->prev; */
	/*	} else { */
	/*		item = focus_window->monitor->item->next; */
	/*	} */
	/* } */
	/* else */
	/* item = focus_window->monitor->item->prev; */

	if (NULL == item)
		return;

	x_percentage = (float)((focus_window->x - focus_window->monitor->x)
			       / (focus_window->monitor->width));
	y_percentage = (float)((focus_window->y - focus_window->monitor->y)
			       / (focus_window->monitor->height));

	focus_window->monitor = item->data;

	focus_window->x = focus_window->monitor->width * x_percentage
		+ focus_window->monitor->x + 0.5;
	focus_window->y = focus_window->monitor->height * y_percentage
		+ focus_window->monitor->y + 0.5;

	focused_monitor = focus_window->monitor;

	raise_current_window();
	fit_on_screen(focus_window);
	move_limit(focus_window);
	set_borders(focus_window, true);
	center_pointer(focus_window->id, focus_window);
}
