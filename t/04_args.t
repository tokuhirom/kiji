use strict;
use warnings;
use utf8;
use Test::More;
use File::Temp;

my $fh = File::Temp->new();
print {$fh} <<'...';
for @*ARGS { say($_) }
say("OK");
...
is(`./kiji $fh a b c`, qq{a\nb\nc\nOK\n});
is(`./kiji $fh`, qq{OK\n});

done_testing;

