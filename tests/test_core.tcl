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
assert_equal [lindex $::seen_args 0] {one two} "command arguments"
assert_equal $::seen_nick alice "message dictionary passed to callback"

ticle::unbind !test
assert_equal [ticle::dispatch_command $msg] 0 "unbound command does not dispatch"

set ::ticle::sent {}
ticle::reply [dict create nick alice target #test] hello
assert_equal [lindex $::ticle::sent end] {PRIVMSG #test :hello} "channel reply"

ticle::reply [dict create nick alice target TiCle] hello
assert_equal [lindex $::ticle::sent end] {PRIVMSG alice :hello} "private reply"

set ::ticle::sent {}
ticle::privmsg #test {hello world}
assert_equal [lindex $::ticle::sent end] {PRIVMSG #test :hello world} "privmsg command"

ticle::notice alice {heads up}
assert_equal [lindex $::ticle::sent end] {NOTICE alice :heads up} "notice command"

ticle::join #test
assert_equal [lindex $::ticle::sent end] {JOIN #test} "join command"

ticle::join #secret key123
assert_equal [lindex $::ticle::sent end] {JOIN #secret key123} "keyed join command"

ticle::part #test {leaving now}
assert_equal [lindex $::ticle::sent end] {PART #test :leaving now} "part command"

ticle::nick NewNick
assert_equal [lindex $::ticle::sent end] {NICK NewNick} "nick command"

if {![catch {ticle::privmsg "#test\r\nQUIT" bad}]} {
    fail "privmsg target accepted CR/LF injection"
}
if {![catch {ticle::notice alice "bad\nQUIT"}]} {
    fail "notice text accepted CR/LF injection"
}
if {![catch {ticle::join {#bad channel}}]} {
    fail "join accepted whitespace in channel"
}

ticle::mode #test +o alice
assert_equal [lindex $::ticle::sent end] {MODE #test +o alice} "mode command"

ticle::mode TiCle +i
assert_equal [lindex $::ticle::sent end] {MODE TiCle +i} "user mode command"

ticle::kick #test alice
assert_equal [lindex $::ticle::sent end] {KICK #test alice} "kick command"

ticle::kick #test alice {rule violation}
assert_equal [lindex $::ticle::sent end] {KICK #test alice :rule violation} "kick reason"

ticle::whois alice
assert_equal [lindex $::ticle::sent end] {WHOIS alice} "whois command"

if {![catch {ticle::mode #test +o {bad nick}}]} {
    fail "mode accepted whitespace in argument"
}
if {![catch {ticle::kick #test "bad\nQUIT"}]} {
    fail "kick accepted CR/LF in nickname"
}
if {![catch {ticle::kick #test alice "bad\r\nQUIT"}]} {
    fail "kick accepted CR/LF in reason"
}
if {![catch {ticle::whois {bad nick}}]} {
    fail "whois accepted whitespace in nickname"
}

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
