use strict;
use warnings;
use utf8;
use Test::More;
use Test::Base::Less;
use File::Temp;

for my $block (blocks) {
    my $tmp = File::Temp->new();
    print {$tmp} $block->code;

    my $ret = `./saru < $tmp`;
    is($ret, $block->expected);
}

done_testing;

__END__

===
--- code: say("Hello");
--- expected
Hello
