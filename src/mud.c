#include <stdarg.h>
  
#include "mud.h"
#include "telnet.h"

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

void log_descriptor_printf(DESCRIPTOR_DATA *d, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);

	vsprintf(buf, fmt, args);

	va_end(args);

	printf("D%d@%s %s\n", d->descriptor, d->host, buf);

	return;
}

/*
	Call translate_telopts() in telopt.c with an MSSP request.
*/

int main(int argc, char **argv)
{
//	DESCRIPTOR_DATA *d;
//	char input[MAX_INPUT_LENGTH], output[MAX_INPUT_LENGTH];
//	int size = 0;

	// Create the mud data structure

	mud = calloc(1, sizeof(MUD_DATA));

	mud->mccp_len = COMPRESS_BUF_SIZE;
	mud->mccp_buf = calloc(COMPRESS_BUF_SIZE, sizeof(unsigned char));

	// Initialize the MSDP table

	init_msdp_table();

	// Initialize a dummy descriptor

	mud->client = calloc(1, sizeof(DESCRIPTOR_DATA));

	mud->client->character = (char *) strdup("test");
	mud->client->proxy = (char *) strdup("");
	mud->client->host = (char *) strdup("");
	mud->client->terminal_type = (char *) strdup("");

	// Open a port

	create_port(MUD_PORT);

	// Poll for input on test server

	mainloop();

	// Lets debug the telopt handler.

/*
	printf("\n\n\033[1;31m-- receiving announce_support\033[0m\n\n");

	announce_support(d);

	printf("\n\n\033[1;31m-- sending IAC DO MSSP as a broken packet:\033[0m\n\n");

	sprintf(input, "%c", IAC);

	size += translate_telopts(d, input, 1, output);

	sprintf(input, "%c", DO);

	size += translate_telopts(d, input, 1, &output[size]);

	sprintf(input, "%c", TELOPT_MSSP);

	size += translate_telopts(d, input, 1, &output[size]);


	printf("\n\n\033[1;31m-- sending broken MSDP packet:\033[0m\n\n");

	sprintf(input, "%c%c%c%c%c%c%c", IAC, DO, TELOPT_MSDP, IAC, SB, TELOPT_MSDP, MSDP_VAR);

	size += translate_telopts(d, input, 7, &output[size]);

	sprintf(input, "%s%c%s%c%c", "LIST", MSDP_VAL, "COMMANDS", IAC, SE);

	size += translate_telopts(d, input, 15, &output[size]);

	sprintf(input, "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP, MSDP_VAR, "LIST", MSDP_VAL, "SENDABLE_VARIABLES", IAC, SE);

	size += translate_telopts(d, input, strlen(input), &output[size]);

	sprintf(input, "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP, MSDP_VAR, "SEND", MSDP_VAL, "SPECIFICATION", IAC, SE);

	size += translate_telopts(d, input, strlen(input), &output[size]);

	sprintf(input, "%c%c%c%c%s%c%c%c%s%c%s%c%c%c", IAC, SB, TELOPT_MSDP, MSDP_VAR, "SEND", MSDP_VAL, MSDP_TABLE_OPEN, MSDP_VAR, "SPECIFICATION", MSDP_VAL, "\003\001TEST\002ING\002DDDD\001BONK\002ING\004", MSDP_TABLE_CLOSE, IAC, SE);

	size += translate_telopts(d, input, strlen(input), &output[size]);

	msdp_send_update(d);
*/

	return 0;
}

/*
	Send response generated above to recv_sb_mssp() in client.c for interpretation
*/

int write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length)
{
	if (d->descriptor)
	{
		if (d->mccp2)
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
