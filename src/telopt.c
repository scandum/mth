/***************************************************************************
 * Mud Telopt Handler 1.5 by Igor van den Hoven                  2009-2019 *
 ***************************************************************************/

#include "mud.h"
#include "mth.h"

#define TELOPT_DEBUG 1

void        debug_telopts            ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_eor           ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_will_ttype       ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_ttype_is      ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_naws          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_will_new_environ ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_new_environ   ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_charset       ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_charset       ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_mssp          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_msdp          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_msdp          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_gmcp          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_gmcp          ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_do_mccp2         ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_dont_mccp2       ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         skip_sb                  ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );

void        end_mccp2                ( DESCRIPTOR_DATA *d );

int         start_mccp2              ( DESCRIPTOR_DATA *d );
void        process_mccp2            ( DESCRIPTOR_DATA *d );

void        end_mccp3                ( DESCRIPTOR_DATA *d );
int         process_do_mccp3         ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );
int         process_sb_mccp3         ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen );

void        send_ga                  ( DESCRIPTOR_DATA *d );
void        send_eor                 ( DESCRIPTOR_DATA *d );

struct telopt_type
{
	int      size;
	unsigned char   * code;
	int   (* func) (DESCRIPTOR_DATA *d, unsigned char *src, int srclen);
};

const struct telopt_type telopt_table [] =
{
	{ 3, (unsigned char []) { IAC, DO,   TELOPT_EOR, 0 },                       &process_do_eor},

	{ 3, (unsigned char []) { IAC, WILL, TELOPT_TTYPE, 0 },                     &process_will_ttype},
	{ 4, (unsigned char []) { IAC, SB,   TELOPT_TTYPE, ENV_IS, 0 },             &process_sb_ttype_is},

	{ 3, (unsigned char []) { IAC, SB,   TELOPT_NAWS, 0 },                      &process_sb_naws},

	{ 3, (unsigned char []) { IAC, WILL, TELOPT_NEW_ENVIRON, 0 },               &process_will_new_environ},
	{ 3, (unsigned char []) { IAC, SB,   TELOPT_NEW_ENVIRON, 0 },               &process_sb_new_environ},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_CHARSET, 0 },                   &process_do_charset},
	{ 3, (unsigned char []) { IAC, SB,   TELOPT_CHARSET, 0 },                   &process_sb_charset},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_MSSP, 0 },                      &process_do_mssp},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_MSDP, 0 },                      &process_do_msdp},
	{ 3, (unsigned char []) { IAC, SB,   TELOPT_MSDP, 0 },                      &process_sb_msdp},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_GMCP, 0 },                      &process_do_gmcp},
	{ 3, (unsigned char []) { IAC, SB,   TELOPT_GMCP, 0 },                      &process_sb_gmcp},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_MCCP2, 0 },                     &process_do_mccp2},
	{ 3, (unsigned char []) { IAC, DONT, TELOPT_MCCP2, 0 },                     &process_dont_mccp2},

	{ 3, (unsigned char []) { IAC, DO,   TELOPT_MCCP3, 0 },                     &process_do_mccp3},
	{ 5, (unsigned char []) { IAC, SB,   TELOPT_MCCP3, IAC, SE, 0 },            &process_sb_mccp3},

	{ 0, NULL,                 NULL}
};

/*
	Call this to announce support for telopts marked as such in tables.c
*/

void announce_support( DESCRIPTOR_DATA *d)
{
	int i;

	for (i = 0 ; i < 255 ; i++)
	{
		if (telnet_table[i].flags)
		{
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_WILL))
			{
				descriptor_printf(d, "%c%c%c", IAC, WILL, i);
			}
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_DO))
			{
				descriptor_printf(d, "%c%c%c", IAC, DO, i);
			}
		}
	}
}

/*
	Call this right before a copyover to reset the telnet state
*/

