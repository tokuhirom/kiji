use strict;
use warnings;
use utf8;
use Test::More;

is(`./kiji eg/fib.p6`, "75025\n");

done_testing;

