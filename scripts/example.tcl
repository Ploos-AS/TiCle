namespace eval ticle {}

proc ticle::on_line {line} {
    puts "IRC: $line"
}

proc ticle::on_message {msg} {
    set command [dict get $msg command]
    set prefix [dict get $msg prefix]
    set params [dict get $msg params]

    if {$command ne "PRIVMSG" || [llength $params] < 2} {
        return
    }

    set target [lindex $params 0]
    set text [lindex $params 1]
    if {$text ne "!ping"} {
        return
    }

    if {[string index $target 0] eq "#"} {
        set reply_target $target
    } else {
        set reply_target [lindex [split $prefix !] 0]
    }
    ticle::raw "PRIVMSG $reply_target :pong"
}
