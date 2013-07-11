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
my $i=4640;
$i+=9;
say($i);
--- expected
4649

===
--- code
my $i=3;
$i+=5.2;
say($i);
--- expected
8.2

===
--- code
my $a = 0; $a += 1; ++$a;
say($a);
--- expected
2

