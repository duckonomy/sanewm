/* SaneWM, a floating WM
 * Copyright (c) 2017, 2020 Ian Park, contact at duckonomy dot org.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "list.h"
#include "window.h"
#include "sanewm.h"
#include "events.h"
#include "ewmh.h"
#include "monitor.h"
#include "helpers.h"
#include "desktop.h"
#include "keys.h"
#include "config.h"

xcb_generic_event_t *ev = NULL;
void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *e);
unsigned int num_lock_mask = 0;
bool is_sloppy = true;              // by default use sloppy focus
int sig_code = 0;                           // Signal code. Non-zero if we've been interruped by a signal.
xcb_connection_t *conn = NULL;             // Connection to X server.
xcb_ewmh_connection_t *ewmh = NULL;        // Ewmh Connection.
xcb_screen_t     *screen = NULL;           // Our current screen.
int randr_base = 0;                         // Beginning of RANDR extension events.

struct sane_window *current_window = NULL;            // Current focus window.
xcb_drawable_t top_win = 0;           // Window always on top.
struct list_item *window_list = NULL;        // Global list of all client windows.
struct list_item *monitor_list = NULL;        // List of all physical monitor outputs.
struct list_item *workspace_list[WORKSPACES];
uint8_t current_workspace = 0;                  // Current workspace.

xcb_randr_output_t primary_output_monitor;
struct monitor *focused_monitor;

///---Global configuration.---///
const char *atom_names[NUM_ATOMS][1] = {
	{"WM_DELETE_WINDOW"},
	{"WM_CHANGE_STATE"}
};

xcb_atom_t ATOM[NUM_ATOMS];

struct conf conf;

///---Function bodies---///


void
sanewm_exit()
{
	exit(EXIT_SUCCESS);
}


/* Set keyboard focus to follow mouse pointer. Then exit. We don't need to
 * bother mapping all windows we know about. They should all be in the X
 * server's Save Set and should be mapped automagically. */
void
cleanup(void)
{
	free(ev);
	if (monitor_list)
		delete_all_items(&monitor_list,NULL);
	struct list_item *curr, *workspace_item;
	for (int i = 0; i < WORKSPACES; ++i){
		if (!workspace_list[i])
			continue;
		curr = workspace_list[i];
		while (curr) {
			workspace_item = curr;
			curr = curr->next;
			free(workspace_item);
		}
	}
	if (window_list){
		delete_all_items(&window_list,NULL);
	}
	if (ewmh != NULL){
		xcb_ewmh_connection_wipe(ewmh);
		free(ewmh);
	}
	if (!conn){
		return;
	}
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	xcb_flush(conn);
	xcb_disconnect(conn);
}

void
start(const Arg *arg)
{
	if (fork())
		return;

	//	if (conn)
	//		close(screen->root);

	setsid();
	execvp((char*)arg->com[0], (char**)arg->com);
}


void
run(void)
{
	sig_code = 0;

	while (0 == sig_code) {
		/* the WM is running */
		xcb_flush(conn);

		if (xcb_connection_has_error(conn)){
			cleanup();
			abort();
		}
		if ((ev = xcb_wait_for_event(conn))) {
			if (ev->response_type == randr_base +
			    XCB_RANDR_SCREEN_CHANGE_NOTIFY)
				get_randr();

			if (events[ev->response_type & ~0x80])
				events[ev->response_type & ~0x80](ev);

			if (top_win != 0)
				raise_window(top_win);

			free(ev);
		}
	}
	if (sig_code == SIGHUP) {
		sig_code = 0;
		sanewm_restart();
	}
}

