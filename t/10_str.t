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
say('x' x 3);
--- expected
xxx

===
--- code
my $n = 'y'; $n x= 3; say($n);
--- expected
yyy

===
--- code
my $n = 'y'; $n ~= 3; say($n);
--- expected
y3
