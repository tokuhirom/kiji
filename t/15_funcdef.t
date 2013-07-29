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
sub sum($a, $b) { $a+$b }; say(sum(3,5))
--- expected
8
