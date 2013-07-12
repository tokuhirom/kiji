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
my $o = class { method bar() { say(5963); } }; my $n=$o.new(); $n.bar()
--- expected
5963

===
--- code
my $o = class { method bar() { say(5963); } }; $o.new().bar()
--- expected
5963

===
--- code
class Foo { method bar() { say(5963); } }; Foo.new().bar()
--- expected
5963
