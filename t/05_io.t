use strict;
use warnings;
use utf8;
use Test::More;

is(`./kiji -e 'print(slurp("t/dat/hello"));'`, "world\n");

done_testing;

