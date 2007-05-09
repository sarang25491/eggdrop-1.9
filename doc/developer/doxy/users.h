/*
 * This file exists for the sole purpose of containing Doxygen code. Don't read
 * this, run doxygen and read the pretty html pages instead.
 */

/*! \file
 * \brief Metafile. Ignore it.
 *
 * This file exists for the sole purpose of containing Doxygen code. Don't read
 * this, read \ref users "the generaged doxygen documentation" instead.
 */

/*!

\page users Eggdrop users and their settings.

All settings about a user, except for the flags, are stored in XML nodes.
Every user has several indipendent sets of settings, one for each channel
they are known on as well as global, settings. Each setting 
These nodes have a name and a value, both being strings. 

\section defined User settings that are used by eggdrop.

\arg \c password A hash of the user's password. Don't touch this! Use the
                 builtin password functions.
\arg \c salt The salt used to hash the user's password. Don't touch this! Use
             the builtin password functions.
\arg \c bot This is the namespace for all bot related settings. Should only
            exist if the user is a bot and should contain all of the
            bot-related settings.
\arg \c bot.type The type of the bot. This setting has to exist in order to
                 initiate a link to the bot. See ::BT_request_link for details.
\arg \c bot.link-priority The priority of this bot for auto-linking. If this
                          setting does not exist a default value of 0 is
                          assumed. If the value is negative the bot will never
                          be linked. For details see botnet_autolink().

\section common User settings that are commonly used by some modules.

\arg \c host The IP or hostname the bot is running on.
\arg \c port The port number the bot is listening on.
\arg \c password The password to send the bot to log in.

*/

