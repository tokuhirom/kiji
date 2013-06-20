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

## Step 2.01 Done at 2013-06-17

 * `say([1,2,3][1]);`

## Step 2.02 Done at 2013-06-18

 * `sub foo() { say(55) } foo();`

## Step 2.2  Done at 2013-06-18

 * `sub foo() { return 55 } say(foo());`

## Step 2.3  Done at 2013-06-18

 * Support -e option
 * apr command parsing

## Step 2.4  Done at 2013-06-18

 * `sub foo() { 55 } say(foo());`

## Step 2.5  Done at 2013-06-19

 * if-else
 * `sub foo($n) { return $n*2 } say(foo(4));`
 * funcall

## Step 3 Done at 2013-06-19

 * Run fib(n)

## Step 3.1 Done at 2013-06-19

 * `my $i:=0; while $i < 3 { say($i:=$i+1) }`
 * die

## Step 3.2 Done at 2013-06-19

 * if-elsif-else

## Step 3.3 2013-06-20

 * for loop
 * `my @a:=1,2,3; for @a { say($_) }`

## Step 3.4 2013-06-20

 * ! operator

## Step 3.5 2013-06-20

 * `<< >>` = qw()

## Step 3.6 2013-06-20

 * x ?? y !! z(conditional expression)

## Step 3.7 2013-06-20

 * Basic REPL

## Step 4.

 * next, last
 * `q<ok 7>`
 * "\x6f\x6b 8"
 * postfix if
 * `loop { }`
 * abs(-3), abs(3)
 * Array operations
 * Hash operations
 * catch
 * bit or, bit and, bit xor
 * logical xor, logical and, logical or
 * `%*ENV`
 * `@_` special variable
 * socket operation
 * implement builtin functions
    * sprintf
    * printf

## Step 4.9

 * str comparation operators
 * eq, ne, gt,ge,lt,le
 * MOP things

## Step 4.10

 * `say([4,5,6].shift())`

## Step 5.

 * Range object

## Step 10.

 * basic i/o
 * `open('hoge.pl').slurp`
 * multi assign like `my ($x, $y) = (1,2)`
 * require
 * use
 * `my @ary := 1,2,3; say($ary[0])`
 * @ARGS
 * `"foo { 3+2 }"`
 * ++$i , $i++ , --$i , $i--
 * heredocs?
  * needs 'assignment'

## Step 20

 * Support most of features in Perl5 without regexp, grammar, OOP

## Step 30.

 * Support OOP
 * read code from file
 * ? :
 * Support XSUB?
   * use MVMCFunction
 * if statement should return value.

## Step 40.

 * regexp
 * True/False
 * multi dispatch

## Step 41.

 * Grammars
 * One liner support
 * constant folding optimization
 * Optimize register allocation
 * $?LINE

## Step 50.

 * Support all statements in NQP
 * Pass NQP test cases

    say(foo());

    sub foo() { bar() }
    sub bar() { 3 }

## Step ∞. Will done in 2999-12-31

 * Support full feature of Perl6

