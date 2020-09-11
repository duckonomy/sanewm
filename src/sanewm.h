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

#ifndef SANEWM_SANEWM_H
#define SANEWM_SANEWM_H

#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>

#include <X11/keysym.h>

#include "definitions.h"
#include "types.h"

extern xcb_generic_event_t *ev;
extern void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *e);
extern unsigned int num_lock_mask;
extern bool is_sloppy;
extern int sig_code;
extern xcb_connection_t *conn;
extern xcb_ewmh_connection_t *ewmh;
extern xcb_screen_t     *screen;
extern int randr_base;
extern uint8_t current_workspace;

extern struct sane_window *current_window;
extern xcb_drawable_t top_win;
extern struct list_item *window_list;
extern struct list_item *monitor_list;
extern struct list_item *workspace_list[WORKSPACES];

extern xcb_randr_output_t primary_output_monitor;
extern struct monitor *current_monitor;
extern const char *atom_names[NUM_ATOMS][1];
extern xcb_atom_t ATOM[NUM_ATOMS];
extern struct conf conf;

void run(void);
bool setup(int);
void install_sig_handlers(void);
void start(const Arg *);
void sanewm_restart();
void sanewm_exit();
void cleanup(void);
void sig_catch(const int);
xcb_atom_t get_atom(const char *);

#endif
