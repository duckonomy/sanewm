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
