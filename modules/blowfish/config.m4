EGG_MODULE_START(blowfish, [blowfish encryption], "yes")

# add blowfish to list of preloaded shared objects
LIBEGGDROP_PRELOAD="$LIBEGGDROP_PRELOAD \
		-dlopen ../modules/blowfish/blowfish.la"

EGG_MODULE_END()
