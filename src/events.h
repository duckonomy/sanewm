#ifndef SANEWM_EVENTS_H
#define SANEWM_EVENTS_H

#include <xcb/xcb.h>

#include "types.h"

void destroy_notify(xcb_generic_event_t *);
void enter_notify(xcb_generic_event_t *);
void unmap_notify(xcb_generic_event_t *);
void config_notify(xcb_generic_event_t *);
void map_request(xcb_generic_event_t *);
void circulate_request(xcb_generic_event_t *);
void handle_keypress(xcb_generic_event_t *);
void configure_window(xcb_window_t, uint16_t, const struct winconf *);
void configure_request(xcb_generic_event_t *);
void button_press(xcb_generic_event_t *);
void client_message(xcb_generic_event_t *);

// could be in pointer.c
void mouse_motion(const Arg *);

#endif
