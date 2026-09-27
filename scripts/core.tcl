namespace eval ticle {
    variable binds [dict create]
}

proc ticle::bind {command callback} {
    variable binds
    if {![string match "!*" $command]} {
        error "command bind must start with !"
    }
    if {$callback eq ""} {
        error "callback must not be empty"
    }
    dict set binds [string tolower $command] $callback
}

proc ticle::unbind {command} {
    variable binds
    dict unset binds [string tolower $command]
}

proc ticle::dispatch_command {msg} {
    variable binds
    set text [dict get $msg text]
    if {![string match "!*" $text]} {
        return 0
    }

    set words [split $text]
    set command [string tolower [lindex $words 0]]
    if {![dict exists $binds $command]} {
        return 0
    }

    set callback [dict get $binds $command]
    set args [lrange $words 1 end]
    uplevel #0 [list $callback $msg $args]
    return 1
}

proc ticle::reply {msg text} {
    set target [dict get $msg target]
    if {![string match "#*" $target]} {
        set target [dict get $msg nick]
    }
    ticle::raw "PRIVMSG $target :$text"
}
