proc test:001:cb:msgm { nick uhost chan text } {
	putlog "msgm { $nick $uhost $chan $text }"
}

proc test:001:cb:msg { nick uhost chan text } {
	putlog "msg { $nick $uhost $chan $text }"
}

if {![module_loaded "server"]} {
	putlog "Module 'server' needs to be loaded for this test."
} else {
	bind msgm -|- "*"    test:001:cb:msgm
	bind msg  -|- "test" test:001:cb:msg
}
