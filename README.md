kiji - -Ofun Perl6 interpreter on MoarVM
=========================================

This is a new great toy for Perl hackers.

Current Status
--------------

Under development.... pre-alpha.

Currently, we only support linux and OSX. Do not send me win32 patch.
Win32 patch makes hard to maintain. Please do it before first stable release.

What's this?
-------------

Tiny interpreter run on MoarVM. It's just small toy.

Build time dependencies
-----------------------

These things are not required at run time.

 * Perl5
 * C++ compiler supports C++11

Supported Environment
---------------------

kiji is tested on ubuntu 12.04. But kiji may works any POSIX compliant environment.

Known bugs
----------

`t/99_fib.t` fails in GC.

See also
--------

 * [NQP::Grammer](https://github.com/perl6/nqp/blob/master/src/NQP/Grammar.nqp)
 * [HLL::Grammer](https://github.com/perl6/nqp/blob/master/src/HLL/Grammar.nqp)
 * [greg](https://github.com/nddrylliog/greg)
   * PEG parser generator
 * [greg manual](http://piumarta.com/software/peg/peg.1.html)

Status of roast
---------------

    2013-08-02 03:19 - OK: 5, FAIL: 874 ( 0.57%) in 11.089065 sec

