/* OS-9 (on 68k) system-dependant routines for editline library.
 *
 * Copyright (c) 1992, 1993  Simmule Turner and Rich Salz. All rights reserved.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or of the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 * 1. The authors are not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits must appear in the documentation.
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits must appear in the documentation.
 * 4. This notice may not be removed or altered.
 */
#include "editline.h"
#include <sgstat.h>
#include <modes.h>

void rl_ttyset(int Reset)
{
    static struct sgbuf	old;
    struct sgbuf	new;

    if (Reset == 0) {
        _gs_opt(0, &old);
        _gs_opt(0, &new);
        new.sg_backsp = 0;	new.sg_delete = 0;	new.sg_echo = 0;
        new.sg_alf = 0;		new.sg_nulls = 0;	new.sg_pause = 0;
        new.sg_page = 0;	new.sg_bspch = 0;	new.sg_dlnch = 0;
        new.sg_eorch = 0;	new.sg_eofch = 0;	new.sg_rlnch = 0;
        new.sg_dulnch = 0;	new.sg_psch = 0;	new.sg_kbich = 0;
        new.sg_kbach = 0;	new.sg_bsech = 0;	new.sg_bellch = 0;
        new.sg_xon = 0;		new.sg_xoff = 0;	new.sg_tabcr = 0;
        new.sg_tabsiz = 0;
        _ss_opt(0, &new);
        rl_erase = old.sg_bspch;
        rl_kill = old.sg_dlnch;
        rl_eof = old.sg_eofch;
        rl_intr = old.sg_kbich;
        rl_quit = -1;
    }
    else {
        _ss_opt(0, &old);
    }
}

void rl_add_slash(char *path, char *p)
{
    strcat(p, access(path, S_IREAD | S_IFDIR) ? " " : "/");
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "ellemtel"
 *  c-basic-offset: 4
 * End:
 */
