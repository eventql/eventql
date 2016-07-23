2.1 Download & Installation
===========================

EventQL currently runs on Linux and OSX. The distribution contains three binaries
called `evql` (client and interactive shell), `evqld` (server) and `evqlctl` (cluster control).

[You can download binary and source tarballs here.](/download/)

### Install from Git

Before we can start we need to install some build dependencies. Currently you
need a modern c++ compiler, libz, autotools and python (for spidermonkey/mozbuild)

    # Ubuntu
    $ apt-get install clang++ cmake make automake autoconf zlib1g-dev

    # OSX
    $ brew install automake autoconf

Now we can clone, compile and install EventQL:

    $ git clone git@github.com:eventql/eventql.git
    $ cd eventql
    $ ./autogen.sh && ./configure && make
    $ sudo make install


### Install from Source Tarball

[Download source tarballs here](/download/). To compile EventQL from a source
(distribution) tarball, you need a modern c++ compiler, libz and python
(for spidermonkey/mozbuild)

    $ ./configure
    $ make
    $ sudo make install


### Install from Binary Tarball

[Download binary tarballs here](/download/). To install a binary tarball, all we
need to do is to extract the archive to the right location:

    $ cd / && tar xzf /path/to/eventql-x.x.x-arch.tgz
