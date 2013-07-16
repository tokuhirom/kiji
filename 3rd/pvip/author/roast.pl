use strict;
use warnings;
use utf8;
use File::Find::Rule;
use Time::Piece;

my @files = sort File::Find::Rule->file()
                              ->name( '*.t' )
                              ->in( glob('~/dev/roast/') );

my $ok;
my $fail;

for (@files) {
    if (system("./pvip $_")==0) {
        $ok++;
    } else {
        print "FAIL: $_\n";
        $fail++;
    }
}

printf "%s - OK: %s, FAIL: %s (  %.2f%%)\n", localtime->strftime('%Y-%m-%d %H:%M'), $ok, $fail, 100.0*((1.0*$ok)/(1.0*($ok+$fail)));

