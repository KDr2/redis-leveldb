#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;

my $tester = Tester->new(db_number=>4);
$tester->start();

my $key_name = "test-key";

for ( 0..3 ) {
    is($tester->try("select", $_), "OK", "command select");
    is($tester->try("set", $key_name, $_), "OK", "command set");
}

for ( 0..3 ) {
    is($tester->try("select", $_), "OK", "command select");
    is($tester->try("get", $key_name), $_, "command get");
}

$tester->stop;
done_testing;
