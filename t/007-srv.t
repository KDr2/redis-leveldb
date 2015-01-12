#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;
BEGIN{ eval "no warnings 'experimental::smartmatch'"; }

my $tester = Tester->new();
$tester->start();

# command keys
my @keys = ("t1-key", "t2-key", "t3-key");

for my $key (@keys) {
    for ( 0..3 ) {
        is($tester->try("set", "${key}-$_", $_), "OK", "command set");
    }
}

for my $key (@keys) {
    my $expected_keys = [ map {; $key . "-$_" } 0..3 ];
    my @keys_in_db = $tester->try("keys", $key . "-*");
    ok(@keys_in_db ~~ $expected_keys, "command keys");
}

my @expected_keys = ();
for my $key (@keys) {
    push @expected_keys, (map {; $key . "-$_" } 0..3);
}
my @keys_in_db = $tester->try("keys", "*");
ok(@keys_in_db ~~ @expected_keys, "command keys");

# command shutdown
my $shutdown_status = $tester->try("shutdown");
is($shutdown_status, 1, "command shutdown");
$tester->stop if (!$shutdown_status);
$tester->clean_data;

done_testing;
