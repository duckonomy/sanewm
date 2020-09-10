#ifndef SANEWM_POINTER_H
#define SANEWM_POINTER_H

#include <xcb/xcb.h>
#include "types.h"

xcb_cursor_t create_font_cursor(xcb_connection_t *, uint16_t);
void cursor_move(const Arg *);
bool get_pointer(const xcb_drawable_t *, int16_t *, int16_t *);
void mouse_resize(struct sane_window *, const int16_t, const int16_t);
void mouse_move(const int16_t, const int16_t);
void move_pointer_back(const int16_t, const int16_t, const struct sane_window *);
void center_pointer(xcb_drawable_t, struct sane_window *);
void grab_buttons(struct sane_window *);

#endif
