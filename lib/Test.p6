module Test;

my $CNT = 0;

sub plan($n) is exportable {
  say "1.." ~ $n ~ "\n";
}

sub ok($x) is exportable {
  $CNT++;
  if ($x) {
    say "ok $CNT";
  } else {
    say "not ok $CNT";
  }
}
