saru - -Ofun prototype of Perl6 interpreter
===========================================

This is a new great toy for Perl hackers.

Current Status
--------------

Under development.... pre-alpha.

Currently, we only support linux and OSX. Do not send me win32 patch.
Win32 patch makes hard to maintain. Please do it before first stable release.

What's this?
-------------

This is a interpreter for subset of Perl6. It's running on MoarVM.

Why?
----

I need lightweight Perl6 sub set interpreter for daily hacking.

Build time dependencies
-----------------------

These things are not required at run time.

 * Perl5
 * C++ compiler supports C++11

Use case
--------

 * You can write NQP compiler based on this library.

See also
--------

 * [NQP::Grammer](https://github.com/perl6/nqp/blob/master/src/NQP/Grammar.nqp)
 * [HLL::Grammer](https://github.com/perl6/nqp/blob/master/src/HLL/Grammar.nqp)
 * [greg](https://github.com/nddrylliog/greg)
   * PEG parser generator
 * [greg manual](http://piumarta.com/software/peg/peg.1.html)

