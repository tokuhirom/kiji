use strict;
use warnings;
use utf8;
use Test::More;
use Test::Base::Less;
use File::Temp;
use POSIX;

for my $block (blocks) {
    note $block->code;

    my $tmp = File::Temp->new();
    print {$tmp} $block->code;

    my $ret = `./saru < $tmp`;
    ok(POSIX::WIFEXITED($?));
    is(POSIX::WEXITSTATUS($?), 0);

    $ret =~ s/\n+\z//;
    (my $expected = $block->expected) =~ s/\n+\z//;
    is($ret, $expected);
}

done_testing;

__END__

===
--- code: say("Hello");
--- expected
Hello

===
--- code: say("Hey");say("Yo!");
--- expected
Hey
Yo!

===
--- code: say("Hey"); say("Yo!");
--- expected
Hey
Yo!

===
--- code: say ( "Hey" ) ; say ( "Yo!" ) ;
--- expected
Hey
Yo!

===
--- code: say(5963)
--- expected
5963

===
--- code: say(3+2)
--- expected
5

===
--- code: say(3-2)
--- expected
1

===
--- code: say(3*2)
--- expected
6

===
--- code: say(20/2)
--- expected
10
