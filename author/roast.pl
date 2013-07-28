#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use 5.010000;
use autodie;

use File::Find::Rule;
use Time::Piece;
use Time::HiRes qw(gettimeofday tv_interval);
use LWP::UserAgent;
use HTTP::Date;

my @files = sort File::Find::Rule->file()
                              ->name( '*.t' )
                              ->in( glob('3rd/roast/') );

my $ok;
my $fail;

my $t0 = [gettimeofday];
for (@files) {
  my $out=`./kiji $_`;
  if (POSIX::WIFEXITED($?) && POSIX::WEXITSTATUS($?)==0 && good_tap($out)) {
      print "OK: $_\n";
      $ok++;
  } else {
      print "FAIL: $_\n";
      $fail++;
  }
}
my $elapsed = tv_interval($t0);

my $percentage = 100.0*((1.0*$ok)/(1.0*($ok+$fail)));
printf "%s - OK: %s, FAIL: %s ( %.2f%%) in %s sec\n", localtime->strftime('%Y-%m-%d %H:%M'), $ok, $fail, $percentage, $elapsed;

my $datetime = time2str(time);

my $hf_base = "http://hf.64p.org/api/perl6/kiji";

my $ua = LWP::UserAgent->new();
my $res = $ua->post("$hf_base/ok", [number => $ok, datetime => $datetime]);
warn $res->as_string unless $res->is_success;
$ua->post("$hf_base/fail", [number => $fail, datetime => $datetime]);
# $ua->post("$hf_base/percentage", [number => $percentage, datetime => $datetime]);
$ua->post("$hf_base/elapsed", [number => int($elapsed*1000), datetime => $datetime]);

sub good_tap {
  my $tap = shift;
  return 0 if $tap =~ /^not ok/m;
  return 0 unless $tap =~ /^ok /m;
  return 1;
}
