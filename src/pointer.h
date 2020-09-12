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

#ifndef SANEWM_POINTER_H
#define SANEWM_POINTER_H

#include <xcb/xcb.h>
#include "types.h"

xcb_cursor_t create_font_cursor(xcb_connection_t *, uint16_t);
void cursor_move(const Arg *);
bool get_pointer(const xcb_drawable_t *, int16_t *, int16_t *);
void mouse_resize(struct sane_window *, const int16_t, const int16_t);
/* void mouse_move(const int16_t, const int16_t); */
void mouse_move_monitor(const int16_t, const int16_t);
void move_pointer_back(const int16_t, const int16_t, const struct sane_window *);
void center_pointer(xcb_drawable_t, struct sane_window *);
void grab_buttons(struct sane_window *);

#endif
