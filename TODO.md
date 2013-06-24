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

## Step 3.8 2013-06-20

 * postfix if/unless

## Step 3.9 2013-06-20

 * str comparation operators
 * eq, ne, gt,ge,lt,le

## Step 4 2013-06-20

 * pow operator

## Step 5 2013-06-20

 * `@*ARGS`

## Step 6 2013-06-20

 * `say([4,5,6].shift())`

## Step 6.1 2013-06-21

 * hash construction

## Step 6.2 2013-06-21

 * $hash<abc>

## Step 6.3 2013-06-22

 * postfix for

## Step 6.4 2013-06-22

 * slurp function
 * print function

## Step 6.5 2013-06-22

 * logical xor, logical and, logical or

## Step 6.6 2013-06-22

 * bit or, bit and, bit xor

## Step 6.6 2013-06-22

 * open('README.md').eof()

## Step 6.7 2013-06-22

 * say((1,2,3,4).join);

## Step 6.8 2013-06-23

 * single quoted string

## Step 6.9 2013-06-23

 * Added String.length method.

## Step 7.0 2013-06-23

 * if statement should return value.

## Step 7.1 2013-06-24

 * `for <a b c> { say($_) }`

## Step 7.2 2013-06-24

 * "\x6f\x6b 8"

## Step 7.3 2013-06-24

 * `for <a b c> -> $n { say($n); }`
 * $?LINE

## Step 7.4 2013-06-24

 * `(-> $n { say($n) })(5)`
 * `3.say`

## Step 7.5 2013-06-24

 * use

## Step 8.

 * `for <a b c d> -> $a, $b { say($a, $b) }`
 * `for <a b c> { .say }`
 * $hash.keys()
 * +(1,2,3)
 * next, last
 * `q<ok 7>`
 * `loop { }`
 * abs(-3), abs(3)
 * Array operations
  * ref http://doc.perl6.org/type/Array#elems
 * Hash operations
 * `my %hash = { a => 3 }; say(%hash<a>);`
 * Str methods
 * catch
 * `%*ENV`
 * `@_` special variable
 * socket operation
 * slurp() function with encoding option
 * Array#map
 * Array#grep
 * caller
 * fork
 * kill
 * waitpid
 * wait
 * `for <a b c> -> $n { say($n); }`
 * END block
 * exporter
 * implement builtin functions
    * sprintf
    * printf

## Step 5.9

 * MOP things

## Step 6.

 * Range object

## Step 10.

 * `my @a=1,2,3; while my $n=@a.shift() { say($n) }`
 * basic i/o
 * `open('hoge.pl').slurp`
 * multi assign like `my ($x, $y) = (1,2)`
 * require
 * `"foo { 3+2 }"`
 * ++$i , $i++ , --$i , $i--
 * variable scope in 'for' stmt
 * variable scope in 'if' stmt
 * variable scope in 'while' stmt
 * eval
 * try S32-str/lines.t in roast
 * `+"0x0a";`
 * heredocs?
  * needs 'assignment'

## Step 20

 * Support most of features in Perl5 without regexp, grammar, OOP

## Step 30.

 * Support OOP
 * read code from file
 * Support XSUB?
   * use MVMCFunction → it works well

## Step 40.

 * regexp
 * True/False
 * multi dispatch

## Step 41.

 * Grammars
 * One liner support
 * constant folding optimization
 * Optimize register allocation

## Step 50.

    say(foo());

    sub foo() { bar() }
    sub bar() { 3 }

## Step 100.

 * pass roast

## Step ∞. Will done in 2999-12-31

 * Complete.