void unannounce_support( DESCRIPTOR_DATA *d )
{
	int i;

	end_mccp2(d);
	end_mccp3(d);

	for (i = 0 ; i < 255 ; i++)
	{
		if (telnet_table[i].flags)
		{
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_WILL))
			{
				descriptor_printf(d, "%c%c%c", IAC, WONT, i);
			}
			if (HAS_BIT(telnet_table[i].flags, ANNOUNCE_DO))
			{
				descriptor_printf(d, "%c%c%c", IAC, DONT, i);
			}
		}
	}
}

/*
	This is the main routine that strips out and handles telopt negotiations.
	It also deals with \r and \0 so commands are separated by a single \n.
*/

int translate_telopts(DESCRIPTOR_DATA *d, unsigned char *src, int srclen, unsigned char *out, int outlen)
{
	int cnt, skip;
	unsigned char *pti, *pto;

	pti = src;
	pto = out + outlen;

	if (srclen > 0 && d->mth->mccp3)
	{
		d->mth->mccp3->next_in   = pti;
		d->mth->mccp3->avail_in  = srclen;

		d->mth->mccp3->next_out   = mud->mccp_buf;
		d->mth->mccp3->avail_out  = mud->mccp_len;

		inflate:

		switch (inflate(d->mth->mccp3, Z_SYNC_FLUSH))
		{
			case Z_BUF_ERROR:
				if (d->mth->mccp3->avail_out == 0)
				{
					mud->mccp_len *= 2;
					mud->mccp_buf  = (unsigned char *) realloc(mud->mccp_buf, mud->mccp_len);

					d->mth->mccp3->avail_out = mud->mccp_len / 2;
					d->mth->mccp3->next_out  = mud->mccp_buf + mud->mccp_len / 2;

					goto inflate;
				}
				else
				{
					descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_MCCP3);
					inflateEnd(d->mth->mccp3);
					free(d->mth->mccp3);
					d->mth->mccp3 = NULL;
					srclen = 0;
				}
				break;

			case Z_OK:
				if (d->mth->mccp3->avail_out == 0)
				{
					mud->mccp_len *= 2;
					mud->mccp_buf  = (unsigned char *) realloc(mud->mccp_buf, mud->mccp_len);

					d->mth->mccp3->avail_out = mud->mccp_len / 2;
					d->mth->mccp3->next_out  = mud->mccp_buf + mud->mccp_len / 2;

					goto inflate;
				}
				srclen = d->mth->mccp3->next_out - mud->mccp_buf;
				pti = mud->mccp_buf;

				if (srclen + outlen > MAX_INPUT_LENGTH)
				{
					srclen = MAX_INPUT_LENGTH - outlen - 1;
				}
				break;

			case Z_STREAM_END:
				log_descriptor_printf(d, "MCCP3: Compression end, disabling MCCP3.");

				skip = d->mth->mccp3->next_out - mud->mccp_buf;

				pti += (srclen - d->mth->mccp3->avail_in);
				srclen = d->mth->mccp3->avail_in;

				inflateEnd(d->mth->mccp3);
				free(d->mth->mccp3);
				d->mth->mccp3 = NULL;

				while (skip + srclen + 1 > mud->mccp_len)
				{
					mud->mccp_len *= 2;
					mud->mccp_buf  = (unsigned char *) realloc(mud->mccp_buf, mud->mccp_len);
				}
				memcpy(mud->mccp_buf + skip, pti, srclen);
				pti = mud->mccp_buf;
				srclen += skip;
				break;

			default:
				log_descriptor_printf(d, "MCCP3: Compression error, disabling MCCP3.");
				descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_MCCP3);
				inflateEnd(d->mth->mccp3);
				free(d->mth->mccp3);
				d->mth->mccp3 = NULL;
				srclen = 0;
				break;
		}
	}

	// packet patching

	if (d->mth->teltop)
	{
		if (d->mth->teltop + srclen + 1 < MAX_INPUT_LENGTH)
		{
			memcpy(d->mth->telbuf + d->mth->teltop, pti, srclen);

			srclen += d->mth->teltop;

			pti = (unsigned char *) d->mth->telbuf;
		}
		else
		{
			// You can close the socket here for input spamming
		}
		d->mth->teltop = 0;
	}

	while (srclen > 0)
	{
		switch (*pti)
		{
			case IAC:
				skip = 2;

				debug_telopts(d, pti, srclen);

				for (cnt = 0 ; telopt_table[cnt].code ; cnt++)
				{
					if (srclen < telopt_table[cnt].size)
					{
						if (!memcmp(pti, telopt_table[cnt].code, srclen))
						{
							skip = telopt_table[cnt].size;

							break;
						}
					}
					else
					{
						if (!memcmp(pti, telopt_table[cnt].code, telopt_table[cnt].size))
						{
							skip = telopt_table[cnt].func(d, pti, srclen);

							if (telopt_table[cnt].func == process_sb_mccp3)
							{
								return translate_telopts(d, pti + skip, srclen - skip, out, pto - out);
							}
							break;
						}
					}
				}

				if (telopt_table[cnt].code == NULL && srclen > 1)
				{
					switch (pti[1])
					{
						case WILL:
						case DO:
						case WONT:
						case DONT:
							skip = 3;
							break;

						case SB:
							skip = skip_sb(d, pti, srclen);
							break;

						case IAC:
							*pto++ = *pti++;
							srclen--;
							skip = 1;
							break;

						default:
							if (TELCMD_OK(pti[1]))
							{
								skip = 2;
							}
							else
							{
								skip = 1;
							}
							break;
					}
				}

				if (skip <= srclen)
				{
					pti += skip;
					srclen -= skip;
				}
				else
				{
					memcpy(d->mth->telbuf, pti, srclen);
					d->mth->teltop = srclen;

					*pto = 0;
					return strlen((char *) out);
				}
				break;

			case '\r':
				if (srclen > 1 && pti[1] == '\0')
				{
					*pto++ = '\n';
				}
				pti++;
				srclen--;
				break;

			case '\0':
				pti++;
				srclen--;
				break;

			default:
				*pto++ = *pti++;
				srclen--;
				break;
		}
	}
	*pto = 0;

	// handle remote echo for windows telnet

	if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_REMOTEECHO))
	{
		skip = strlen((char *) out);

		for (cnt = 0 ; cnt < skip ; cnt++)
		{
			switch (out[cnt])
			{
				case   8:
				case 127:
					out[cnt] = '\b';
					write_to_descriptor(d, "\b \b", 3);
					break;

				case '\n':
					write_to_descriptor(d, "\r\n", 2);
					break;

				default:
					if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD))
					{
						write_to_descriptor(d, "*", 1);
					}
					else
					{
						write_to_descriptor(d, (char *) out + cnt, 1);
					}
					break;
			}
		}
	}
	return strlen((char *) out);
}

