/*
 * This file exists for the sole purpose of containing Doxygen code. Don't read
 * this, run doxygen and read the pretty html pages instead.
 */

/*! \file
 * \brief Metafile. Ignore it.
 *
 * This file exists for the sole purpose of containing Doxygen code. Don't read
 * this, read \ref events "the generaged doxygen documentation" instead.
 */

/*!

\page events How to use asyncronous events.

\section client_data The client_data

Every asyncrounous event in the eggdrop API has a pointer to void called
client_data. This can be supplied at the time the event is registered and will
be passed back every time the event is triggered. It is never used in any other
way so it's save to small integers instead of a pointer as long as <b>no
assumptions about the max values are made</b>. This value is usually stored
inside a member of the related struct called \e client_data. Although it can be
changed it should not because that'd be an ugly hack.

\section owner The event owner

Modules and scripts can register events too, but they can also be unloaded at
any time. Because it'd be a real mess if every script and module had to keep
a list of all its registered events there is a way to indicate who owns what
useing the ::event_owner_t struct. Just like the client_data, it's supplied at
the time the event is registered, stored in a member (called \e owner) and
should not be changed. Events registered by the eggdrop core code don't have
to supply an owner but they can in case there'll be a feature that displays
some kind of information about events and their owners in the future. Modules
however \b must pass an owner to the registering function. Failing to do so
will result in a crash sooner or later! Every time a script or module is
unloaded, all events whose owners match the unloaded script or module are
deleted without calling any kind of event dependent cancel callback. The
::event_owner_t's \link event_owner_t::on_delete on_delete \endlink callback
however will be called when the event is deleted no matter why. The automatic
event deletion happens after the module's \link egg_module_t::close_func close
\endlink function is called, so the on_delete code might be called even after
that.

\section on_delete The on_delete callback.

The \link event_owner_t::on_delete on_delete \endlink callback function is a
member of the ::event_owner_t struct and will be called after the object the
owner belongs to has been deleted. <b>This means the object itself no longer
exists at the time of this call.</b> The callback function should free the
memory allocated for the client_data and the owner struct. This should not be
done anywhere else and nothing else should be done here! If neither the
client_data nor the owner have been malloc'd nothing needs to be done and the
on_delete pointer may be a NULL pointer so nothing will be called.

The callback function takes two parameters: A pointer to an ::event_owner_t
struct called \e owner and a pointer to void called \e client_data. The owner
parameter is the same pointer that was passed to the function registering the
event that was just deleted. If it's malloc'd memory it should be free'd here
otherwise it can be ignored. The client_data parameter is the same pointer
that was passed to the function registering the event.

An example might look like this:
\code
static void on_delete(event_owner_t *owner, void *client_data)
{
	somestruct_t *mydata = client_data;

	free(mydata->member1);
	free(mydata->member2);
	free(mydata);

	free(owner); // only if it was malloc'd.
}
\endcode

\subsection multipleevents Handleing multiple callbacks with the same client_data.

Sometimes multiple events use the same pointer for their client_data. For
example the ::partymember_t event handler and the partymember's
::sockbuf_t event handler use the same client_data. This means that only
one of those two on_delete callbacks can actually free the malloc'd data.
Usually both of these objects will be deleted at the same time but one will be
created before the other. The freeing of the memory should happen in the
on_delete callback of the object that was created first while the other's
callback should just delete the first object. Because the two callbacks do
different things they cannot have the same owner object.

An example might look like this:
\code
static void first_on_delete(event_owner_t *owner, void *client_data)
{   
	somestruct_t *mydata = client_data;

	mydata->object1 = NULL;
	if (mydata->object2) object2_delete(mydata->object2); // should not happen.

	free(mydata->member1);
	free(mydata->member2);
	free(mydata);

	free(owner); // only if it was malloc'd.
}

static void second_on_delete(event_owner_t *owner, void *client_data)
{   
	somestruct_t *mydata = client_data;

	mydata->object2 = NULL;
	if (mydata->object1) object1_delete(mydata->object1); // the avarage case.
}
\endcode

*/

