module Test;

my $CNT = 0;

sub proclaim($cond, $desc) {
    $CNT++;
    unless $cond {
        print "not ";
    }
    print "ok $CNT";
    if ($desc) {
        print " - $desc\n";
    } else {
        print "\n";
    }
    !!$cond;
}

sub plan($n) is exportable {
    say "1.." ~ $n ~ "\n";
}

sub pass($desc='') is exportable{
    proclaim(1, $desc);
}

sub ok($cond, $msg="") is exportable {
    proclaim($cond, $msg);
}

sub is($got, $expected, $desc="") is exportable {
    ok($got eq $expected, $desc);
}

