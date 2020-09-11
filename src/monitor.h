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

#ifndef SANEWM_MONITOR_H
#define SANEWM_MONITOR_H

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include "types.h"

xcb_screen_t *xcb_screen_of_display(xcb_connection_t *, int);
void get_monitor_size(int8_t, int16_t *, int16_t *, uint16_t *, uint16_t *, const struct sane_window *);
int setup_randr(void);
void get_randr(void);
void get_randr_outputs(xcb_randr_output_t *, const int, xcb_timestamp_t);
void arrange_by_monitor(struct monitor *);
struct monitor *find_monitor(xcb_randr_output_t);
struct monitor *find_clone_monitors(xcb_randr_output_t, const int16_t, const int16_t);
struct monitor *find_monitor_by_coordinate(const int16_t, const int16_t);
struct monitor *add_monitor(xcb_randr_output_t, const int16_t, const int16_t, const uint16_t, const uint16_t);
void delete_monitor(struct monitor *);
void change_monitor(const Arg *);
void send_to_monitor(const Arg *);

#endif
