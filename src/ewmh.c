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
