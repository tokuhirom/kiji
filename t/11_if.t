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
if 0 { say 'not ok 6' } elsif 1 { say 'ok 6' } elsif 0 { say 'not ok 6' }
--- expected
ok 6
