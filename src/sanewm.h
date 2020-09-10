#ifndef SANEWM_SANEWM_H
#define SANEWM_SANEWM_H

#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_xrm.h>

#include <X11/keysym.h>

#include "definitions.h"
#include "types.h"

///---Internal Constants---///
///---Globals---///
extern xcb_generic_event_t *ev;
extern void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *e);
extern unsigned int num_lock_mask;
extern bool is_sloppy;              // by default use sloppy focus
extern int sig_code;                           // Signal code. Non-zero if we've been interruped by a signal.
extern xcb_connection_t *conn;             // Connection to X server.
extern xcb_ewmh_connection_t *ewmh;        // Ewmh Connection.
extern xcb_screen_t     *screen;           // Our current screen.
extern int randr_base;                         // Beginning of RANDR extension events.

extern uint8_t current_workspace;                  // Current workspace.

extern struct sane_window *current_window;            // Current focus window.
extern xcb_drawable_t top_win;           // Window always on top.
extern struct list_item *window_list;        // Global list of all client windows.
extern struct list_item *monitor_list;        // List of all physical monitor outputs.
extern struct list_item *workspace_list[WORKSPACES];

extern xcb_randr_output_t primary_output_monitor;
extern struct monitor *focused_monitor;
///---Global configuration.---///
extern const char *atom_names[NUM_ATOMS][1];

extern xcb_atom_t ATOM[NUM_ATOMS];

extern struct conf conf;

///---Functions prototypes---///
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
