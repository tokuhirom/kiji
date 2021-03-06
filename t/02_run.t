use strict;
use warnings;
use utf8;
use Test::More 0.98;
use Test::Base::Less;
use t::Util;

run_test_cases(blocks);

done_testing;

__END__

===
--- code: say("Hello");
--- expected
Hello

===
--- code: say("Hey");say("Yo!");
--- expected
Hey
Yo!

===
--- code: say("Hey"); say("Yo!");
--- expected
Hey
Yo!

===
--- code: say ( "Hey" ) ; say ( "Yo!" ) ;
--- expected
Hey
Yo!

===
--- code: say(5963)
--- expected
5963

===
--- code: say(3+2)
--- expected
5

===
--- code: say(3-2)
--- expected
1

===
--- code: say(3*2)
--- expected
6

===
--- code: say(20/2)
--- expected
10

===
--- code: say(10%2)
--- expected
0

===
--- code: say(10%3)
--- expected
1

===
--- code: say(10%4)
--- expected
2

===
--- code: say(10%5)
--- expected
0

===
--- code: my $i:=4649;say($i)
--- expected
4649

===
--- code: my $i:=2;say($i*3);
--- expected
6

===
--- code: say("Hello, " ~ "world");
--- expected
Hello, world

===
--- code: say(3 ~ 4);
--- expected
34

===
--- code: my $s:="X ";say($s ~ "Japan!");
--- expected
X Japan!
===

--- code: say(3.14);
--- expected
3.14

===
--- code: say(3.14+2.12);
--- expected
5.26

===
--- code: my $pi:=3.14; say($pi *2);
--- expected
6.28

===
--- code: say(-555);
--- expected
-555

===
--- code: say(-555*2);
--- expected
-1110

===
--- code: if 1 { say(4) } say(5);
--- expected
4
5

===
--- code: if 0 { say(4) } say(5);
--- expected
5

===
--- code: if 0.0 { say(4) } say(5);
--- expected
5

===
--- code: if 0.1 { say(4) } say(5);
--- expected
4
5

===
--- code: if "" { say(4) } say(5);
--- expected
5

===
--- code: if "a" { say(4) } say(5);
--- expected
4
5

===
--- code: my $i:=0; if $i { say(4) } say(5);
--- expected
5

===
--- code: my $i:=1; if $i { say(4) } say(5);
--- expected
4
5

===
--- code: if 4==4 { say(4) } say(5);
--- expected
4
5

===
--- code: if 4!=4 { say(4) } say(5);
--- expected
5

===
--- code: say([1,2,3][1]);
--- expected
2

===
--- code: say([4,5,6].shift());
--- expected
4

===
--- code
sub foo() { say(55) }
foo();
--- expected
55

===
--- code
sub foo() { return 5963 } say(foo());
--- expected
5963

===
--- code
sub foo() { 5963 } say(foo());
--- expected
5963

===
--- code
sub foo($n) { $n*2 } say(foo(5963));
--- expected
11926

===
--- code: if 1 { say(4) } else { say(5) }
--- expected
4

===
--- code: if 0 { say(4) } else { say(5) }
--- expected
5

===
--- code: if 3==3 { say(4) } else { say(5) }
--- expected
4

===
--- code: if 3==3 { say(4) } else { say(5) }
--- expected
4

===
--- code: sub foo() { if 3 { say(4) } }; foo()
--- expected
4

===
--- code: sub foo($n) { if 3 { say(4) } }; foo(4)
--- expected
4

===
--- code: sub foo($n) { if $n==4 { say(5) } }; foo(4)
--- expected
5

===
--- code
sub foo($n) {
    if $n <= 2 {
        return 1;
    } else {
        return 8;
    }
}

say(foo(1));
say(foo(3));
--- expected
1
8

===
--- code
sub foo($n) {
    if $n <= 2 {
        return 1;
    } else {
        return foo($n-1);
    }
}

say(foo(1));
say(foo(3));
--- expected
1
1

===
--- code
sub foo($n) {
    if $n <= 2 {
        return 1;
    } else {
        return foo($n-1)+foo($n-2);
    }
}

say(foo(1));
say(foo(3));
say(foo(5));
--- expected
1
2
5

====
--- code
my $a := [1,2,3];
say($a[1]);
--- expected
2

===
--- code
my $i:=0;
while $i < 3 { say($i:=$i+1) }
--- expected
1
2
3

===
--- code
my $i := "25" + 10;
say($i);
--- expected
35

===
--- code
if 0 { say("FAIL") } elsif (1) { say("OK") }
--- expected
OK

===
--- code
if 0 { say("FAIL") } elsif (0) { say("FAIL") } else { say("OK") }
--- expected
OK

====
--- code
my @a := 1,5,3;
say(@a[1]);
--- expected
5

====
--- code
say(1,2,3); say(1,2,3);
--- expected
123
123

====
--- code
my @a:=1,2,3;
for @a { say($_); }
--- expected
1
2
3

====
--- code
unless 1 { say("UNLESS 1"); }
unless 0 { say("UNLESS 2"); }
--- expected
UNLESS 2

====
--- code
for 1,2,3 { say($_); }
--- expected
1
2
3

===
--- code
if !1 { say("FAIL"); }
if !0 { say("OK"); }
if ! 0 { say("OK"); }
--- expected
OK
OK

