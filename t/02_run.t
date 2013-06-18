use strict;
use warnings;
use utf8;
use Test::More 0.98;
use Test::Base::Less;
use File::Temp;
use POSIX;

for my $block (blocks) {
    subtest $block->code => sub {
        my $tmp = File::Temp->new();
        print {$tmp} $block->code;

        my $ret = `./saru < $tmp`;
        ok(POSIX::WIFEXITED($?));
        is(POSIX::WEXITSTATUS($?), 0);

        $ret =~ s/\n+\z//;
        (my $expected = $block->expected) =~ s/\n+\z//;
        is($ret, $expected);
    };
}

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
--- SKIP
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
