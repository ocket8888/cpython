This is Paradisi version 0.0.1+
=============================

This project is forked from the CPython Project, and is subject to the terms of its license (see [LICENSE](./LICENSE) for more information).

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Python Software Foundation; All rights reserved.

See the end of this file for further copyright and license information.

Build Instructions
==================

On Unix, Linux, BSD, macOS, and Cygwin:

    ./configure
    make
    make test
    sudo make install

This will install Paradisi as `paradisi`.

You can pass many options to the configure script; run `./configure --help` to find out more. On macOS and Cygwin, the executable is called `paradisi.exe`; elsewhere it's just `paradisi`.

If you are running on macOS with the latest updates installed, make sure to install openSSL or some other SSL software along with Homebrew or another package manager. If issues persist, see <https://devguide.python.org/setup/#macos-and-os-x> for more information.

On macOS, if you have configured Paradisi with `--enable-framework`, you should use `make frameworkinstall` to do the installation. Note that this installs the Paradisi executable in a place that is not normally on your PATH, you may want to set up a symlink in `/usr/local/bin`.

On Windows, see [PCbuild/readme.txt](https://github.com/python/cpython/blob/master/PCbuild/readme.txt).

If you wish, you can create a subdirectory and invoke configure from there. For example:

    mkdir debug
    cd debug
    ../configure --with-pydebug
    make
    make test

(This will fail if you *also* built at the top-level directory. You should do a `make clean` at the toplevel first.)

To get an optimized build of Paradisi, `configure --enable-optimizations` before you run `make`. This sets the default make targets up to enable Profile Guided Optimization (PGO) and may be used to auto-enable Link Time Optimization (LTO) on some platforms. For more details, see the sections below.

Profile Guided Optimization
---------------------------

PGO takes advantage of recent versions of the GCC or Clang compilers. If used, either via `configure --enable-optimizations` or by manually running `make profile-opt` regardless of configure flags, the optimized build process will perform the following steps:

The entire Paradisi directory is cleaned of temporary files that may have resulted from a previous compilation.

An instrumented version of the interpreter is built, using suitable compiler flags for each flavour. Note that this is just an intermediary step. The binary resulting from this step is not good for real life workloads as it has profiling instructions embedded inside.

After the instrumented interpreter is built, the Makefile will run a training workload. This is necessary in order to profile the interpreter execution. Note also that any output, both stdout and stderr, that may appear at this step is suppressed.

The final step is to build the actual interpreter, using the information collected from the instrumented one. The end result will be a Paradisi binary that is optimized; suitable for distribution or production installation.

Link Time Optimization
----------------------

Enabled via configure's `--with-lto` flag. LTO takes advantage of the ability of recent compiler toolchains to optimize across the otherwise arbitrary `.o` file boundary when building final executables or shared libraries for additional performance gains.

Testing
=======

To test the interpreter, type `make test` in the top-level directory. The test set produces some output. You can generally ignore the messages about skipped tests due to optional features which can't be imported. If a message is printed about a failed test or a traceback or core dump is produced, something is wrong.

By default, tests are prevented from overusing resources like disk space and memory. To enable these tests, run `make testall`.

If any tests fail, you can re-run the failing test(s) in verbose mode. For example, if `test_os` and `test_gdb` failed, you can run:

    make test TESTOPTS="-v test_os test_gdb"

If the failure persists and appears to be a problem with Paradisi rather than your environment, you can [file an issue](https://github.com/ocket8888/cPython/issues/new) and include relevant output from that command to show the issue.

Issue Tracker and Mailing List
==============================

Bug reports are welcome! You can use the [issue tracker](https://github.com/ocket8888/cPython/issues/new) to report bugs, and/or submit pull requests [on GitHub](https://github.com/ocket8888/cpython).

You can also follow development discussion on the [python-dev mailing list](https://mail.python.org/mailman/listinfo/python-dev/).

Copyright and License Information
=================================

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Python Software Foundation. All rights reserved.

Copyright (c) 2000 BeOpen.com. All rights reserved.

Copyright (c) 1995-2001 Corporation for National Research Initiatives. All rights reserved.

Copyright (c) 1991-1995 Stichting Mathematisch Centrum. All rights reserved.

See the file "LICENSE" for information on the history of this software, terms & conditions for usage, and a DISCLAIMER OF ALL WARRANTIES.

This Python distribution contains *no* GNU General Public License (GPL) code, so it may be used in proprietary projects. There are interfaces to some GNU code but these are entirely optional.

All trademarks referenced herein are property of their respective holders.