void debug_telopts( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	if (srclen > 1 && TELOPT_DEBUG)
	{
		switch(src[1])
		{
			case IAC:
				log_descriptor_printf(d, "RCVD IAC IAC");
				break;

			case DO:
			case DONT:
			case WILL:
			case WONT:
			case SB:
				if (srclen > 2)
				{
					if (src[1] == SB)
					{
						if (skip_sb(d, src, srclen) == srclen + 1)
						{
							log_descriptor_printf(d, "RCVD IAC SB %s ?", TELOPT(src[2]));
						}
						else
						{
							log_descriptor_printf(d, "RCVD IAC SB %s IAC SE", TELOPT(src[2]));
						}
					}
					else
					{
						log_descriptor_printf(d, "RCVD IAC %s %s", TELCMD(src[1]), TELOPT(src[2]));
					}
				}
				else
				{
					log_descriptor_printf(d, "RCVD IAC %s ?", TELCMD(src[1]));
				}
				break;

			default:
				if (TELCMD_OK(src[1]))
				{
					log_descriptor_printf(d, "RCVD IAC %s", TELCMD(src[1]));
				}
				else
				{
					log_descriptor_printf(d, "RCVD IAC %d", src[1]);
				}
				break;
		}
	}
	else
	{
		log_descriptor_printf(d, "RCVD IAC ?");
	}
}

