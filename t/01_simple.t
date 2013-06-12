use strict;
use warnings;
use utf8;
use Test::More;
use t::Util;

test( '33', int_(33));
test( '.33', number(0.33));
test( '0.33', number(0.33));
test( '3.14', number(3.14));
test( '0b1000', int_(8));
test( '0b0100', int_(4));
test( '0b0010', int_(2));
test( '0b0001', int_(1));
test( '0xdeadbeef', int_(0xdeadbeef));
test( '0o755', int_(0755));

test( '-5963', int_(-5963));
test( '3*4', mul(int_(3), int_(4)));

done_testing;

