/* Some channel-specific stuff. */

#include "lib/eggdrop/module.h"

extern eggdrop_t *egg;

void channels_join_all()
{
	struct chanset_t *chan;
	for (chan = chanset; chan; chan = chan->next) {
		chan->status &= ~(CHAN_ACTIVE | CHAN_PEND);
		if (!channel_inactive(chan)) {
			dprintf(DP_SERVER, "JOIN %s %s\n",
				(chan->name[0]) ? chan->name : chan->dname,
				chan->channel.key[0] ? chan->channel.key : chan->key_prot);
		}
	}
}
