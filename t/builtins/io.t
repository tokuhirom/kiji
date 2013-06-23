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
say(open("README.md").eof());
say(open("README.md").eof);
--- expected
0
0
