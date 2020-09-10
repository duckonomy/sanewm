#ifndef SANEWM_WINDOW_H
#define SANEWM_WINDOW_H

#include <xcb/xcb.h>

#include "sanewm.h"

void fix();
void unkillable();
void raise_current_window(void);
void raise_or_lower(void);
void raise_window(xcb_drawable_t);
void set_unfocus(void);
void no_border(int16_t *, struct sane_window *, bool);
void maximize(const Arg *);
void fullscreen(const Arg *);
void unmaximize_window(struct sane_window *);
void maximize_window(struct sane_window *, uint8_t);
void maximize_helper(struct sane_window *,uint16_t, uint16_t, uint16_t, uint16_t);
/* void hide(void); */
void focus_next(const Arg *);
void update_window_list(void);
struct sane_window *setup_window(xcb_window_t);
void unkillable_window(struct sane_window *);
void fix_window(struct sane_window *);
void focus_next_helper(bool);
void arrange_windows(void);
void delete_window();
void check_name(struct sane_window *);
bool get_unkill_state(xcb_drawable_t);
void always_on_top();
void move_limit(struct sane_window *);
void move_window(xcb_drawable_t, const int16_t, const int16_t);
struct sane_window *find_window(const xcb_drawable_t *);
void focus_window(struct sane_window *);
void resize_limit(struct sane_window *);
void resize(xcb_drawable_t, const uint16_t, const uint16_t);
void move_resize(xcb_drawable_t, const uint16_t, const uint16_t, const uint16_t, const uint16_t);
void set_borders(struct sane_window *, const bool);
void unmaximize_window_sizeimize_window_size(struct sane_window *);
bool get_geometry(const xcb_drawable_t *, int16_t *, int16_t *, uint16_t *, uint16_t *, uint8_t *);
void snap_window(struct sane_window *);
void forget_window(struct sane_window *);
void forget_window_id(xcb_window_t);
void max_half(const Arg *);
void resize_step(const Arg *);
void resize_step_aspect(const Arg *);
void move_step(const Arg *);
void maximize_vertical_horizontal(const Arg *);
void max_half_half(const Arg *);
void teleport(const Arg *);
void add_to_window_list(const xcb_drawable_t);
struct sane_window create_back_window(void);
void fit_on_screen(struct sane_window *);
void save_original_geometry(struct sane_window *);
bool setup_screen(void);

#endif
