saru - -Ofun interpreter on MoarVM
===================================

This is a new great toy for Perl hackers.

Current Status
--------------

Under development.... pre-alpha.

Currently, we only support linux and OSX. Do not send me win32 patch.
Win32 patch makes hard to maintain. Please do it before first stable release.

What's this?
-------------

saru aims to be the language of the backward compatibility of Perl6.
It's running on MoarVM.

Why?
----

I need lightweight Perl6 like interpreter for daily hacking.

Build time dependencies
-----------------------

These things are not required at run time.

 * Perl5
 * C++ compiler supports C++11

Supported Environment
---------------------

saru is tested on ubuntu 12.04. But saru may works any POSIX compliant environment.

Load map
========

 * Version 1.0 with most of nqp features without OOP, Grammar, Regex.
 * Version 2.0 with P5Regexp support.
 * Version 3.0 with OOP support.
 * and more features will be supported.

See also
--------

 * [NQP::Grammer](https://github.com/perl6/nqp/blob/master/src/NQP/Grammar.nqp)
 * [HLL::Grammer](https://github.com/perl6/nqp/blob/master/src/HLL/Grammar.nqp)
 * [greg](https://github.com/nddrylliog/greg)
   * PEG parser generator
 * [greg manual](http://piumarta.com/software/peg/peg.1.html)

