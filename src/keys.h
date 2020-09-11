#ifndef SANEWM_KEYS_H
#define SANEWM_KEYS_H

#include <xcb/xcb_keysyms.h>

bool setup_keyboard(void);
void grab_keys(void);
xcb_keycode_t *keysym_to_keycode(xcb_keysym_t);
xcb_keysym_t keycode_to_keysym(xcb_keycode_t);

#endif
