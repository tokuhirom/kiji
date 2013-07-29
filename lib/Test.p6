module Test;

my $CNT = 0;

sub plan($n) is exportable {
  say "1.." ~ $n ~ "\n";
}

sub ok($x, $msg="") is exportable {
  $CNT++;
  if ($x) {
    print "ok $CNT";
    if ($msg) {
      print " - $msg\n";
    }
  } else {
    say "not ok $CNT";
    if ($msg) {
      print " - $msg\n";
    }
  }
}
