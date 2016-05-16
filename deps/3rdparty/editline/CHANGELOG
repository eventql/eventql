Change Log
==========

All notable changes to the project are documented in this file.

[1.15.0][] - 2015-09-10
-----------------------

### Changes
- Add support for `--disable-eof` and `--disable-sigint` to disable
  default Ctrl-D and Ctrl-C behavior
- Add support for `el_no_hist` to disable access to and auto-save of history
- GNU readline compat functions for prompt handling and redisplay
- Refactor: replace variables named 'new' with non-reserved word
- Add support for [Travis-CI][], continuous integration with GitHub
- Add support for [Coverity Scan][], the best static code analyzer,
  integrated with [Travis-CI][] -- scan runs for each push to master
- Rename NEWS.md --> ChangeLog.md, with symlinks for <kbd>make install</kbd>
- Attempt to align with http://keepachangelog.com/ for this file
- Cleanup and improve Markdown syntax in [README.md][]
- Add API and example to [README.md][], inspired by [libuEv][]
- Removed generated files from version control.  Use `./autogen.sh`
  to generate the `configure` script when working from GIT.  This
  does not affect distributed tarballs

### Fixes
- Fix issue #2, regression in Ctrl-D (EOF) behavior.  Regression
  introduced in [1.14.1][].  Fixed by @TobyGoodwin
- Fix memory leak in completion handler.  Found by [Coverity Scan][].
- Fix suspicious use of `sizeof(char **)`, same as `sizeof(char *)` but
  non-portable.  Found by [Coverity Scan][]
- Fix out-of-bounds access in user key binding routines
  Found by [Coverity Scan][].
- Fix invisible example code in man page


[1.14.2][] - 2014-09-14
-----------------------

Bug fixes only.

### Fixes
  - Fix `el_no_echo` bug causing secrets to leak when disabling no-echo
  - Handle `EINTR` in syscalls better


[1.14.1][] - 2014-09-14
-----------------------

Minor fixes and additions.

### Changes
- Don't print status message on `stderr` in key binding funcions
- Export `el_del_char()`
- Check for and return pending signals when detected
- Allow custom key bindings ...

### Fixes
- Bug fixes ...


[1.14.0][] - 2010-08-10
-----------------------

Major cleanups and further merges with Debian editline package.

### Changes
- Merge in changes to `debian/` from `editline_1.12-6.debian.tar.gz`
- Migrate to use libtool
- Make `UNIQUE_HISTORY` configurable
- Make scrollback history (`HIST_SIZE`) configurable
- Configure options for toggling terminal bell and `SIGSTOP` (Ctrl-Z)
- Configure option for using termcap to read/control terminal size
- Rename Signal to `el_intr_pending`, from Festival speech-tools
- Merge support for capitalizing words (`M-c`) from Festival
  speech-tools by Alan W Black <mailto:awb()cstr!ed!ac!uk>
- Fallback backspace handling, in case `tgetstr("le")` fails

### Fixes
- Cleanups and fixes thanks to the Sparse static code analysis tool
- Merge `el_no_echo` patch from Festival speech-tools
- Merge fixes from Heimdal project
- Completely refactor `rl_complete()` and `rl_list_possib()` with
  fixes from the Heimdal project.  Use `rl_set_complete_func()` and
  `rl_set_list_possib_func()`.  Default completion callbacks are now
  available as a configure option `--enable-default-complete`
- Memory leak fixes
- Actually fix 8-bit handling by reverting old Debian patch
- Merge patch to improve compatibility with GNU readline, thanks to
  Steve Tell from way back in 1997 and 1998


[1.13.0][] - 2010-03-09
-----------------------

Adaptations to Debian editline package.

### Changes
- Major version number bump, adapt to Jim Studt's v1.12
- Import `debian/` directory and adapt it to configure et al.
- Change library name to libeditline to distinguish it from BSD libedit


[0.3.0][] - 2009-02-08
----------------------

### Changes
- Support for ANSI arrow keys using <kbd>configure --enable-arrow-keys</kbd>


[0.2.3][] - 2008-12-02
----------------------

### Changes
- Patches from Debian package merged
- Support for custom command completion


[0.1.0][] - 2008-06-07
----------------------

### Changes
- First version, forked from Minix current 2008-06-06


[UNRELEASED]:    https://github.com/troglobit/finit/compare/1.15.0...HEAD
[1.15.0]:        https://github.com/troglobit/finit/compare/1.14.2...1.15.0
[1.14.2]:        https://github.com/troglobit/finit/compare/1.14.1...1.14.2
[1.14.1]:        https://github.com/troglobit/finit/compare/1.14.0...1.14.1
[1.14.0]:        https://github.com/troglobit/finit/compare/1.13.0...1.14.0
[1.13.0]:        https://github.com/troglobit/finit/compare/0.3.0...1.13.0
[0.3.0]:         https://github.com/troglobit/finit/compare/0.2.3...0.3.0
[0.2.3]:         https://github.com/troglobit/finit/compare/0.1.0...0.2.3
[0.1.0]:         https://github.com/troglobit/finit/compare/0.0.0...0.1.0
[libuEv]:        http://github.com/troglobit/libuev
[Travis-CI]:     https://travis-ci.org/troglobit/uftpd
[Coverity Scan]: https://scan.coverity.com/projects/2947
[README.md]:     https://github.com/troglobit/editline/blob/master/README.md

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
