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
--- code
my $i:=3;
my $j:=4;
{
    my $i:=5;
    say($i);
    say($j);
}
say($i);
say($j);
--- expected
5
4
3
4

===
--- code
our $n=3;
say($n);
--- expected
3

===
--- code
our $n=3;
$n=4;
say($n);
--- expected
4

===
--- code
our $x = 5; { my $x; }; { say 3; }
--- expected
3

===
--- code
{ say("HO") }; { say("YO"); }
--- expected
HO
YO
