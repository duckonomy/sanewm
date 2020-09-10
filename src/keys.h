#ifndef SANEWM_KEYS_H
#define SANEWM_KEYS_H

#include <xcb/xcb_keysyms.h>

bool setup_keyboard(void);
void grab_keys(void);
xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t);
xcb_keysym_t xcb_get_keysym(xcb_keycode_t);

#endif
