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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ewmh.h"
#include "sanewm.h"

void
ewmh_init(void)
{
	if (!(ewmh = calloc(1, sizeof(xcb_ewmh_connection_t))))
		printf("Fail\n");

	xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, ewmh);
	if (!xcb_ewmh_init_atoms_replies(ewmh, cookie, (void *)0)){
		fprintf(stderr, "%s\n", "xcb_ewmh_init_atoms_replies:faild.");
		exit(EXIT_FAILURE);
	}
}


/* Get a defined atom from the X server. */
xcb_atom_t
get_atom(const char *atom_name)
{
	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(conn, 0,
							       strlen(atom_name), atom_name);

	xcb_intern_atom_reply_t *rep = xcb_intern_atom_reply(conn, atom_cookie,
							     NULL);

	/* XXX Note that we return 0 as an atom if anything goes wrong.
	 * Might become interesting.*/

	if (NULL == rep)
		return 0;

	xcb_atom_t atom = rep->atom;

	free(rep);
	return atom;
}
