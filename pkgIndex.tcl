# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded unqlite 0.3.8 \
	    [list load [file join $dir libtcl9unqlite0.3.8.so] [string totitle unqlite]]
} else {
    package ifneeded unqlite 0.3.8 \
	    [list load [file join $dir libunqlite0.3.8.so] [string totitle unqlite]]
}
