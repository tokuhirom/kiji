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
use Test;
ok(1);
--- expected
ok 1

===
--- code
use Test;
ok(0);
--- expected
not ok 1

===
--- code
use Test;
ok(1, 'ah');
--- expected
ok 1 - ah

===
--- code
use Test;
ok(0, 'ah');
--- expected
not ok 1 - ah

===
--- code
sub foo () { say("YO") } ; foo()
--- expected
YO
