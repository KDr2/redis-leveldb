#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;
BEGIN{ eval "no warnings 'experimental::smartmatch'"; }

my $tester = Tester->new;
$tester->start();

my $list_name = "test-list";
my $key_name = "test-key";

# multi, exec
is($tester->try("multi"), "OK", "command multi");

for ( 1..20 ) {
    is($tester->try("lpush", $list_name, "elm-l-$_"), "QUEUED", "queued command");
    is($tester->try("incr", $key_name), "QUEUED", "queued command");
}

my $result = $tester->try("exec");
ok(@$result ~~ @{[(map +("1", "$_"), 1..20)]}, "command exec");
is($tester->try("llen", $list_name), 20, "command llen");
ok($tester->try("get", $key_name) == 20, "command incr");

# multi, discard
is($tester->try("multi"), "OK", "command multi");
for ( 1..20 ) {
    is($tester->try("lpush", $list_name, "elm-l-$_"), "QUEUED", "queued command");
    is($tester->try("incr", $key_name), "QUEUED", "queued command");
}

my $result = $tester->try("discard");
ok($result == "OK", "command exec");
is($tester->try("llen", $list_name), 20, "command llen");
ok($tester->try("get", $key_name) == 20, "command incr");


$tester->stop;
done_testing;
