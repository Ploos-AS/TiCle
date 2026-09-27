namespace eval ticle {}

proc ticle::on_line {line} {
    puts "IRC: $line"
}

proc ticle::on_privmsg {msg} {
    set nick [dict get $msg nick]
    set target [dict get $msg target]
    set text [dict get $msg text]

    if {$text ne "!ping"} {
        return
    }

    if {[string index $target 0] eq "#"} {
        set reply_target $target
    } else {
        set reply_target $nick
    }
    ticle::raw "PRIVMSG $reply_target :pong"
}

proc ticle::on_join {msg} {
    puts "JOIN: [dict get $msg nick] -> [dict get $msg target]"
}

proc ticle::on_part {msg} {
    puts "PART: [dict get $msg nick] -> [dict get $msg target]"
}
