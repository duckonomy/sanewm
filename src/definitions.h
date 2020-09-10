#ifndef SANEWM_DEFINITIONS_H
#define SANEWM_DEFINITIONS_H

#define WORKSPACES       7
#define BUTTON_MASK      XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
#define NET_WM_FIXED     0xffffffff  // Value in WM hint which means this window is fixed on all workspaces.
#define SANEWM_NOWS      0xfffffffe  // This means we didn't get any window hint at all.
#define LENGTH(x)        (sizeof(x) / sizeof(*x))
#define MIN(X, Y)        ((X) < (Y) ? (X) : (Y))
#define CLEAN_MASK(mask) (mask & ~(num_lock_mask | XCB_MOD_MASK_LOCK))
#define CONTROL          XCB_MOD_MASK_CONTROL /* Control key */
#define ALT              XCB_MOD_MASK_1       /* ALT key */
#define SHIFT            XCB_MOD_MASK_SHIFT   /* Shift key */

enum {SANEWM_MOVE,SANEWM_RESIZE};
enum {SANEWM_FOCUS_NEXT, SANEWM_FOCUS_PREVIOUS};
enum {SANEWM_RESIZE_LEFT, SANEWM_RESIZE_DOWN, SANEWM_RESIZE_UP, SANEWM_RESIZE_RIGHT,SANEWM_RESIZE_LEFT_SLOW, SANEWM_RESIZE_DOWN_SLOW, SANEWM_RESIZE_UP_SLOW, SANEWM_RESIZE_RIGHT_SLOW};
enum {SANEWM_MOVE_LEFT, SANEWM_MOVE_DOWN, SANEWM_MOVE_UP, SANEWM_MOVE_RIGHT,SANEWM_MOVE_LEFT_SLOW, SANEWM_MOVE_DOWN_SLOW, SANEWM_MOVE_UP_SLOW, SANEWM_MOVE_RIGHT_SLOW};
enum {SANEWM_TELEPORT_CENTER_X, SANEWM_TELEPORT_TOP_RIGHT, SANEWM_TELEPORT_BOTTOM_RIGHT,SANEWM_TELEPORT_CENTER, SANEWM_TELEPORT_BOTTOM_LEFT, SANEWM_TELEPORT_TOP_LEFT, SANEWM_TELEPORT_CENTER_Y};
enum {BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT, MIDDLE};
enum {wm_delete_window, wm_change_state, NUM_ATOMS};
enum {SANEWM_RESIZE_KEEP_ASPECT_GROW, SANEWM_RESIZE_KEEP_ASPECT_SHRINK};
enum {SANEWM_MAXIMIZE_HORIZONTALLY, SANEWM_MAXIMIZE_VERTICALLY};
enum {SANEWM_MAX_HALF_FOLD_HORIZONTAL, SANEWM_MAX_HALF_UNFOLD_HORIZONTAL, SANEWM_MAX_HALF_HORIZONTAL_TOP, SANEWM_MAX_HALF_HORIZONTAL_BOTTOM, MAX_HALF_UNUSED,SANEWM_MAX_HALF_VERTICAL_RIGHT, SANEWM_MAX_HALF_VERTICAL_LEFT, SANEWM_MAX_HALF_UNFOLD_VERTICAL, SANEWM_MAX_HALF_FOLD_VERTICAL};
enum {SANEWM_PREVIOUS_SCREEN, SANEWM_NEXT_SCREEN};
enum {SANEWM_CURSOR_UP, SANEWM_CURSOR_DOWN, SANEWM_CURSOR_RIGHT, SANEWM_CURSOR_LEFT,SANEWM_CURSOR_UP_SLOW, SANEWM_CURSOR_DOWN_SLOW, SANEWM_CURSOR_RIGHT_SLOW, SANEWM_CURSOR_LEFT_SLOW};
enum {SANEWM_MAX_HALF_HALF_TOP_LEFT,SANEWM_MAX_HALF_HALF_BOTTOM_LEFT,SANEWM_MAX_HALF_HALF_TOP_RIGHT,SANEWM_MAX_HALF_HALF_BOTTOM_RIGHT,SANEWM_MAX_HALF_HALF_CENTER};

#endif