#!/usr/bin/env ruby

require "redis"

redis = Redis.new(:host => "127.0.0.1", :port => 8323, :timeout => 600)

puts redis.set("mykey", "hello world")

puts redis.get("mykey")


def pipeline_test(redis)
  # transaction: multi/exec/discard
  #redis.pipelined do
  redis.multi do
	100000.times do
	  redis.set('foo1', 'bar')
	  redis.get('foo1')
	  redis.incr("c")
	end
  end
end

p(pipeline_test redis)

