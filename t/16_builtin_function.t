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
my $a = [4,6,4];
push $a, 9;
say($a[0])
say($a[1])
say($a[2])
say($a[3])
say($a.elems);
--- expected
4
6
4
9
4

===
--- code
my $a = [];
push $a, 4,6,4,9;
say($a[0])
say($a[1])
say($a[2])
say($a[3])
say($a.elems);
--- expected
4
6
4
9
4