/*
	Send to client to have it disable local echo
*/

void send_echo_off( DESCRIPTOR_DATA *d )
{
	SET_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD);

	descriptor_printf(d, "%c%c%c", IAC, WILL, TELOPT_ECHO);
}

/*
	Send to client to have it enable local echo
*/

void send_echo_on( DESCRIPTOR_DATA *d )
{
	DEL_BIT(d->mth->comm_flags, COMM_FLAG_PASSWORD);

	descriptor_printf(d, "%c%c%c", IAC, WONT, TELOPT_ECHO);
}

/*
	Send right after the prompt to mark it as such.
*/

void send_eor( DESCRIPTOR_DATA *d )
{
	if (HAS_BIT(d->mth->comm_flags, COMM_FLAG_EOR))
	{
		descriptor_printf(d, "%c%c", IAC, EOR);
	}
}

/*
	End Of Record negotiation - not enabled by default in tables.c
*/

int process_do_eor( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	SET_BIT(d->mth->comm_flags, COMM_FLAG_EOR);

	return 3;
}

/*
	Terminal Type negotiation - make sure d->mth->terminal_type is initialized.
*/

int process_will_ttype( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	if (*d->mth->terminal_type == 0)
	{
		// Request the first three terminal types to see if MTTS is supported, next reset to default.

		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c%c%c%c", IAC, SB, TELOPT_TTYPE, ENV_SEND, IAC, SE);
		descriptor_printf(d, "%c%c%c", IAC, DONT, TELOPT_TTYPE);
	}
	return 3;
}

int process_sb_ttype_is( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char val[MAX_INPUT_LENGTH];
	char *pto;
	int i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	pto = val;

	for (i = 4 ; i < srclen && src[i] != SE ; i++)
	{
		switch (src[i])
		{
			default:			
				*pto++ = src[i];
				break;

			case IAC:
				*pto = 0;

				if (TELOPT_DEBUG)
				{
					log_descriptor_printf(d, "INFO IAC SB TTYPE RCVD VAL %s.", val);
				}

				if (*d->mth->terminal_type == 0)
				{
					RESTRING(d->mth->terminal_type, val);
				}
				else
				{
					if (sscanf(val, "MTTS %lld", &d->mth->mtts) == 1)
					{
						if (HAS_BIT(d->mth->mtts, MTTS_FLAG_256COLORS))
						{
							SET_BIT(d->mth->comm_flags, COMM_FLAG_256COLORS);
						}

						if (HAS_BIT(d->mth->mtts, MTTS_FLAG_UTF8))
						{
							SET_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
						}
					}

					if (strstr(val, "-256color") || strstr(val, "-256COLOR") || strcasecmp(val, "xterm"))
					{
						SET_BIT(d->mth->comm_flags, COMM_FLAG_256COLORS);
					}
				}
				break;
		}
	}
	return i + 1;
}

/*
	NAWS: Negotiate About Window Size
*/

int process_sb_naws( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	int i, j;

	d->mth->cols = d->mth->rows = 0;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	for (i = 3, j = 0 ; i < srclen && j < 4 ; i++, j++)
	{
		switch (j)
		{
			case 0:
				d->mth->cols += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
				break;
			case 1:
				d->mth->cols += (src[i] == IAC) ? src[i++] : src[i];
				break;
			case 2:
				d->mth->rows += (src[i] == IAC) ? src[i++] * 256 : src[i] * 256;
				break;
			case 3:
				d->mth->rows += (src[i] == IAC) ? src[i++] : src[i];
				break;
		}
	}

	if (TELOPT_DEBUG)
	{
		log_descriptor_printf(d, "INFO IAC SB NAWS RCVD ROWS %d COLS %d", d->mth->rows, d->mth->cols);
	}

	return skip_sb(d, src, srclen);
}

