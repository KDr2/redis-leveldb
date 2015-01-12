#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;
BEGIN{ eval "no warnings 'experimental::smartmatch'"; }

my $tester = Tester->new;
$tester->start();

foreach (1..20) {
    $tester->client->set("key-$_" => "value-$_");
}

foreach (1..20) {
    ok($tester->client->get("key-$_") eq "value-$_", "command get/set");
}

$tester->client->set('test-inc-0' => 1);
$tester->client->incr('test-inc-0');
ok($tester->client->get('test-inc-0') == 2, 'command incr');

$tester->client->set('test-inc-1' => 1024);
$tester->client->incrby('test-inc-1', 1024);
ok($tester->client->get('test-inc-1') == 2048, 'command incrby');

$tester->client->mset('mset_k1' => 1, 'mset_k2'=>2, 'mset_k3'=>3);
ok($tester->client->mget('mset_k1', 'mset_k2', 'mset_k3') ~~ [1, 2, 3], 'command mset/mget');

$tester->stop;
done_testing;
