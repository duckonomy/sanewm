#ifndef SANEWM_WINDOW_H
#define SANEWM_WINDOW_H

#include <xcb/xcb.h>

#include "sanewm.h"

void fix_current_window();
void unkillable_current_window();
void raise_current_window(void);
void raise_or_lower(void);
void raise_window(xcb_drawable_t);
void set_unfocus_window(void);
// Is this screen-related?
void no_window_border(int16_t *, struct sane_window *, bool);
void maximize_current_window(const Arg *);
void fullscreen_window(const Arg *);
void unmaximize_window(struct sane_window *);
void maximize_window(struct sane_window *, uint8_t);
void maximize_window_helper(struct sane_window *,uint16_t, uint16_t, uint16_t, uint16_t);
/* void hide(void); */
void focus_next_window(const Arg *);
void update_window_list(void);
struct sane_window *setup_window(xcb_window_t);
void unkillable_window(struct sane_window *);
void fix_window(struct sane_window *);
void focus_next_window_helper(bool);
void arrange_windows(void);
void delete_window();
void check_window_name(struct sane_window *);
bool check_window_state_unkill(xcb_drawable_t);
void always_on_top_window();
void move_window_limit(struct sane_window *);
void move_window(xcb_drawable_t, const int16_t, const int16_t);
struct sane_window *find_window(const xcb_drawable_t *);
void focus_window(struct sane_window *);
void resize_window_limit(struct sane_window *);
void resize_window(xcb_drawable_t, const uint16_t, const uint16_t);
void move_resize_window(xcb_drawable_t, const uint16_t, const uint16_t, const uint16_t, const uint16_t);
void set_window_borders(struct sane_window *, const bool);
void unmaximize_window_size(struct sane_window *);
bool get_window_geometry(const xcb_drawable_t *, int16_t *, int16_t *, uint16_t *, uint16_t *, uint8_t *);
void snap_window(struct sane_window *);
void forget_window(struct sane_window *);
void forget_window_id(xcb_window_t);
void max_half_window(const Arg *);
void resize_step_window(const Arg *);
void resize_step_aspect_window(const Arg *);
void move_step_window(const Arg *);
void maximize_vertical_horizontal_window(const Arg *);
void max_half_half_window(const Arg *);
void teleport_window(const Arg *);
void add_to_window_list(const xcb_drawable_t);
struct sane_window create_back_window(void);
void fit_window_on_screen(struct sane_window *);
void save_original_window_geometry(struct sane_window *);
bool setup_screen(void);

#endif
