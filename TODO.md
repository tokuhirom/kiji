TODO
====

## Step 0. Done in 2013-06-12

 * Start project

## Step 0.1. Done in 2013-06-12

 * Parse some expressions

## Step 0.2  Done in 2013-06-13

 * Hello world on MoarVM with very simple C code

## Step 1.   Done in 2013-06-14

 * simple string
 * ident
 * say('Hello, world') on MoarVM
 * double quoted string

## Step 1.1 Done in 2013-06-14

 * say(3)

## Step 1.1 Done in 2013-06-14

 * say(3+2)

## Step 1.11 Done in 2013-06-14

 * `say(3*2)`
 * `say(3/2)`
 * `say(3-2)`

## Step 1.12 Done at 2013-06-14

 * `say(1+(3-2)/5)`

## Step 1.2 Done at 2013-06-15

 * say(3%2)

## Step 1.3 Done at 2013-06-15

 * Parse `$n`
 * Parse `my $n`
 * Parse `my $n := 3`

## Step 1.4 Done at 2013-06-16

 * my $i:=0; say($i);
 * `my $i:=2; say($i*3);`

## Step 1.5 Done at 2013-06-16

 * Refactor frame

## Step 1.6 Done at 2013-06-16

 * string concat

## Step 1.7 Done at 2013-06-17

 * `3.14*2`
 * say(3.2)

## Step 1.8 Done at 2013-06-17

 * Generate assembler from MoarVM/src/core/oplist
 * say(-3);
 * `say(-3*-1);`
 * variable assignment for string

## Step 1.9 Done at 2013-06-17

 * `if 0 { }`

## Step 1.91 Done at 2013-06-17

 * `if 0.1 { }`
 * `if "hoge" { }`
 * `my $i:=0; if $i { }`

## Step 1.92 Done at 2013-06-17

 * int/double comparation operators

## Step 2 Done at 2013-06-17

 * `s/SARU_NODE_/saru::NODE_/g`

## Step 2.01

 * str comparation operators

## Step 3.

 * apr command parsing
 * abs(-3), abs(3)
 * eq, ne, gt,ge,lt,le
 * Array operations
 * Hash operations

## Step 4.

 * funcall
 * Run fib(n)
 * Range object

## Step 10.

 * basic i/o
 * multi assign like `my ($x, $y) = (1,2)`

## Step 20

 * Support most of features in Perl5 without regexp, grammar, OOP
 * Support -e option

## Step 30.

 * Support OOP
 * read code from file
 * ? :

## Step 40.

 * regexp
 * True/False
 * multi dispatch

## Step 41.

 * Grammars
 * REPL
 * One liner support
 * constant folding optimization

## Step 50.

 * Support all statements in NQP
 * Pass NQP test cases

## Step ∞. Will done in 2999-12-31

 * Support full feature of Perl6

