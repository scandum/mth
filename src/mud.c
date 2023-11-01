#include <stdarg.h>
  
#include "mud.h"
#include "mth.h"

MUD_DATA *mud;

/*
	This file is primarily for debugging so the source code can be
	compiled, modified, and tested without having to plug it into an
	existing codebase.
*/

void log_printf(char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);

	vsprintf(buf, fmt, args);

	va_end(args);

	printf("%s\n", buf);

	return;
}

/*
	Call translate_telopts() in telopt.c with an MSSP request.
*/

int main(int argc, char **argv)
{
	// MTH addition

	init_mth();

	// Initialize a dummy descriptor

	mud->client  = calloc(1, sizeof(DESCRIPTOR_DATA));
	mud->client->character = (char *) strdup("test");
	mud->client->host = (char *) strdup("");

	create_port(MUD_PORT);

	mainloop();

	return 0;
}


int write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length)
{
	if (d->descriptor)
	{
		// MTH addition

		if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_DISCONNECT))
		{
			return 0;
		}

		if (d->mth->mccp2)
		{
			write_mccp2(d, txt, length);
		}
		else
		{
			write(d->descriptor, txt, length);
		}
	}
	else
	{
		log_printf("ERROR: Write to descriptor (%s) no client connection.", txt);
	}

	return 0;
}
