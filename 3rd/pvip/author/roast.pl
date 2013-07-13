use strict;
use warnings;
use utf8;
use File::Find::Rule;

my @files = File::Find::Rule->file()
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

print "OK: $ok, FAIL: $fail\n";

