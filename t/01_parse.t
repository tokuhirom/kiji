use strict;
use warnings;
use utf8;
use Test::More;
use t::Util;

test( '"hoge"', statements(string('hoge')));
test( '"ho\nge"', statements(string("ho\nge")));


done_testing;

