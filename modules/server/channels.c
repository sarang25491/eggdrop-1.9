/* Some channel-specific stuff. */

#include "lib/eggdrop/module.h"
#include "output.h"

extern eggdrop_t *egg;

void channels_join_all()
{
	struct chanset_t *chan;
	for (chan = chanset; chan; chan = chan->next) {
		chan->status &= ~(CHAN_ACTIVE | CHAN_PEND);
		if (!channel_inactive(chan)) {
			printserv(SERVER_NORMAL, "JOIN %s%s%s",
				(chan->name[0]) ? chan->name : chan->dname,
				chan->channel.key[0] ? " " : "",
				chan->channel.key[0] ? chan->channel.key : chan->key_prot);
		}
	}
}