/*
	NEW ENVIRON, used here to discover Windows telnet.
*/

int process_will_new_environ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	descriptor_printf(d, "%c%c%c%c%c%s%c%c", IAC, SB, TELOPT_NEW_ENVIRON, ENV_SEND, ENV_VAR, "SYSTEMTYPE", IAC, SE);

	return 3;
}

int process_sb_new_environ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char var[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
	char *pto;
	int i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	var[0] = val[0] = 0;

	i = 4;

	while (i < srclen && src[i] != SE)
	{
		switch (src[i])
		{
			case ENV_VAR:
			case ENV_USR:
				i++;
				pto = var;

				while (i < srclen && src[i] >= 32 && src[i] != IAC)
				{
					*pto++ = src[i++];
				}
				*pto = 0;

				if (src[i] != ENV_VAL)
				{
					log_descriptor_printf(d, "INFO IAC SB NEW-ENVIRON RCVD %d VAR %s", src[3], var);
				}
				break;

			case ENV_VAL:
				i++;
				pto = val;

				while (i < srclen && src[i] >= 32 && src[i] != IAC)
				{
					*pto++ = src[i++];
				}
				*pto = 0;

				if (TELOPT_DEBUG)
				{
					log_descriptor_printf(d, "INFO IAC SB NEW-ENVIRON RCVD %d VAR %s VAL %s", src[3], var, val);
				}

				if (src[3] == ENV_IS)
				{
					// Detect Windows telnet and enable remote echo.

					if (!strcasecmp(var, "SYSTEMTYPE") && !strcasecmp(val, "WIN32"))
					{
						if (!strcasecmp(d->mth->terminal_type, "ANSI"))
						{
							SET_BIT(d->mth->comm_flags, COMM_FLAG_REMOTEECHO);

							RESTRING(d->mth->terminal_type, "WINDOWS TELNET");
						}
					}

					// Get the real IP address when connecting to mudportal and other MTTS compliant proxies.

					if (!strcasecmp(var, "IPADDRESS"))
					{
						RESTRING(d->mth->proxy, val);
					}
				}
				break;

			default:
				i++;
				break;
		}
	}
	return i + 1;
}

/*
	CHARSET, used to detect UTF-8 support
*/

int process_do_charset( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	descriptor_printf(d, "%c%c%c%c%c%s%c%c", IAC, SB, TELOPT_CHARSET, CHARSET_REQUEST, ' ', "UTF-8", IAC, SE);

	return 3;
}

int process_sb_charset( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char val[MAX_INPUT_LENGTH];
	char *pto;
	int i;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	val[0] = 0;

	i = 5;

	while (i < srclen && src[i] != SE && src[i] != src[4])
	{
		pto = val;

		while (i < srclen && src[i] != src[4] && src[i] != IAC)
		{
			*pto++ = src[i++];
		}
		*pto = 0;

		if (TELOPT_DEBUG)
		{
			log_descriptor_printf(d, "INFO IAC SB CHARSET RCVD %d VAL %s", src[3], val);
		}

		if (src[3] == CHARSET_ACCEPTED)
		{
			if (!strcasecmp(val, "UTF-8"))
			{
				SET_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
			}
		}
		else if (src[3] == CHARSET_REJECTED)
		{
			if (!strcasecmp(val, "UTF-8"))
			{
				DEL_BIT(d->mth->comm_flags, COMM_FLAG_UTF8);
			}
		}
		i++;
	}
	return i + 1;
}

/*
	MSDP: Mud Server Status Protocol

	http://tintin.sourceforge.net/msdp
*/

