#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;

my $tester = Tester->new;
$tester->start();

my $list_name = "test-list";

for ( 1..5 ) {
    $tester->try(
        "lpush", $list_name, "elm-l-$_"
    );
    $tester->try(
        "rpush", $list_name, "elm-r-$_"
    );
}

is($tester->try("llen", $list_name), 10, "command lpush/rpush/llen");
ok($tester->try("lpop", $list_name) == "elm-l-5", "command lpop");
ok($tester->try("rpop", $list_name) == "elm-r-5", "command rpop");
is($tester->try("llen", $list_name), 8, "command llen");

$tester->stop;
done_testing;
