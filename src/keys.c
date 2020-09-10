#include <xcb/xcb_keysyms.h>
#include <stdbool.h>

#include "keys.h"
#include "sanewm.h"
#include "config.h"

bool
setup_keyboard(void)
{
	xcb_get_modifier_mapping_reply_t *reply;
	xcb_keycode_t *modmap, *num_lock;
	unsigned int i, j, n;

	reply = xcb_get_modifier_mapping_reply(conn,
					       xcb_get_modifier_mapping_unchecked(conn), NULL);

	if (!reply)
		return false;

	modmap = xcb_get_modifier_mapping_keycodes(reply);

	if (!modmap)
		return false;

	num_lock = xcb_get_keycodes(XK_Num_Lock);

	for (i = 4; i < 8; ++i) {
		for (j = 0; j < reply->keycodes_per_modifier; ++j) {
			xcb_keycode_t keycode = modmap[i
						       * reply->keycodes_per_modifier + j];

			if (keycode == XCB_NO_SYMBOL)
				continue;

			if (num_lock != NULL)
				for (n = 0; num_lock[n] != XCB_NO_SYMBOL; ++n)
					if (num_lock[n] == keycode) {
						num_lock_mask = 1 << i;
						break;
					}
		}
	}

	free(reply);
	free(num_lock);

	return true;
}

/* the wm should listen to key presses */
void
grab_keys(void)
{
	xcb_keycode_t *keycode;
	int i, k, m;
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		num_lock_mask,
		num_lock_mask | XCB_MOD_MASK_LOCK
	};

	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	for (i = 0; i < LENGTH(keys); ++i) {
		keycode = xcb_get_keycodes(keys[i].keysym);

		for (k = 0; keycode[k] != XCB_NO_SYMBOL; ++k)
			for (m = 0; m < LENGTH(modifiers); ++m)
				xcb_grab_key(conn, 1, screen->root, keys[i].mod
					     | modifiers[m], keycode[k],
					     XCB_GRAB_MODE_ASYNC, // pointer mode
					     XCB_GRAB_MODE_ASYNC); // keyboard mode
		free(keycode); // allocated in xcb_get_keycodes()
	}
}
/* wrapper to get xcb keycodes from keysymbol */
xcb_keycode_t *
xcb_get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t *keysyms;
	xcb_keycode_t *keycode;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return NULL;

	keycode = xcb_key_symbols_get_keycode(keysyms, keysym);
	xcb_key_symbols_free(keysyms);

	return keycode;
}

/* wrapper to get xcb keysymbol from keycode */
xcb_keysym_t
xcb_get_keysym(xcb_keycode_t keycode)
{
	xcb_key_symbols_t *keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return 0;

	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);
	xcb_key_symbols_free(keysyms);

	return keysym;
}
