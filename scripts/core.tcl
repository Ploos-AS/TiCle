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

namespace eval ticle::module {
    variable loaded [dict create]
}

proc ticle::module::load {path} {
    variable loaded
    set path [file normalize $path]
    if {[dict exists $loaded $path]} {
        error "module already loaded: $path"
    }

    set before [namespace children ::]
    source $path

    if {![info exists ::ticle_module]} {
        error "module did not define ::ticle_module metadata: $path"
    }
    set meta $::ticle_module
    unset ::ticle_module

    foreach key {name version} {
        if {![dict exists $meta $key]} {
            error "module metadata missing $key: $path"
        }
    }

    set ns "::ticle::mod::[dict get $meta name]"
    if {![namespace exists $ns]} {
        error "module namespace not found: $ns"
    }
    if {[llength [info commands "${ns}::init"]]} {
        "${ns}::init"
    }

    dict set loaded $path [dict create metadata $meta namespace $ns]
    return $meta
}

proc ticle::module::unload {path} {
    variable loaded
    set path [file normalize $path]
    if {![dict exists $loaded $path]} {
        error "module not loaded: $path"
    }

    set entry [dict get $loaded $path]
    set ns [dict get $entry namespace]
    if {[llength [info commands "${ns}::shutdown"]]} {
        "${ns}::shutdown"
    }
    namespace delete $ns
    dict unset loaded $path
}

proc ticle::module::reload {path} {
    set path [file normalize $path]
    ticle::module::unload $path
    return [ticle::module::load $path]
}

proc ticle::module::list {} {
    variable loaded
    return $loaded
}
