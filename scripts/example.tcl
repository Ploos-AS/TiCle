namespace eval ticle {}

set script_dir [file dirname [file normalize [info script]]]
source [file join $script_dir core.tcl]
ticle::module::load [file join $script_dir .. modules hello.tcl]

proc cmd_ping {msg args} {
    ticle::reply $msg "pong"
}

proc cmd_echo {msg args} {
    if {[llength $args] == 0} {
        ticle::reply $msg "usage: !echo <text>"
        return
    }
    ticle::reply $msg [join $args " "]
}

ticle::bind !ping cmd_ping
ticle::bind !echo cmd_echo

proc ticle::on_privmsg {msg} {
    ticle::dispatch_command $msg
}

proc ticle::on_join {msg} {
    puts "JOIN: [dict get $msg nick] -> [dict get $msg target]"
}

proc ticle::on_part {msg} {
    puts "PART: [dict get $msg nick] -> [dict get $msg target]"
}
