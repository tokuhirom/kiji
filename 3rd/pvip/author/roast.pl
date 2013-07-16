use strict;
use warnings;
use utf8;
use File::Find::Rule;
use Time::Piece;
use Time::HiRes qw(gettimeofday tv_interval);

my @files = sort File::Find::Rule->file()
                              ->name( '*.t' )
                              ->in( glob('~/dev/roast/') );

my $ok;
my $fail;

my $t0 = [gettimeofday];
for (@files) {
    if (system("./pvip $_")==0) {
        $ok++;
    } else {
        print "FAIL: $_\n";
        $fail++;
    }
}
my $elapsed = tv_interval($t0);

printf "%s - OK: %s, FAIL: %s (  %.2f%%) in %s sec\n", localtime->strftime('%Y-%m-%d %H:%M'), $ok, $fail, 100.0*((1.0*$ok)/(1.0*($ok+$fail))), $elapsed;