bool
setup(int scrno)
{
	unsigned int i;
	uint32_t event_mask_pointer[] = { XCB_EVENT_MASK_POINTER_MOTION };

	unsigned int values[1] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE
		| XCB_EVENT_MASK_BUTTON_PRESS
	};

	screen = xcb_screen_of_display(conn, scrno);

	if (!screen)
		return false;

	ewmh_init();
	xcb_ewmh_set_wm_pid(ewmh, screen->root, getpid());
	xcb_ewmh_set_wm_name(ewmh, screen->root, 4, "sanewm");

	xcb_atom_t net_atoms[] = {
		ewmh->_NET_SUPPORTED,              ewmh->_NET_WM_DESKTOP,
		ewmh->_NET_NUMBER_OF_DESKTOPS,     ewmh->_NET_CURRENT_DESKTOP,
		ewmh->_NET_ACTIVE_WINDOW,          ewmh->_NET_WM_ICON,
		ewmh->_NET_WM_STATE,               ewmh->_NET_WM_NAME,
		ewmh->_NET_SUPPORTING_WM_CHECK ,   ewmh->_NET_WM_STATE_HIDDEN,
		ewmh->_NET_WM_ICON_NAME,           ewmh->_NET_WM_WINDOW_TYPE,
		ewmh->_NET_WM_WINDOW_TYPE_DOCK,    ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
		ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR, ewmh->_NET_WM_PID,
		ewmh->_NET_CLIENT_LIST,            ewmh->_NET_CLIENT_LIST_STACKING,
		ewmh->WM_PROTOCOLS,                ewmh->_NET_WM_STATE,
		ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
		ewmh->_NET_WM_STATE_FULLSCREEN
	};

	xcb_ewmh_set_supported(ewmh, scrno, LENGTH(net_atoms), net_atoms);

	xcb_xrm_database_t* db = xcb_xrm_database_from_default(conn);

	// Load the default config anyway.
	conf.border_width			= borders[1];
	conf.outer_border		 = borders[0];
	conf.focus_color				 = get_color(colors[0]);
	conf.unfocus_color			 = get_color(colors[1]);
	conf.fixed_color				 = get_color(colors[2]);
	conf.unkill_color				= get_color(colors[3]);
	conf.outer_border_color = get_color(colors[5]);
	conf.fixed_unkill_color	= get_color(colors[4]);
	conf.empty_color				= get_color(colors[6]);
	conf.inverted_colors	= inverted_colors;
	conf.enable_compton	 = false;

	if (db != NULL) {
		char* value;

		if (xcb_xrm_resource_get_string(db, "sanewm.border_width", NULL, &value) >= 0)
			conf.border_width = atoi(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.outer_border", NULL, &value) >= 0)
			conf.outer_border = atoi(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.focus_color", NULL, &value) >= 0)
			conf.focus_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.unfocus_color", NULL, &value) >= 0)
			conf.unfocus_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.fixed_color", NULL, &value) >= 0)
			conf.fixed_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.unkill_color", NULL, &value) >= 0)
			conf.unkill_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.outer_border_color", NULL, &value) >= 0)
			conf.outer_border_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.fixed_unkill_color", NULL, &value) >= 0)
			conf.fixed_unkill_color = get_color(value);

		if (xcb_xrm_resource_get_string(db, "sanewm.inverted_colors", NULL, &value) >= 0)
			conf.inverted_colors = strcmp(value, "true") == 0;

		if (xcb_xrm_resource_get_string(db, "sanewm.enable_compton", NULL, &value) >= 0)
			conf.enable_compton = strcmp(value, "true") == 0;
	}

	xcb_xrm_database_free(db);

	for (i = 0; i < NUM_ATOMS; ++i)
		ATOM[i] = get_atom(atom_names[i][0]);

	randr_base = setup_randr();

	if (!setup_screen())
		return false;

	if (!setup_keyboard())
		return false;

	xcb_generic_error_t *error = xcb_request_check(conn,
						       xcb_change_window_attributes_checked(conn, screen->root,
											    XCB_CW_EVENT_MASK, values));
	xcb_flush(conn);

	if (error){
		fprintf(stderr, "%s\n","xcb_request_check:faild.");
		free(error);
		return false;
	}
	xcb_ewmh_set_current_desktop(ewmh, scrno, current_workspace);
	xcb_ewmh_set_number_of_desktops(ewmh, scrno, WORKSPACES);

	grab_keys();
	/* set events */
	for (i = 0; i < XCB_NO_OPERATION; ++i)
		events[i] = NULL;

	events[XCB_CONFIGURE_REQUEST]   = configure_request;
	events[XCB_DESTROY_NOTIFY]      = destroy_notify;
	events[XCB_ENTER_NOTIFY]        = enter_notify;
	events[XCB_KEY_PRESS]           = handle_keypress;
	events[XCB_MAP_REQUEST]         = map_request;
	events[XCB_UNMAP_NOTIFY]        = unmap_notify;
	events[XCB_CONFIGURE_NOTIFY]    = config_notify;
	events[XCB_CIRCULATE_REQUEST]   = circulate_request;
	events[XCB_BUTTON_PRESS]        = button_press;
	events[XCB_CLIENT_MESSAGE]      = client_message;

	return true;
}

void
sanewm_restart()
{
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	xcb_ewmh_connection_wipe(ewmh);

	if (ewmh)
		free(ewmh);

	xcb_disconnect(conn);
	execvp(SANEWM_PATH, NULL);
}

void
sig_catch(const int sig)
{
	sig_code = sig;
}

void
install_sig_handlers(void)
{
	struct sigaction sa;
	struct sigaction sa_chld;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP;
	//could not initialize signal handler
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		exit(-1);
	sa.sa_handler = sig_catch;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; /* Restart if interrupted by handler */
	if (sigaction(SIGINT, &sa, NULL) == -1 ||
	    sigaction(SIGHUP, &sa, NULL) == -1 ||
	    sigaction(SIGTERM, &sa, NULL) == -1)
		exit(-1);
}

int
main(int argc, char **argv)
{
	int scrno = 0;
	atexit(cleanup);
	install_sig_handlers();
	if (!xcb_connection_has_error(conn = xcb_connect(NULL, &scrno)))
		if (setup(scrno))
			run();
	/* the WM has stopped running, because sig_code is not 0 */
	exit(sig_code);
}
