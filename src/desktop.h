#ifndef SANEWM_DESKTOP_H
#define SANEWM_DESKTOP_H

#include <inttypes.h>

#include "types.h"

void delete_from_workspace(struct sane_window *);
void change_workspace(const Arg *);
void next_workspace();
void previous_workspace();
void add_to_workspace(struct sane_window *, uint32_t);
void change_workspace_helper(const uint32_t);
void send_to_workspace(const Arg *);
void send_to_next_workspace(const Arg *);
void send_to_previous_workspace(const Arg *);
uint32_t get_wm_desktop(xcb_drawable_t);

#endif