===
--- code
say(<< a b c >>[0]);
say(<< a b c >>[1]);
say(<< a b c >>[2]);
--- expected
a
b
c

===
--- code
for <<a b c >> { say($_); }
--- expected
a
b
c

===
--- code
say(1 ?? 2 !! 3);
say(0 ?? 2 !! 3);
--- expected
2
3

===
--- code
say("o2") if 1
--- expected
o2

===
--- code
say("o2") if 1;
--- expected
o2

===
--- code
say("o1") if 0;
say("o2") if 1;
--- expected
o2

===
--- code
say("o1") unless 0;
say("o2") unless 1;
--- expected
o1

===
--- code
say("OK1") if "o" eq "o";
say("FAIL1") if "o" eq "b";
say("OK2") if "o" ne "b";
say("FAIL2") if "o" ne "o";
--- expected
OK1
OK2

===
--- code
say(3**2);
say(3**3);
say(2**10);
--- expected
9
27
1024

===
--- code
say($_) for 1,2,3;
--- expected
1
2
3

===
--- code
say({abc => "def", geh => "ijk"}{"abc"})
say({"abc" => "def", "geh" => "ijk"}{"geh"})
--- expected
def
ijk

===
--- code
my $h:={abc => "def", geh => "ijk"};
say($h<abc>);
--- expected
def

===
--- code
my $h:={abc => "def", geh => "ijk"};
say($h.elems());
--- expected
2

===
--- SKIP
--- code
my $h:={abc => "def", geh => "ijk"}; for $h.keys() { say($_); }
--- expected
abc
geh

===
--- code
say($_) for 1,2,3;
--- expected
1
2
3

===
--- SKIP
--- code
sub plan($n) { say($n); }
plan 3;
--- expected
3

=== blocks
--- code
{ say(1); say(2); }
--- expected
1
2

=== omit semicolon
--- code
{ say(1); say(2) }
--- expected
1
2

===
--- code
sub a() { say 5 }
a();
--- expected
5

===
--- code
say(0 && 0);
say(0 && 5);
say(4 && 5);
say(3 && 2);
say(8 && 0);
--- expected
0
0
5
2
0

===
--- code
say(0 || 0);
say(0 || 5);
say(4 || 5);
say(3 || 2);
say(8 || 0);
--- expected
0
5
4
3
8

===
--- SKIP
--- code
say(0 ^^ 0);
say(0 ^^ 5);
say(4 ^^ 5);
say(3 ^^ 2);
say(8 ^^ 0);
--- expected
0
5
Nil
Nil
8

===
--- code
0 or say(3);
1 or say(4);
0 and say(5);
1 and say(6);
--- expected
3
6

=== ** is weaken than *
--- code
say(2*3**4);
--- expected
162

===
--- code
say(4 +& 4);
say(0b101 +& 0b111);
say(0b0101 +| 0b1111);
say(0b0101 +| 0b1000);
say(0b0101 +^ 0b1111);
say(0b0101 +^ 0b1000);
--- expected
4
5
15
13
10
13

===
--- code
sub foo($n) { if $n==0 { 5 } elsif $n==1 { 3 } else { 8 } }
say(foo(0));
say(foo(1));
say(foo(2));
--- expected
5
3
8

===
--- code
for <a b c> { say($_) }
--- expected
a
b
c

===
--- code
(-> $n { say($n) })(5)
--- expected
5

=== lambda without arguments
--- code
(-> { say("GO") })()
--- expected
GO

===
--- code
for <a b c> -> $s { say($s) }
--- expected
a
b
c

===
--- code
use Hello;
say("YO");
--- expected
Hello, world!
YO

===
--- code
use v6;
use Hello;
say("YO");
--- expected
Hello, world!
YO

===
--- code
say((not 0) ?? 'OK' !! 'FAIL');
say((not 1) ?? 'FAIL' !! 'OK');
--- expected
OK
OK

===
--- code
# comment
say("YO");
--- expected
YO

===
--- code
say 'ok ', 0_0_1_4;
--- expected
ok 14

===
--- code

# L<S01/"Random Thoughts"/specifically tell it you're running Perl 6>
use v6;
say("OK");
--- expected
OK

===
--- code
say 7 +& +^1;
--- expected
6

=======
--- code
for 1,2,3 { .say() }
--- expected
1
2
3

===
--- code
for 1,2,3 { .say }
--- expected
1
2
3

=== last for 'while'
--- code
my $i:=0;
while 1 {
    $i:=$i+1;
    say $i;
    if $i == 3 { last }
}
--- expected
1
2
3

=== last for 'for'
--- code
for 1,2,3,4,5 { .say; if $_ == 4 { last } }
--- expected
1
2
3
4

=== last for 'for'
--- code
for 1,2,3 { .say; next if $_==2; .say }
--- expected
1
1
2
3
3

=== last for 'for'
--- code
my $i:=0; while $i<4 {  $i:=$i+1; say($i); next if $i==2; say($i); }
--- expected
1
1
2
3
3
4
4

===
--- code
my $i=5;
say("i=$i\nGah!");
--- expected
i=5
Gah!

===
--- code
say(abs -2);
say(abs 2);
--- expected
2
2

===
--- code
1 and say 'ok 1';
--- expected
ok 1

===
--- code
0 ^^ say 'ok 6';
0 xor say 'ok 7';
--- expected
ok 6
ok 7
