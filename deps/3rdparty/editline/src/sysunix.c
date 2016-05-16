/* Unix system-dependant routines for editline library.
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

#include <errno.h>
#include "editline.h"

#ifndef HAVE_TCGETATTR
/* Wrapper for ioctl syscalls to restart on signal */
static int ioctl_wrap(int fd, int req, void *arg)
{
    int result, retries = 3;

    while (-1 == (result = ioctl(fd, req, arg)) && retries > 0) {
	retries--;

	if (EINTR == errno)
	    continue;

	break;
    }

    return result;
}
#endif

/* Prefer termios over the others since it is likely the most portable. */
#if defined(HAVE_TCGETATTR)
#include <termios.h>

/* Wrapper for tcgetattr */
static int getattr(int fd, struct termios *arg)
{
    int result, retries = 3;

    while (-1 == (result = tcgetattr(fd, arg)) && retries > 0) {
	retries--;

	if (EINTR == errno)
	    continue;

	break;
    }

    return result;
}

/* Wrapper for tcgetattr */
static int setattr(int fd, int opt, const struct termios *arg)
{
    int result, retries = 3;

    while (-1 == (result = tcsetattr(fd, opt, arg)) && retries > 0) {
	retries--;

	if (EINTR == errno)
	    continue;

	break;
    }

    return result;
}

void rl_ttyset(int Reset)
{
    static struct termios       old;
    struct termios              new;

    if (!Reset) {
        if (-1 == getattr(0, &old))
	    perror("Failed tcgetattr()");
        rl_erase = old.c_cc[VERASE];
        rl_kill = old.c_cc[VKILL];
        rl_eof = old.c_cc[VEOF];
        rl_intr = old.c_cc[VINTR];
        rl_quit = old.c_cc[VQUIT];
#ifdef CONFIG_SIGSTOP
        rl_susp = old.c_cc[VSUSP];
#endif

        new = old;
        new.c_lflag &= ~(ECHO | ICANON | ISIG);
        new.c_iflag &= ~INPCK;
	if (rl_meta_chars)
	    new.c_iflag |= ISTRIP;
	else
	    new.c_iflag &= ~ISTRIP;
        new.c_cc[VMIN] = 1;
        new.c_cc[VTIME] = 0;
        if (-1 == setattr(0, TCSADRAIN, &new))
	    perror("Failed tcsetattr(TCSADRAIN)");
    } else {
        if (-1 == setattr(0, TCSADRAIN, &old))
	    perror("Failed tcsetattr(TCSADRAIN)");
    }
}

#elif defined(HAVE_TERMIO_H)
#include <termio.h>

void rl_ttyset(int Reset)
{
    static struct termio        old;
    struct termio               new;

    if (!Reset) {
	if (-1 == ioctl_wrap(0, TCGETA, &old))
	    perror("Failed ioctl(TCGETA)");
        rl_erase = old.c_cc[VERASE];
        rl_kill = old.c_cc[VKILL];
        rl_eof = old.c_cc[VEOF];
        rl_intr = old.c_cc[VINTR];
        rl_quit = old.c_cc[VQUIT];
#ifdef CONFIG_SIGSTOP
        rl_susp = old.c_cc[VSUSP];
#endif

        new = old;
        new.c_lflag &= ~(ECHO | ICANON | ISIG);
        new.c_iflag &= ~INPCK;
	if (rl_meta_chars)
	    new.c_iflag |= ISTRIP;
	else
	    new.c_iflag &= ~ISTRIP;

        new.c_cc[VMIN] = 1;
        new.c_cc[VTIME] = 0;
        if (-1 == ioctl_wrap(0, TCSETAW, &new))
	    perror("Failed ioctl(TCSETAW)");
    } else {
	if (-1 == ioctl_wrap(0, TCSETAW, &old))
	    perror("Failed ioctl(TCSETAW)");
    }
}

#elif defined(HAVE_SGTTY_H)
#include <sgtty.h>

void rl_ttyset(int Reset)
{
    static struct sgttyb        old_sgttyb;
    static struct tchars        old_tchars;
    struct sgttyb               new_sgttyb;
    struct tchars               new_tchars;
#ifdef CONFIG_SIGSTOP
    struct ltchars              old_ltchars;
#endif

    if (!Reset) {
        if (-1 == ioctl_wrap(0, TIOCGETP, &old_sgttyb))
	    perror("Failed TIOCGETP");
        rl_erase = old_sgttyb.sg_erase;
        rl_kill = old_sgttyb.sg_kill;

        if (-1 == ioctl_wrap(0, TIOCGETC, &old_tchars))
	    perror("Failed TIOCGETC");
        rl_eof = old_tchars.t_eofc;
        rl_intr = old_tchars.t_intrc;
        rl_quit = old_tchars.t_quitc;

#ifdef CONFIG_SIGSTOP
        if (-1 == ioctl_wrap(0, TIOCGLTC, &old_ltchars))
	    perror("Failed TIOCGLTC");
        rl_susp = old_ltchars.t_suspc;
#endif

        new_sgttyb = old_sgttyb;
        new_sgttyb.sg_flags &= ~ECHO;
        new_sgttyb.sg_flags |= RAW;
	if (rl_meta_chars)
	    new_sgttyb.sg_flags &= ~PASS8;
	else
	    new_sgttyb.sg_flags |= PASS8;
        if (-1 == ioctl_wrap(0, TIOCSETP, &new_sgttyb))
	    perror("Failed TIOCSETP");
        new_tchars = old_tchars;
        new_tchars.t_intrc = -1;
        new_tchars.t_quitc = -1;
        if (-1 == ioctl_wrap(0, TIOCSETC, &new_tchars))
	    perror("Failed TIOCSETC");
    } else {
        if (-1 == ioctl_wrap(0, TIOCSETP, &old_sgttyb))
	    perror("Failed TIOCSETP");
        if (-1 == ioctl_wrap(0, TIOCSETC, &old_tchars))
	    perror("Failed TIOCSETC");
    }
}
#else /* Neither HAVE_SGTTY_H, HAVE_TERMIO_H or HAVE_TCGETATTR */
#error Unsupported platform, missing tcgetattr(), termio.h and sgtty.h
#endif /* Neither HAVE_SGTTY_H, HAVE_TERMIO_H or HAVE_TCGETATTR */

#ifndef HAVE_STRDUP
/* Return an allocated copy of a string. */
char *strdup(const char *p)
{
    char *new = malloc(sizeof(char) * strlen(p));

    if (new) {
        strcpy(new, p);
	return new;
    }

    return NULL;
}
#endif

void rl_add_slash(char *path, char *p)
{
    struct stat Sb;

    if (stat(path, &Sb) >= 0)
        strcat(p, S_ISDIR(Sb.st_mode) ? "/" : " ");
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "ellemtel"
 *  c-basic-offset: 4
 * End:
 */