int process_do_msdp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	int index;

	if (d->mth->msdp_data)
	{
		return 3;
	}

	d->mth->msdp_data = (struct msdp_data **) calloc(mud->msdp_table_size, sizeof(struct msdp_data *));

	for (index = 0 ; index < mud->msdp_table_size ; index++)
	{
		d->mth->msdp_data[index] = (struct msdp_data *) calloc(1, sizeof(struct msdp_data));

		d->mth->msdp_data[index]->flags = msdp_table[index].flags;
		d->mth->msdp_data[index]->value = strdup("");
	}

	log_descriptor_printf(d, "INFO MSDP INITIALIZED");

	// Easiest to handle variable initialization here.

	msdp_update_var(d, "SPECIFICATION", "%s", "http://tintin.sourceforge.net/msdp");

	return 3;
}

int process_sb_msdp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char var[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
	char *pto;
	int i, nest;

	if (skip_sb(d, src, srclen) > srclen)
	{
		return srclen + 1;
	}

	var[0] = val[0] = 0;

	i = 3;
	nest = 0;

	while (i < srclen && src[i] != SE)
	{
		switch (src[i])
		{
			case MSDP_VAR:
				i++;
				pto = var;

				while (i < srclen && src[i] != MSDP_VAL && src[i] != IAC)
				{
					*pto++ = src[i++];
				}
				*pto = 0;

				break;

			case MSDP_VAL:
				i++;
				pto = val;

				while (i < srclen && src[i] != IAC)
				{
					if (src[i] == MSDP_TABLE_OPEN || src[i] == MSDP_ARRAY_OPEN)
					{
						nest++;
					}
					else if (src[i] == MSDP_TABLE_CLOSE || src[i] == MSDP_ARRAY_CLOSE)
					{
						nest--;
					}
					else if (nest == 0 && (src[i] == MSDP_VAR || src[i] == MSDP_VAL))
					{
						break;
					}
					*pto++ = src[i++];
				}
				*pto = 0;

				if (nest == 0)
				{
					process_msdp_varval(d, var, val);
				}
				break;

			default:
				i++;
				break;
		}
	}
	return i + 1;
}

// MSDP over GMCP

int process_do_gmcp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	if (d->mth->msdp_data)
	{
		return 3;
	}
	log_descriptor_printf(d, "INFO MSDP OVER GMCP INITIALIZED");

	SET_BIT(d->mth->comm_flags, COMM_FLAG_GMCP);

	return process_do_msdp(d, src, srclen);
}

int process_sb_gmcp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char out[MAX_INPUT_LENGTH];
	int outlen, skiplen;

	skiplen = skip_sb(d, src, srclen);

	if (skiplen > srclen)
	{
		return srclen + 1;
	}

	outlen = json2msdp(src, srclen, out);

	process_sb_msdp(d, (unsigned char *) out, outlen);

	return skiplen;
}

/*
	MSSP: Mud Server Status Protocol

	http://tintin.sourceforge.net/mssp

	Uncomment and update as needed
*/

int process_do_mssp( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	char buffer[MAX_STRING_LENGTH] = { 0 };

	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "NAME",              MSSP_VAL, "MTH 1.5"); // Change to your Mud's name
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PLAYERS",           MSSP_VAL, mud->total_plr);
	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "UPTIME",            MSSP_VAL, mud->boot_time);

//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "HOSTNAME",          MSSP_VAL, "example.com");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "PORT",              MSSP_VAL, "4321");

//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CODEBASE",          MSSP_VAL, "MTH 1.5");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CONTACT",           MSSP_VAL, "mud@example.com");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CRAWL DELAY",       MSSP_VAL, "-1");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "CREATED",           MSSP_VAL, "2009");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "ICON",              MSSP_VAL, "http://example.com/icon.gif");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LANGUAGE",          MSSP_VAL, "English");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "LOCATION",          MSSP_VAL, "United States of America");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "MINIMUM AGE",       MSSP_VAL, "13");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "WEBSITE",           MSSP_VAL, "http://example.com");

