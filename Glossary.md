Glossary about Saru and MorVM
=============================

## MVM

Shorthand for MoarVM

## C++11

Latest C++ version. see wikipedia

## SC

Serialization Context.

ref. src/6model/reprs/SCRef.h

    /* A serialization context exists (optionally) per compilation unit.
     * It contains the declarative objects for the compilation unit, and
     * they are serialized if code is pre-compiled. */

`deserialize` op reads SC.

moarvm reads sc it in src/6model/serialization.c.

It contains definition of classes.

You can disable SC related serialization at src/QASTCompilerMAST.nqp.
