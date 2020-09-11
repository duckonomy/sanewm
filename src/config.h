/* Copyright (c) 2017-2020 Ian Park
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SANEWM_CONFIG_H
#define SANEWM_CONFIG_H

#include <inttypes.h>

#include "types.h"
#include "definitions.h"
#include "sanewm.h"
#include "window.h"
#include "events.h"
#include "pointer.h"
#include "desktop.h"
#include "monitor.h"

///---User configurable stuff---///
///---Modifiers---///
#define MOD             XCB_MOD_MASK_4       /* Super/Windows key  or check xmodmap(1) with -pm  defined in /usr/include/xcb/xproto.h */
///--Speed---///
/* Move this many pixels when moving or resizing with keyboard unless the window has hints saying otherwise.
 *0)move step slow   1)move step fast
 *2)mouse slow       3)mouse fast     */
static const uint16_t movements[] = {20,40,15,400};
/* resize by line like in mcwm -- jmbi */
static const bool     resize_by_line          = true;
/* the ratio used when resizing and keeping the aspect */
static const float    resize_keep_aspect_ratio= 1.03;
///---Offsets---///
/*0)offset_x          1)offset_y
 *2)max_width         3)max_height */
static const uint8_t offsets[] = {0,38,0,38};
///---Colors---///
/*0)focuscol         1)unfocuscol
 *2)fixedcol         3)unkilcol
 *4)fixedunkilcol    5)outerbordercol
 *6)emptycol         */
static const char *colors[] = {"#8CBCDD","#313539","#7a8c5c","#ff6666","#cc9933","#313539","#313539"};
/* if this is set to true the inner border and outer borders colors will be swapped */
static const bool inverted_colors = true;
///---Cursor---///
/* default position of the cursor:
 * correct values are:
 * TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, MIDDLE
 * All these are relative to the current window. */
#define CURSOR_POSITION MIDDLE
///---Borders---///
/*0) Outer border size. If you put this negative it will be a square.
 *1) Full borderwidth    2) Magnet border size
 *3) Resize border size  */
static const uint8_t borders[] = {10,10,10,0};
/* Windows that won't have a border.
 * It uses substring comparison with what is found in the WM_NAME
 * attribute of the window. You can test this using `xprop WM_NAME`
 */
#define LOOK_INTO "WM_NAME"
static const char *ignore_names[] = {"polybar", "xclock"};
///--Menus and Programs---///
static const char *menu_cmd[]   = { "$HOME/.bin/runrofi", NULL };
///--Custom foo---///
static void half_and_centered(const Arg *arg)
{
	Arg arg2 = {.i = SANEWM_MAX_HALF_VERTICAL_LEFT};
	max_half_window(&arg2);
	Arg arg3 = {.i = SANEWM_TELEPORT_CENTER};
	teleport_window(&arg3);
}
///---Sloppy focus behavior---///
/*
 * Command to execute when switching from sloppy focus to click to focus
 * The strings "Sloppy" and "Click" will be passed as the last argument
 * If NULL this is ignored
 */
static const char *sloppy_switch_cmd[] = {};
//static const char *sloppy_switch_cmd[] = { "notify-send", "toggle sloppy", NULL };
static void toggle_sloppy(const Arg *arg)
{
	is_sloppy = !is_sloppy;
	if (arg->com != NULL && LENGTH(arg->com) > 0) {
		start(arg);
	}
}
///---Shortcuts---///
/* Check /usr/include/X11/keysymdef.h for the list of all keys
 * 0x000000 is for no modkey
 * If you are having trouble finding the right keycode use the `xev` to get it
 * For example:
 * KeyRelease event, serial 40, synthetic NO, window 0x1e00001,
 *  root 0x98, subw 0x0, time 211120530, (128,73), root:(855,214),
 *  state 0x10, keycode 171 (keysym 0x1008ff17, XF86AudioNext), same_screen YES,
 *  XLookupString gives 0 bytes:
 *  XFilterEvent returns: False
 *
 *  The keycode here is keysym 0x1008ff17, so use  0x1008ff17
 *
 *
 * For AZERTY keyboards XK_1...0 should be replaced by :
 *      DESKTOP_CHANGE(     XK_ampersand,                     0)
 *      DESKTOP_CHANGE(     XK_eacute,                        1)
 *      DESKTOP_CHANGE(     XK_quotedbl,                      2)
 *      DESKTOP_CHANGE(     XK_apostrophe,                    3)
 *      DESKTOP_CHANGE(     XK_parenleft,                     4)
 *      DESKTOP_CHANGE(     XK_minus,                         5)
 *      DESKTOP_CHANGE(     XK_egrave,                        6)
 *      DESKTOP_CHANGE(     XK_underscore,                    7)
 *      DESKTOP_CHANGE(     XK_ccedilla,                      8)
 *      DESKTOP_CHANGE(     XK_agrave,                        9)*
 */