//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "FAMILY",            MSSP_VAL, "DikuMUD");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GENRE",             MSSP_VAL, "Fantasy");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMEPLAY",          MSSP_VAL, "Hack and Slash");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "GAMESYSTEM",        MSSP_VAL, "Custom");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "INTERMUD",          MSSP_VAL, "");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "STATUS",            MSSP_VAL, "Live");
//	cat_sprintf(buffer, "%c%s%c%s", MSSP_VAR, "SUBGENRE",          MSSP_VAL, "High Fantasy");

//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "AREAS",             MSSP_VAL, mud->top_area);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HELPFILES",         MSSP_VAL, mud->top_help);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MOBILES",           MSSP_VAL, mud->top_mob_index);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "OBJECTS",           MSSP_VAL, mud->top_obj_index);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ROOMS",             MSSP_VAL, mud->top_room);

//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "CLASSES",           MSSP_VAL, MAX_CLASS);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "LEVELS",            MSSP_VAL, MAX_LEVEL);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "RACES",             MSSP_VAL, MAX_RACE);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "SKILLS",            MSSP_VAL, MAX_SKILL);

//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "ANSI",              MSSP_VAL, 1);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCCP",              MSSP_VAL, 1);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MCP",               MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MSDP",              MSSP_VAL, 1);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MSP",               MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "MXP",               MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PUEBLO",            MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "UTF-8",             MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "VT100",             MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "XTERM 256 COLORS",  MSSP_VAL, 0);

//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY TO PLAY",       MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "PAY FOR PERKS",     MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING BUILDERS",   MSSP_VAL, 0);
//	cat_sprintf(buffer, "%c%s%c%d", MSSP_VAR, "HIRING CODERS",     MSSP_VAL, 0);

	descriptor_printf(d, "%c%c%c%s%c%c", IAC, SB, TELOPT_MSSP, buffer, IAC, SE);

	return 3;
}

/*
	MCCP: Mud Client Compression Protocol
*/

void *zlib_alloc( void *opaque, unsigned int items, unsigned int size )
{
	return calloc(items, size);
}


void zlib_free( void *opaque, void *address ) 
{
	free(address);
}


int start_mccp2( DESCRIPTOR_DATA *d )
{
	z_stream *stream;

	if (d->mth->mccp2)
	{
		return TRUE;
	}

	stream = calloc(1, sizeof(z_stream));

	stream->next_in	    = NULL;
	stream->avail_in    = 0;

	stream->next_out    = mud->mccp_buf;
	stream->avail_out   = mud->mccp_len;

	stream->data_type   = Z_ASCII;
	stream->zalloc      = zlib_alloc;
	stream->zfree       = zlib_free;
	stream->opaque      = Z_NULL;

	/*
		12, 5 = 32K of memory, more than enough
	*/

	if (deflateInit2(stream, Z_BEST_COMPRESSION, Z_DEFLATED, 12, 5, Z_DEFAULT_STRATEGY) != Z_OK)
	{
		log_descriptor_printf(d, "start_mccp2: failed deflateInit2");
		free(stream);

		return FALSE;
	}

	descriptor_printf(d, "%c%c%c%c%c", IAC, SB, TELOPT_MCCP2, IAC, SE);

	/*
		The above call must send all pending output to the descriptor, since from now on we'll be compressing.
	*/

	d->mth->mccp2 = stream;

	return TRUE;
}


