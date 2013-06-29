use strict;
use warnings;
use utf8;
use Test::More;
use Test::Base::Less;
use t::Util;

run_test_cases(blocks);

done_testing;

__END__

===
--- code
my $i=0;
say($i++);
say($i++);
say($i++);
say($i);
--- expected
0
1
2
3

===
--- code
my $i=5;
say($i--);
say($i--);
say($i--);
say($i);
--- expected
5
4
3
2

===
--- code
my $i=0;
say(++$i);
say(++$i);
say(++$i);
say($i);
--- expected
1
2
3
3

===
--- code
my $i=5;
say(--$i);
say(--$i);
say(--$i);
say($i);
--- expected
4
3
2
2
