typedef struct {
	int flagval;
	char *name;
} channel_flag_map_t;

static channel_flag_map_t normal_flag_map[] = {
	{CHAN_ENFORCEBANS, "enforcebans"},
	{CHAN_DYNAMICBANS, "dynamicbans"},
	{CHAN_USERBANS, "userbans"},
	{CHAN_OPONJOIN, "autoop"},
	{CHAN_BITCH, "bitch"},
	{CHAN_NODESYNCH, "nodesynch"},
	{CHAN_GREET, "greet"},
	{CHAN_PROTECTOPS, "protectops"},
	{CHAN_PROTECTFRIENDS, "protectfriends"},
	{CHAN_DONTKICKOPS, "dontkickops"},
	{CHAN_INACTIVE, "inactive"},
	{CHAN_LOGSTATUS, "statuslog"},
	{CHAN_REVENGE, "revenge"},
	{CHAN_REVENGEBOT, "revengebot"},
	{CHAN_SECRET, "secret"},
	{CHAN_SHARED, "shared"},
	{CHAN_AUTOVOICE, "autovoice"},
	{CHAN_CYCLE, "cycle"},
	{CHAN_HONORGLOBALBANS, "honor-global-bans"},
	{0, 0}
};

static channel_flag_map_t stupid_ircnet_flag_map[] = {
	{CHAN_DYNAMICEXEMPTS, "dynamicexempts"},
	{CHAN_USEREXEMPTS, "userexempts"},
	{CHAN_DYNAMICINVITES, "dynamicinvites"},
	{CHAN_USERINVITES, "userinvites"},
	{CHAN_HONORGLOBALEXEMPTS, "honor-global-exempts"},
	{CHAN_HONORGLOBALINVITES, "honor-global-invites"},
	{0, 0}
};
