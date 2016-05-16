Editline
========
[![Travis Status]][Travis] [![Coverity Status]][Coverity Scan]


Table of Contents
-----------------

* [Introduction](#introduction)
* [API](#api)
* [Example](#example)
* [Build & Install](#build--install)
* [Origin & References](#origin--references)


Introduction
------------

This is a small [line editing][] library.  It can be linked into almost
any program to provide command line editing and history functions.  It
is call compatible with the [FSF readline][] library, but at a fraction
of the size, and as a result fewer features.  It is also distributed
under a much more liberal [LICENSE][].

The small size (<30k), lack of dependencies (no ncurses needed!), and
the free license should make this library interesting to many embedded
developers.

Editline has several optional build-time features that can be enabled by
by supplying different options to the GNU configure script.  See the
output from <kbd>configure --help</kbd> for details.  In the `examples/`
directory you can find some small code snippets used for testing.

Editline is maintained collaboratively at [GitHub][].


API
---

Here is the interface to editline.  It has a small compatibility layer
to [FSF readline][], which may not be entirely up-to-date.

```C
    /* Editline specific global variables. */
    int         el_no_echo;   /* Do not echo input characters */
    int         el_no_hist;   /* Disable auto-save of and access to history,
                               * e.g. for password prompts or wizards */
    int         el_hist_size; /* Size of history scrollback buffer, default: 15 */
    
    /* Editline specific functions. */
    char *      el_find_word(void);
    void        el_print_columns(int ac, char **av);
    el_status_t el_ring_bell(void);
    el_status_t el_del_char(void);
    
    /* Callback function for key binding */
    typedef el_status_t el_keymap_func_t(void);
    
    /* Bind key to a callback, use CTL('f') to change Ctrl-F, for example */
    el_status_t el_bind_key(int key, el_keymap_func_t function);
    el_status_t el_bind_key_in_metamap(int key, el_keymap_func_t function);
    
    /* For compatibility with FSF readline. */
    int         rl_point;
    int         rl_mark;
    int         rl_end;
    int         rl_inhibit_complete;
    char       *rl_line_buffer;
    const char *rl_readline_name;
    
    void rl_initialize(void);
    void rl_reset_terminal(const char *terminal_name);

    void rl_save_prompt(void);
    void rl_restore_prompt(void);
    void rl_set_prompt(const char *prompt);
    
    void rl_clear_message(void);
    void rl_forced_update_display(void);

    /* Main function to use, saves history by default */
    char *readline(const char *prompt);

    /* Use to save a read line to history, when el_no_hist is set */
    void add_history(const char *line);
    
    /* Load and save editline history from/to a file. */
    int read_history(const char *filename);
    int write_history(const char *filename);
    
    /* Magic completion API, see examples/cli.c for more info */
    rl_complete_func_t    *rl_set_complete_func(rl_complete_func_t *func);
    rl_list_possib_func_t *rl_set_list_possib_func(rl_list_possib_func_t *func);
```


Example
-------

Here is a very brief example to illustrate how one can use Editline to
create a simple CLI.  More examples are availble in the source tree.

```C
    #include <stdlib.h>

    extern char *readline(char *prompt);

    int main (void)
    {
        char *p;

        while ((p = readline("CLI> ")) != NULL) {
            puts(p);
            free(p);
        }

        return 0;
    }
```


Build & Install
---------------

Editline was originally designed for older UNIX systems and Plan 9.  The
current maintainer works exclusively on GNU/Linux systems, so it may use
GCC and GNU Make specific extensions here and there.  This is not on
purpose and patches or pull requests to correct this are most welcome!

1. Configure editline with default features: <kbd>./configure</kbd>
2. Build the library and examples: <kbd>make all</kbd>
3. Install using <kbd>make install</kbd>

The `$DESTDIR` environment variable is honored at install.  See
<kbd>./configure --help</kbd> for more options.

Origin & References
--------------------

This [line editing][] library was created by Simmule Turner and Rich Salz
in in 1992.  It is distributed under a “C News-like” license, similar to
the [BSD license][].  For details, see the file [LICENSE][].

This version of the editline library is forked from the [Minix 3][] source
tree.  Patches and bug fixes from the following forks, all based on the
original comp.sources.unix posting, have been merged:

* Debian [libeditline][]
* [Heimdal][]
* [Festival][] speech-tools
* [Steve Tell][]'s editline patches

The version numbering scheme today follows that of the Debian version,
which can be seen in the [ChangeLog.md][].  The Debian version was
unknown to the current [maintainer][] for quite some time, so a
different name and different versioning scheme was used.  In June 2009
this was changed to line up alongside Debian, with the intent is to
eventually merge the efforts.

Outstanding issues are listed in the [TODO.md][] file.

[GitHub]:          https://github.com/troglobit/editline
[line editing]:    https://github.com/troglobit/editline/blob/master/doc/README
[maintainer]:      http://troglobit.com
[LICENSE]:         https://github.com/troglobit/editline/blob/master/LICENSE
[TODO.md]:         https://github.com/troglobit/editline/blob/master/TODO.md
[ChangeLog.md]:    https://github.com/troglobit/editline/blob/master/ChangeLog.md
[FSF readline]:    http://www.gnu.org/software/readline/
[Minix 3]:         http://www.minix3.org/
[BSD license]:     http://en.wikipedia.org/wiki/BSD_licenses
[libeditline]:     http://packages.qa.debian.org/e/editline.html
[Heimdal]:         http://www.h5l.org
[Festival]:        http://festvox.org/festival/
[Steve Tell]:      http://www.cs.unc.edu/~tell/dist.html
[Travis]:          https://travis-ci.org/troglobit/editline
[Travis Status]:   https://travis-ci.org/troglobit/editline.png?branch=master
[Coverity Scan]:   https://scan.coverity.com/projects/2982
[Coverity Status]: https://scan.coverity.com/projects/2982/badge.svg

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
