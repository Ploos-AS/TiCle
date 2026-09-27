set ::ticle_module [dict create \
    name hello \
    version 0.1.0 \
    description "Example TiCle command module"]

namespace eval ::ticle::mod::hello {}

proc ::ticle::mod::hello::command {msg args} {
    if {[llength $args]} {
        ticle::reply $msg "hello [join $args { }]"
    } else {
        ticle::reply $msg "hello [dict get $msg nick]"
    }
}

proc ::ticle::mod::hello::init {} {
    ticle::bind !hello ::ticle::mod::hello::command
}

proc ::ticle::mod::hello::shutdown {} {
    ticle::unbind !hello
}
