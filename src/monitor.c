#include "monitor.h"

#include "sanewm.h"
#include "list.h"
#include "config.h"
#include "pointer.h"

void
delete_monitor(struct monitor *monitor)
{
	free_item(&monitor_list, NULL, monitor->item);
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
		 const struct sane_window *window)
{
	if (NULL == window || NULL == window->monitor) {
		/* Window isn't attached to any monitor, so we use
		 * the root window size. */
		*monitor_x      = *monitor_y = 0;
		*monitor_width  = screen->width_in_pixels;
		*monitor_height = screen->height_in_pixels;
	} else {
		*monitor_x      = window->monitor->x;
		*monitor_y      = window->monitor->y;
		*monitor_width  = window->monitor->width;
		*monitor_height = window->monitor->height;
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
		get_randr();

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
get_randr(void)
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
	len = xcb_randr_get_screen_resources_current_outputs_length(res);
	xcb_randr_output_t *outputs =
		xcb_randr_get_screen_resources_current_outputs(res);

	/* Request information for all outputs. */
	get_randr_outputs(outputs, len, timestamp);
	free(res);
}

/* Walk through all the RANDR outputs (number of outputs == len) there */
void
get_randr_outputs(xcb_randr_output_t *outputs, const int len,
	    xcb_timestamp_t timestamp)
{
	int i;
	int name_len;
	char *name;

	/* was at time timestamp. */
	xcb_randr_get_crtc_info_cookie_t icookie;
	xcb_randr_get_crtc_info_reply_t *crtc = NULL;
	xcb_randr_get_output_info_reply_t *output;
	struct monitor *original_monitor, *clone_monitor;
	struct list_item *item;
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
			clone_monitor = find_clone_monitors(outputs[i], crtc->x, crtc->y);

			if (NULL != clone_monitor)
				continue;

			/* Do we know this monitor already? */
			if (NULL == (original_monitor = find_monitor(outputs[i])))
				add_monitor(outputs[i], crtc->x, crtc->y,
					    crtc->width,crtc->height);
			else
				/* We know this monitor. Update information.
				 * If it's smaller than before, rearrange windows. */
				if (crtc->x != original_monitor->x ||
				    crtc->y != original_monitor->y ||
				    crtc->width != original_monitor->width ||
				    crtc->height != original_monitor->height) {
					if (crtc->x != original_monitor->x)
						original_monitor->x = crtc->x;
					if (crtc->y != original_monitor->y)
						original_monitor->y = crtc->y;
					if (crtc->width != original_monitor->width)
						original_monitor->width = crtc->width;
					if (crtc->height != original_monitor->height)
						original_monitor->height = crtc->height;

					// TODO when lid closed, one screen
					arrange_by_monitor(original_monitor);
				}
			free(crtc);
		} else {
			/* Check if it was used before. If it was, do something. */
			if ((original_monitor = find_monitor(outputs[i]))) {
				struct sane_window *window;
				for (item = window_list; item != NULL; item = item->next) {
					/* Check all windows on this monitor
					 * and move them to the next or to the
					 * first monitor if there is no next. */
					window = item->data;

					if (window->monitor == original_monitor) {
						if (NULL == window->monitor->item->next)
							if (NULL == monitor_list)
								window->monitor = NULL;
							else
								window->monitor = monitor_list->data;
						else
							window->monitor = window->monitor->item->next->data;
						fit_window_on_screen(window);
					}
				}

				/* It's not active anymore. Forget about it. */
				delete_monitor(original_monitor);
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
	struct sane_window *window;
	struct list_item *item;

	for (item = window_list; item != NULL; item = item->next) {
		window = item->data;

		if (window->monitor == monitor)
			fit_window_on_screen(window);
	}
}

struct monitor *
find_monitor(xcb_randr_output_t id)
{
	struct monitor *monitor;
	struct list_item *item;

	for (item = monitor_list; item != NULL; item = item->next) {
		monitor = item->data;

		if (id == monitor->id)
			return monitor;
	}

	return NULL;
}

struct monitor *
find_clone_monitors(xcb_randr_output_t id, const int16_t x, const int16_t y)
{
	struct monitor *clone_monitor;
	struct list_item *item;

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
	struct monitor *monitor;
	struct list_item *item;

	for (item = monitor_list; item != NULL; item = item->next) {
		monitor = item->data;

		if (x >= monitor->x && x <= monitor->x + monitor->width && y >= monitor->y && y
		    <= monitor->y+monitor->height)
			return monitor;
	}

	return NULL;
}

struct monitor *
add_monitor(xcb_randr_output_t id, const int16_t x, const int16_t y,
	    const uint16_t width, const uint16_t height)
{
	struct list_item *item;
	struct monitor *monitor = malloc(sizeof(struct monitor));

	if (NULL == (item = add_item(&monitor_list)))
		return NULL;

	if (NULL == monitor)
		return NULL;

	item->data	= monitor;
	monitor->id     = id;
	monitor->item   = item;
	monitor->x      = x;
	monitor->y      = y;
	monitor->width  = width;
	monitor->height = height;

	return monitor;
}

/* Actually need a underlying window (fake) for detecting focus with mouse change... */
/* Causes some random death when hibernating */
void
change_monitor(const Arg *arg)
{
	int16_t cur_x;
	int16_t cur_y;

	struct list_item *item, *head, *tail;

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

void
send_to_monitor(const Arg *arg)
{
	struct list_item *item, *head, *tail;
	float x_percentage, y_percentage;

	if (NULL == current_window || NULL == current_window->monitor)
		return;

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

	if (NULL == item)
		return;

	x_percentage = (float)((current_window->x - current_window->monitor->x) /
			       (current_window->monitor->width));
	y_percentage = (float)((current_window->y - current_window->monitor->y) /
			       (current_window->monitor->height));

	current_window->monitor = item->data;

	current_window->x = current_window->monitor->width * x_percentage +
		current_window->monitor->x + 0.5;
	current_window->y = current_window->monitor->height * y_percentage +
		current_window->monitor->y + 0.5;

	focused_monitor = current_window->monitor;

	raise_current_window();
	fit_window_on_screen(current_window);
	move_window_limit(current_window);
	set_window_borders(current_window, true);
	center_pointer(current_window->id, current_window);
}
