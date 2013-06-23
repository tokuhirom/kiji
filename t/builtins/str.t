use strict;
use warnings;
use utf8;
use Test::More 0.98;
use Test::Base::Less;
use t::Util;

binmode *STDOUT, ':utf8';
binmode *STDERR, ':utf8';

run_test_cases(blocks);

done_testing;

__END__

=== lc
--- code
say("hOgE".lc());
--- expected
hoge

=== uc
--- code
say("HOGE".uc());
--- expected
HOGE

=== length
--- code
say("HOGE".length());
say("ほげ".length());
say("".length());
--- expected
4
2
0

