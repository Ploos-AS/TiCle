#!/usr/bin/env tclsh

proc fail {message} {
    puts stderr "FAIL: $message"
    exit 1
}

proc assert_equal {actual expected message} {
    if {$actual ne $expected} {
        fail "$message: expected <$expected>, got <$actual>"
    }
}

namespace eval ticle {}
proc ticle::raw {line} {
    variable sent
    lappend sent $line
}

set root [file normalize [file join [file dirname [info script]] ..]]
source [file join $root scripts core.tcl]

proc test_command {msg args} {
    set ::seen_args $args
    set ::seen_nick [dict get $msg nick]
}

set msg [dict create nick alice target #test text {!TeSt one two}]
ticle::bind !test test_command
assert_equal [ticle::dispatch_command $msg] 1 "bound command dispatch"
assert_equal $::seen_args {one two} "command arguments"
assert_equal $::seen_nick alice "message dictionary passed to callback"

ticle::unbind !test
assert_equal [ticle::dispatch_command $msg] 0 "unbound command does not dispatch"

set ::ticle::sent {}
ticle::reply [dict create nick alice target #test] hello
assert_equal [lindex $::ticle::sent end] {PRIVMSG #test :hello} "channel reply"

ticle::reply [dict create nick alice target TiCle] hello
assert_equal [lindex $::ticle::sent end] {PRIVMSG alice :hello} "private reply"

set module [file join $root modules hello.tcl]
set meta [ticle::module::load $module]
assert_equal [dict get $meta name] hello "module metadata"
if {![dict exists [ticle::module::list] [file normalize $module]]} {
    fail "loaded module missing from module list"
}

ticle::module::unload $module
if {[dict exists [ticle::module::list] [file normalize $module]]} {
    fail "module remains listed after unload"
}

set meta [ticle::module::load $module]
set meta [ticle::module::reload $module]
assert_equal [dict get $meta name] hello "module reload"
ticle::module::unload $module

puts "PASS: Tcl API tests"
