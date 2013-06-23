use strict;
use warnings;
use utf8;
use Test::More 0.98;
use Test::Base::Less;
use t::Util;

run_test_cases(blocks);

done_testing;

__END__

=== shift
--- code
my @a:=1,2,3;
say(@a.shift());
say(@a.shift());
say(@a.shift());
--- expected
1
2
3

=== shift
--- code
my @a:=1,2,3;
say(@a.elems());
say(@a.shift());
say(@a.elems());
--- expected
3
1
2


=== pop
--- code
my @a:=423,553,664;
say(@a.elems());
say(@a.pop());
say(@a.elems());
--- expected
3
664
2


=== join
--- code
my @a:=5,9,6,3;
say(@a.join("-"));
say([1,2,3,4].join("*"));
say([1,2,3,4].join(5));
say([1,2,3,4].join);
--- expected
5-9-6-3
1*2*3*4
1525354
1234

=== join
--- code
say((1,2,3,4).join);
--- expected
1234


=== push
--- code
my $a:=[5,9,6];
$a.push(3);
say($a.join("-"));
--- expected
5-9-6-3

=== unshift
--- code
my $a:=[5,9,6];
$a.unshift(3);
say($a.join("-"));
--- expected
3-5-9-6

