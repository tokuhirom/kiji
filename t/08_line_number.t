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
say($?LINE);
say($?LINE);
say($?LINE);
say($?LINE);
--- expected
1
2
3
4

===
--- code
say($?LINE);
# comment
# comment
say($?LINE);
--- expected
1
4

===
--- code
say($?LINE);
my $a := "hoge
fuga";
say($?LINE);
--- expected
1
4

===
--- code
say($?LINE);
my $a := 'hoge
fuga';
say($?LINE);
--- expected
1
4
