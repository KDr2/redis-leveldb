#!/usr/bin/env perl
# -*- perl; coding: utf-8 -*-

use 5.010;
use Test::More;
use Tester;
BEGIN{ eval "no warnings 'experimental::smartmatch'"; }

our $tester = Tester->new;
$tester->start();

our $set_name = "test-set";

$tester->client->sadd($set_name, map { "set-element-$_" } 1..20);
ok($tester->client->scard($set_name) == 20, 'command sadd/scard');

$tester->client->srem($set_name, "set-element-7");
ok($tester->client->scard($set_name) == 19, 'command srem/scard');

our @members_in_db = $tester->client->smembers($set_name);
our @members_expected = map { "set-element-$_" } (grep { $_ != 7 } 1..20);
ok(@members_in_db ~~ @members_expected, "command smembers");
ok($tester->client->sismember($set_name, "set-element-1"), "command sismember");
ok(!$tester->client->sismember($set_name, "set-element-7"), "command sismemeber");

$tester->stop;
done_testing;
