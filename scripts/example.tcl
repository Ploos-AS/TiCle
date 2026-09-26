namespace eval ticle {}

proc ticle::on_line {line} {
    puts "IRC: $line"
}

proc ticle::on_privmsg {msg} {
    set prefix [dict get $msg prefix]
    set params [dict get $msg params]
    if {[llength $params] < 2 || [lindex $params 1] ne "!ping"} {
        return
    }

    set target [lindex $params 0]
    if {[string index $target 0] eq "#"} {
        set reply_target $target
    } else {
        set reply_target [lindex [split $prefix !] 0]
    }
    ticle::raw "PRIVMSG $reply_target :pong"
}

proc ticle::on_join {msg} {
    puts "JOIN: [dict get $msg prefix] [dict get $msg params]"
}

proc ticle::on_part {msg} {
    puts "PART: [dict get $msg prefix] [dict get $msg params]"
}
