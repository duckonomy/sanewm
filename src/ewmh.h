#ifndef SANEWM_EWMH_H
#define SANEWM_EWMH_H

#include <xcb/xcb.h>

void ewmh_init(void);
xcb_atom_t get_atom(const char *atom_name);

#endif