void end_mccp2( DESCRIPTOR_DATA *d )
{
	if (d->mth->mccp2 == NULL)
	{
		return;
	}

	d->mth->mccp2->next_in	 = NULL;
	d->mth->mccp2->avail_in	 = 0;

	d->mth->mccp2->next_out	 = mud->mccp_buf;
	d->mth->mccp2->avail_out = mud->mccp_len;

	if (deflate(d->mth->mccp2, Z_FINISH) != Z_STREAM_END)
	{
		log_descriptor_printf(d, "end_mccp2: failed to deflate");
	}

	if (!HAS_BIT(d->mth->comm_flags, COMM_FLAG_DISCONNECT))
	{
		process_mccp2(d);
	}

	if (deflateEnd(d->mth->mccp2) != Z_OK)
	{
		log_descriptor_printf(d, "end_mccp2: failed to deflateEnd");
	}

	free(d->mth->mccp2);

	d->mth->mccp2 = NULL;

	log_descriptor_printf(d, "MCCP2: COMPRESSION END");

	return;
}


void write_mccp2( DESCRIPTOR_DATA *d, char *txt, int length)
{
	d->mth->mccp2->next_in    = (unsigned char *) txt;
	d->mth->mccp2->avail_in   = length;

	d->mth->mccp2->next_out   = (unsigned char *) mud->mccp_buf;
	d->mth->mccp2->avail_out  = mud->mccp_len;

	if (deflate(d->mth->mccp2, Z_SYNC_FLUSH) != Z_OK)
	{
		return;
	}

	process_mccp2(d);

	return;
}


void process_mccp2( DESCRIPTOR_DATA *d )
{
	if (write(d->descriptor, mud->mccp_buf, mud->mccp_len - d->mth->mccp2->avail_out) < 1)
	{
		perror("write in process_mccp2");

		SET_BIT(d->mth->comm_flags, COMM_FLAG_DISCONNECT);
	}
}

int process_do_mccp2( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	start_mccp2(d);

	return 3;
}


int process_dont_mccp2( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	end_mccp2(d);

	return 3;
}

// MCCP3

int process_do_mccp3( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	return 3;
}

int process_sb_mccp3( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	if (d->mth->mccp3)
	{
		end_mccp3(d);
	}

	d->mth->mccp3 = (z_stream *) calloc(1, sizeof(z_stream));

	d->mth->mccp3->data_type = Z_ASCII;
	d->mth->mccp3->zalloc    = zlib_alloc;
	d->mth->mccp3->zfree     = zlib_free;
	d->mth->mccp3->opaque    = NULL;

	if (inflateInit(d->mth->mccp3) != Z_OK)
	{
		log_descriptor_printf(d, "INFO IAC SB MCCP3 FAILED TO INITIALIZE");

		descriptor_printf(d, "%c%c%c", IAC, WONT, TELOPT_MCCP3);

		free(d->mth->mccp3);
		d->mth->mccp3 = NULL;
	}
	else
	{
		log_descriptor_printf(d, "INFO IAC SB MCCP3 INITIALIZED");
	}
	return 5;
}

void end_mccp3( DESCRIPTOR_DATA *d )
{
	if (d->mth->mccp3)
	{
		log_descriptor_printf(d, "MCCP3: COMPRESSION END");
		inflateEnd(d->mth->mccp3);
		free(d->mth->mccp3);
		d->mth->mccp3 = NULL;
	}
}

/*
	Returns the length of a telnet subnegotiation, return srclen + 1 for incomplete state.
*/

int skip_sb( DESCRIPTOR_DATA *d, unsigned char *src, int srclen )
{
	int i;

	for (i = 1 ; i < srclen ; i++)
	{
		if (src[i] == SE && src[i-1] == IAC)
		{
			return i + 1;
		}
	}

	return srclen + 1;
}


		
/*
	Utility function
*/

void descriptor_printf( DESCRIPTOR_DATA *d, char *fmt, ... )
{
	char buf[MAX_STRING_LENGTH];
	int size;
	va_list args;

	va_start(args, fmt);

	size = vsprintf(buf, fmt, args);

	va_end(args);

	write_to_descriptor(d, buf, size);
}
