
micropython
-----------

  This is a port of a build environment for Micro Python:

    https://micropython.org/

  Configuration Options:

    CONFIG_INTERPRETERS_MICROPYTHON - Enables support for the Micro Python
      interpreter
    CONFIG_INTERPRETERS_MICROPYTHON_URL - URL where Micro Python can be
      downloaded. default "https://github.com/micropython/micropython/archive".
      There are more recent snapshots at https://micropython.org/download/
    CONFIG_INTERPRETERS_MICROPYTHON_VERSION - Version number. Default "1.3.8"
    CONFIG_INTERPRETERS_MICROPYTHON_APPNAME - Executable name.  Only needed
      if CONFIG_NSH_BUILTIN_APPS=y.  Default: "micropython"
    CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE - Interpreter stack size.  Only
      needed if CONFIG_NSH_BUILTIN_APPS=y.  Default: 2048
    CONFIG_INTERPRETERS_MICROPYTHON_PRIORITY - Interpreter priority.  Only
      needed if CONFIG_NSH_BUILTIN_APPS=y.  Default: 100
    CONFIG_INTERPRETERS_MICROPYTHON_PROGNAME - Program name.  Only needed
      if CONFIG_BUILD_KERNEL=y.  Default: "micropython"

  NOTE that Micro Python is not included in this directory.  Be default,
  it will be downloaded at build time from the github .  You can avoid
  this download by pre-installing Micro Python.  Before building, just
  download Micro Python from:

    https://micropython.org/download/
    https://github.com/micropython/micropython/releases

  Or clone from the GIT repository at:

    https://github.com/micropython/
    https://github.com/micropython/micropython

  The Micro Python should be provided as a tarbll name:

    apps/interpreters/micropython/v$(CONFIG_INTERPRETERS_MICROPYTHON_VERSION).tar.gz

  and the unpacked code should reside in directory at:

    apps/interpreters/micropython/micropython-$(CONFIG_INTERPRETERS_MICROPYTHON_VERSION)

  This port was contributed by Dave Marples using Micro Python circa
  1.3.8.  It may not be compatible with other versions.

  NOTES:

  1. Micro Python will not build on Windows with a Windows native toolchain
     due to use of POSIX paths in the Micro Python build system.  It should build
     correctly on Linux or under Cygwin with the NuttX buildroot tools.

  2. Micro Python will not run correctly on a 64-bit target (such as the NuttX
     simulation on a 64-bit platfform).  In that case it generates assertions
     like:

       OverflowError: long int not supported in this build

     This change to mpconfigport.h is a partial work-around but does not solve
     all issues:

       -#define MICROPY_LONGINT_IMPL           (MICROPY_LONGINT_IMPL_MPZ)
       +#define MICROPY_LONGINT_IMPL           (MICROPY_LONGINT_IMPL_LONGLONG)

     Someday it will probably be necessary to autogenerate the mpconfigport.h
     header file with the correct properties for the target system.\

  3. Can't fine alloca.h?  With GCC compilers you may be able replace the
     inclusion of alloca.h in mkconfigport.h with:

       #define alloca(a) __builtin_alloca(a)

  4. Micro Python needs the math library libm.a.  The math library built into
     NuttX is sufficient and that can be included with CONFIG_LIBM=y.  If you
     prefer a more highly tuned math library then refer to the discussion of
     math.h in the the top-level nuttx/README.txt file.  Also refer to the
     discussions in the NuttX Yahoo! forum; people have found many creative
     way to link with the newlib math library, for example.

  5. See errors like this?

       error: unknown type name 'wint_t'

     You can't include the NuttX wchar.h header file where this is defined, but
     you can add this to the mpconfigport.h header file (if it is not already
     there):

       typedef int wint_t;

     Is the missing wint_t definition coming from alloca.h?  You can either
     (1) replace alloc(a) with the #define described above, or (2) move the
     typedef of wint_t to before the inclusion of alloca.h.

  6. Micro Python is released under the MIT license which is license-compatible
     with the NuttX 3-clause BSD license.  Here is the full text of the Micro
     Python LICENSE file as of 2015-01-14:

     The MIT License (MIT)

     Copyright (c) 2013, 2014 Damien P. George

     Permission is hereby granted, free of charge, to any person obtaining a copy
     of this software and associated documentation files (the "Software"), to deal
     in the Software without restriction, including without limitation the rights
     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     copies of the Software, and to permit persons to whom the Software is
     furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in
     all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
     THE SOFTWARE.

