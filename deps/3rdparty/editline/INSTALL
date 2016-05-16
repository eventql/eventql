HowTo Build Minix Editline
==========================

Minix editline use the GNU configure tools, which includes autoconf,
automake and libtool.  This enables high levels of portability and ease
of use.  In most cases all you need to do is unpack the tarball, enter
the directory and type:

    ./configure

There are are, however, more options available.  For instance, sometimes
it is useful to build editline as a static library, type:

    ./configure --disable-shared

By default editline employs a default handler for the TAB key, pressing
it once completes to the nearest matching filename in the current
working directory, or it can display a listing of possible completions.
For some uses this default is not desirable at all, type:

    ./configure --disable-default-complete

An even more common desire is to change the default install location.
By default all configure scripts setup /usr/local as the install
"prefix", to change this type:

    ./configure --prefix=/home/troglobit/tmp

Advanced users are encouraged to read up on --libdir, --mandir, etc. in
the GNU Configure and Build System.

For more available options, type:

    ./configure --help

To build and install, simply type:

    make

followed by

    sudo make install

Good Luck!
     //Joachim