#define DESKTOP_CHANGE(K,N)						     \
	{  MOD ,             K,			change_workspace, {.i = N}}, \
	{  MOD | SHIFT,      K,			send_to_workspace, {.i = N}},

static key keys[] = {
	/* modifier           key            function           argument */
	// Focus to next/previous window
	{  MOD ,               XK_Tab,        focus_next_window,           {.i = SANEWM_FOCUS_NEXT}},
	{  MOD | SHIFT,        XK_Tab,        focus_next_window,           {.i = SANEWM_FOCUS_PREVIOUS}},
	// Kill a window
	{  MOD ,               XK_c,          delete_window,               {}},
	{  MOD | SHIFT,        XK_c,          kill_window,                 {}},
	// Resize a window
	{  MOD | SHIFT,        XK_k,          resize_step_window,          {.i = SANEWM_RESIZE_UP}},
	{  MOD | SHIFT,        XK_j,          resize_step_window,          {.i = SANEWM_RESIZE_DOWN}},
	{  MOD | SHIFT,        XK_l,          resize_step_window,          {.i = SANEWM_RESIZE_RIGHT}},
	{  MOD | SHIFT,        XK_h,          resize_step_window,          {.i = SANEWM_RESIZE_LEFT}},
	// Resize a window slower
	{  MOD | SHIFT | CONTROL, XK_k,          resize_step_window,       {.i = SANEWM_RESIZE_UP_SLOW}},
	{  MOD | SHIFT | CONTROL, XK_j,          resize_step_window,       {.i = SANEWM_RESIZE_DOWN_SLOW}},
	{  MOD | SHIFT | CONTROL, XK_l,          resize_step_window,       {.i = SANEWM_RESIZE_RIGHT_SLOW}},
	{  MOD | SHIFT | CONTROL, XK_h,          resize_step_window,       {.i = SANEWM_RESIZE_LEFT_SLOW}},
	// Move a window
	{  MOD ,              XK_k,          move_step_window,             {.i = SANEWM_MOVE_UP}},
	{  MOD ,              XK_j,          move_step_window,             {.i = SANEWM_MOVE_DOWN}},
	{  MOD ,              XK_l,          move_step_window,             {.i = SANEWM_MOVE_RIGHT}},
	{  MOD ,              XK_h,          move_step_window,             {.i = SANEWM_MOVE_LEFT}},
	// Move a window slower
	{  MOD | CONTROL,      XK_k,          move_step_window,            {.i = SANEWM_MOVE_UP_SLOW}},
	{  MOD | CONTROL,      XK_j,          move_step_window,            {.i = SANEWM_MOVE_DOWN_SLOW}},
	{  MOD | CONTROL,      XK_l,          move_step_window,            {.i = SANEWM_MOVE_RIGHT_SLOW}},
	{  MOD | CONTROL,      XK_h,          move_step_window,            {.i = SANEWM_MOVE_LEFT_SLOW}},
	// Teleport the window to an area of the screen.
	{  MOD | SHIFT,        XK_g,          teleport_window,             {.i = SANEWM_TELEPORT_CENTER_Y}},
	// Center x:
	{  MOD | CONTROL,      XK_g,          teleport_window,             {.i = SANEWM_TELEPORT_CENTER_X}},
	// Top left:
	{  MOD,              XK_y,          max_half_half_window,          {.i = SANEWM_MAX_HALF_HALF_TOP_LEFT}},
	// Top right:
	{  MOD,              XK_o,          max_half_half_window,          {.i = SANEWM_MAX_HALF_HALF_TOP_RIGHT}},
	// Bottom left:
	{  MOD,              XK_u,          max_half_half_window,          {.i = SANEWM_MAX_HALF_HALF_BOTTOM_LEFT}},
	// Bottom right:
	{  MOD,              XK_i,          max_half_half_window,          {.i = SANEWM_MAX_HALF_HALF_BOTTOM_RIGHT}},
	// Center
	{  MOD,              XK_g,          max_half_half_window,          {.i = SANEWM_MAX_HALF_HALF_CENTER}},
	// Resize while keeping the window aspect
	{  MOD,              XK_Home,       resize_step_aspect_window,     {.i = SANEWM_RESIZE_KEEP_ASPECT_GROW}},
	{  MOD,              XK_End,        resize_step_aspect_window,     {.i = SANEWM_RESIZE_KEEP_ASPECT_SHRINK}},
	// Maximize (ignore offset and no EWMH atom)
	{  MOD,              XK_a,          maximize_current_window,       {}},
	// Full screen (disregarding offsets and adding EWMH atom)
	{  MOD,              XK_f,          fullscreen_window,             {}},
	// vertically left
	{  MOD | SHIFT,        XK_y,          max_half_window,             {.i = SANEWM_MAX_HALF_VERTICAL_LEFT}},
	// vertically right
	{  MOD | SHIFT,        XK_o,          max_half_window,             {.i = SANEWM_MAX_HALF_VERTICAL_RIGHT}},
	// horizontally left
	{  MOD | SHIFT,        XK_u,          max_half_window,             {.i = SANEWM_MAX_HALF_HORIZONTAL_BOTTOM}},
	// horizontally right
	{  MOD | SHIFT,        XK_i,          max_half_window,             {.i = SANEWM_MAX_HALF_HORIZONTAL_TOP}},
	// Next/Previous screen
	{  MOD,                XK_m,      change_monitor,                  {.i = SANEWM_NEXT_SCREEN}},
	{  MOD | SHIFT,        XK_m,      send_to_monitor,                 {.i = SANEWM_NEXT_SCREEN}},
	// Next/Previous workspace
	{  MOD,              XK_bracketright,          next_workspace,     {}},
	{  MOD,              XK_bracketleft,           previous_workspace, {}},
	// Move to Next/Previous workspace
	{  MOD | SHIFT,       XK_bracketright,          send_to_next_workspace,    {}},
	{  MOD | SHIFT,       XK_bracketleft,           send_to_previous_workspace,{}},
	// Make the window unkillable
	{  MOD | SHIFT,        XK_t,          unkillable_current_window, {.i = 0}},
	// Make the window appear always on top
	{  MOD,                XK_t,          always_on_top_window,      {.i = 0}},
	// Make the window stay on all workspaces
	{  MOD | SHIFT,        XK_f,          fix_current_window,        {.i = 0}},
	// Move the cursor
	{  MOD,                XK_Up,         cursor_move,       {.i = SANEWM_CURSOR_UP_SLOW}},
	{  MOD,                XK_Down,       cursor_move,       {.i = SANEWM_CURSOR_DOWN_SLOW}},
	{  MOD,                XK_Right,      cursor_move,       {.i = SANEWM_CURSOR_RIGHT_SLOW}},
	{  MOD,                XK_Left,       cursor_move,       {.i = SANEWM_CURSOR_LEFT_SLOW}},
	// Move the cursor faster
	{  MOD | SHIFT,        XK_Up,         cursor_move,       {.i = SANEWM_CURSOR_UP}},
	{  MOD | SHIFT,        XK_Down,       cursor_move,       {.i = SANEWM_CURSOR_DOWN}},
	{  MOD | SHIFT,        XK_Right,      cursor_move,       {.i = SANEWM_CURSOR_RIGHT}},
	{  MOD | SHIFT,        XK_Left,       cursor_move,       {.i = SANEWM_CURSOR_LEFT}},
	// Exit or restart sanewm
	{  MOD | SHIFT,        XK_q,          sanewm_exit,       {.i = 0}},
	{  MOD | CONTROL,      XK_r,          sanewm_restart,    {.i = 0}},
	{  MOD ,               XK_space,      half_and_centered, {.i = 0}},
	{  MOD | SHIFT,        XK_space,      toggle_sloppy,     {.com = sloppy_switch_cmd}},
	// Change current workspace
	DESKTOP_CHANGE(     XK_1,                             0)
	DESKTOP_CHANGE(     XK_2,                             1)
	DESKTOP_CHANGE(     XK_3,                             2)
	DESKTOP_CHANGE(     XK_4,                             3)
	DESKTOP_CHANGE(     XK_5,                             4)
	DESKTOP_CHANGE(     XK_6,                             5)
	DESKTOP_CHANGE(     XK_7,                             6)
	DESKTOP_CHANGE(     XK_8,                             7)
	DESKTOP_CHANGE(     XK_9,                             8)
	DESKTOP_CHANGE(     XK_0,                             9)
};

// the last argument makes it a root window only event
static Button buttons[] = {
	{  MOD        , XCB_BUTTON_INDEX_1, mouse_motion,     {.i = SANEWM_MOVE}, false},
	{  MOD        , XCB_BUTTON_INDEX_3, mouse_motion,     {.i = SANEWM_RESIZE}, false},
	{  MOD | SHIFT, XCB_BUTTON_INDEX_1, change_workspace, {.i = 0}, false},
	{  MOD | SHIFT, XCB_BUTTON_INDEX_3, change_workspace, {.i = 1}, false},
	{  MOD | ALT  , XCB_BUTTON_INDEX_1, send_to_monitor,  {.i = 1}, false},
	{  MOD | ALT  , XCB_BUTTON_INDEX_3, send_to_monitor,  {.i = 0}, false}
};

#endif
