namespace eval ticle {}

proc ticle::on_line {line} {
    puts "IRC: $line"

    if {[regexp {^:([^! ]+)!.* PRIVMSG ([^ ]+) :!ping$} $line -> nick target]} {
        if {[string index $target 0] eq "#"} {
            ticle::raw "PRIVMSG $target :pong"
        } else {
            ticle::raw "PRIVMSG $nick :pong"
        }
    }
}
