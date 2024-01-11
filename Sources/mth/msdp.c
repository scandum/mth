/***************************************************************************
 * Mud Telopt Handler 1.5 Copyright 2009-2019 Igor van den Hoven           *
 ***************************************************************************/

#include "mud.h"
#include "mth.h"

void        process_msdp_varval           ( DESCRIPTOR_DATA *d, char *var, char *val );
void        msdp_send_update              ( DESCRIPTOR_DATA *d );
void        msdp_update_var               ( DESCRIPTOR_DATA *d, char *var, char *fmt, ... );
void        msdp_update_var_instant       ( DESCRIPTOR_DATA *d, char *var, char *fmt, ... );

void        msdp_configure_arachnos       ( DESCRIPTOR_DATA *d, int index );
void        msdp_configure_pluginid       ( DESCRIPTOR_DATA *d, int index );

void        write_msdp_to_descriptor      ( DESCRIPTOR_DATA *d, char *src, int length );
int         msdp2json                     ( unsigned char *src, int srclen, char *out );
int         json2msdp                     ( unsigned char *src, int srclen, char *out );

// Set table size and check for errors. Call once at startup.

void init_msdp_table(void)
{
	int index;

	for (index = 0 ; *msdp_table[index].name ; index++)
	{
		if (strcmp(msdp_table[index].name, msdp_table[index+1].name) > 0)
		{
			if (*msdp_table[index+1].name)
			{
				log_printf("\e[31minit_msdp_table: Improperly sorted variable: %s.\e0m", msdp_table[index+1].name);
			}
		}
	}
	mud->msdp_table_size = index;
}

// Binary search on the msdp_table.

int msdp_find(char *var)
{
	int val, bot, top, srt;

	bot = 0;
	top = mud->msdp_table_size - 1;
	val = top / 2;

	while (bot <= top)
	{
		srt = strcmp(var, msdp_table[val].name);

		if (srt < 0)
		{
			top = val - 1;
		}
		else if (srt > 0)
		{
			bot = val + 1;
		}
		else
		{
			return val;
		}
		val = bot + (top - bot) / 2;
	}
	return -1;
}

void msdp_update_all(char *var, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	//for each descriptor
	//	msdp_update_var(ses, d, var, "%s", buf);
}

// Update a variable and queue it if it's being reported.

void msdp_update_var(DESCRIPTOR_DATA *d, char *var, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	int index;
	va_list args;

	if (d->mth->msdp_data == NULL)
	{
		return;
	}

	index = msdp_find(var);

	if (index == -1)
	{
		log_printf("msdp_update_var: Unknown variable: %s.", var);

		return;
	}

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	if (strcmp(d->mth->msdp_data[index]->value, buf))
	{
		if (HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_REPORTED))
		{
			SET_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_UPDATED);
			SET_BIT(d->mth->comm_flags, COMM_FLAG_MSDPUPDATE);
		}
		RESTRING(d->mth->msdp_data[index]->value, buf);
	}
}

// Update a variable and send it instantly.

void msdp_update_var_instant(DESCRIPTOR_DATA *d, char *var, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	int index, length;
	va_list args;

	if (d->mth->msdp_data == NULL)
	{
		return;
	}

	index = msdp_find(var);

	if (index == -1)
	{
		log_printf("msdp_update_var_instant: Unknown variable: %s.", var);

		return;
	}

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	if (strcmp(d->mth->msdp_data[index]->value, buf))
	{
		RESTRING(d->mth->msdp_data[index]->value, buf);
	}

	if (HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_REPORTED))
	{
		length = sprintf(out, "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP, MSDP_VAR, msdp_table[index].name, MSDP_VAL, buf, IAC, SE);

		write_msdp_to_descriptor(d, out, length);
	}
}

// Send all reported variables that have been updated.

void msdp_send_update(DESCRIPTOR_DATA *d)
{
	char *ptr, buf[MAX_STRING_LENGTH];
	int index;

	if (d->mth->msdp_data == NULL)
	{
		return;
	}

	ptr = buf;

	ptr += sprintf(ptr, "%c%c%c", IAC, SB, TELOPT_MSDP);

	for (index = 0 ; index < mud->msdp_table_size ; index++)
	{
		if (HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_UPDATED))
		{
			ptr += sprintf(ptr, "%c%s%c%s", MSDP_VAR, msdp_table[index].name, MSDP_VAL, d->mth->msdp_data[index]->value);

			DEL_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_UPDATED);
		}

		if (ptr - buf > MAX_STRING_LENGTH - MAX_INPUT_LENGTH)
		{
			log_descriptor_printf(d, "MSDP BUFFER OVERFLOW");
			break;
		}
	}

	ptr += sprintf(ptr, "%c%c", IAC, SE);

	write_msdp_to_descriptor(d, buf, ptr - buf);

	DEL_BIT(d->mth->comm_flags, COMM_FLAG_MSDPUPDATE);
}


char *msdp_get_var(DESCRIPTOR_DATA *d, char *var)
{
	int index;

	if (d->mth->msdp_data == NULL)
	{
		return NULL;
	}

	index = msdp_find(var);

	if (index == -1)
	{
		log_printf("msdp_get_var: Unknown variable: %s.", var);

		return NULL;
	}

	return d->mth->msdp_data[index]->value;
}

// 1d array support for commands

void process_msdp_index_val(DESCRIPTOR_DATA *d, int var_index, char *val )
{
	int val_index;

	val_index = msdp_find(val);

//	log_printf("process_msdp_index_val(%d, %d, %s)", var_index, val_index, val);

	if (val_index >= 0)
	{
		if (msdp_table[var_index].fun)
		{
			msdp_table[var_index].fun(d, val_index);
		}
	}
} 

// 1d array support for commands

void process_msdp_array( DESCRIPTOR_DATA *d, int var_index, char *val)
{
	char buf[MAX_STRING_LENGTH], *pto, *pti;

	pti = val;
	pto = buf;

	while (*pti)
	{
		switch (*pti)
		{
			case MSDP_ARRAY_OPEN:
				break;

			case MSDP_VAL:
				*pto = 0;

				if (*buf)
				{
					process_msdp_index_val(d, var_index, buf);
				}
				pto = buf;
				break;

			case MSDP_ARRAY_CLOSE:
				*pto = 0;

				if (*buf)
				{
					process_msdp_index_val(d, var_index, buf);
				}
				return;

			default:
				*pto++ = *pti;
				break;
		}
		pti++;
	}
	*pto = 0;

	if (*buf)
	{
		process_msdp_index_val(d, var_index, buf);
	}
}

void process_msdp_varval( DESCRIPTOR_DATA *d, char *var, char *val )
{
	int var_index, val_index;

	if (d->mth->msdp_data == NULL)
	{
		return;
	}

	var_index = msdp_find(var);

	if (var_index == -1)
	{
		return;
	}

	if (HAS_BIT(msdp_table[var_index].flags, MSDP_FLAG_CONFIGURABLE))
	{
		RESTRING(d->mth->msdp_data[var_index]->value, val);

		if (msdp_table[var_index].fun)
		{
			msdp_table[var_index].fun(d, var_index);
		}
		return;
	}

	// Commands only take variables as arguments.

	if (HAS_BIT(msdp_table[var_index].flags, MSDP_FLAG_COMMAND))
	{
		if (*val == MSDP_ARRAY_OPEN)
		{
//			log_printf("msdp_varval: %s array %s.", var, val);

			process_msdp_array(d, var_index, val);
		}
		else
		{
			val_index = msdp_find(val);

			if (val_index == -1)
			{
				return;
			}

			if (msdp_table[var_index].fun)
			{
				msdp_table[var_index].fun(d, val_index);
			}
		}
	}
} 

void msdp_command_list(DESCRIPTOR_DATA *d, int index)
{
	char *ptr, buf[MAX_STRING_LENGTH];
	int flag;

	if (!HAS_BIT(msdp_table[index].flags, MSDP_FLAG_LIST))
	{
		return;
	}

	ptr = buf;

	flag = msdp_table[index].flags;

	ptr += sprintf(ptr, "%c%c%c%c%s%c%c", IAC, SB, TELOPT_MSDP, MSDP_VAR, msdp_table[index].name, MSDP_VAL, MSDP_ARRAY_OPEN);

	for (index = 0 ; index < mud->msdp_table_size ; index++)
	{
		if (flag != MSDP_FLAG_LIST)
		{
			if (HAS_BIT(d->mth->msdp_data[index]->flags, flag) && !HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_LIST))
			{
				ptr += sprintf(ptr, "%c%s", MSDP_VAL, msdp_table[index].name);
			}
		}
		else
		{
			if (HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_LIST))
			{
				ptr += sprintf(ptr, "%c%s", MSDP_VAL, msdp_table[index].name);
			}
		}
	}

	ptr += sprintf(ptr, "%c%c%c", MSDP_ARRAY_CLOSE, IAC, SE);

	write_msdp_to_descriptor(d, buf, ptr - buf);
}

void msdp_command_report(DESCRIPTOR_DATA *d, int index)
{
	if (!HAS_BIT(msdp_table[index].flags, MSDP_FLAG_REPORTABLE))
	{
		return;
	}

	SET_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_REPORTED);

	if (!HAS_BIT(msdp_table[index].flags, MSDP_FLAG_SENDABLE))
	{
		return;
	}

	SET_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_UPDATED);
	SET_BIT(d->mth->comm_flags, COMM_FLAG_MSDPUPDATE);
}

void msdp_command_reset(DESCRIPTOR_DATA *d, int index)
{
	int flag;

	if (!HAS_BIT(msdp_table[index].flags, MSDP_FLAG_LIST))
	{
		return;
	}

	flag = msdp_table[index].flags &= ~MSDP_FLAG_LIST;

	for (index = 0 ; index < mud->msdp_table_size ; index++)
	{
		if (HAS_BIT(d->mth->msdp_data[index]->flags, flag))
		{
			d->mth->msdp_data[index]->flags = msdp_table[index].flags;
		}
	}
}

void msdp_command_send(DESCRIPTOR_DATA *d, int index)
{
	if (HAS_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_SENDABLE))
	{
		SET_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_UPDATED);	
		SET_BIT(d->mth->comm_flags, COMM_FLAG_MSDPUPDATE);
	}
}

void msdp_command_unreport(DESCRIPTOR_DATA *d, int index)
{
	if (!HAS_BIT(msdp_table[index].flags, MSDP_FLAG_REPORTABLE))
	{
		return;
	}

	DEL_BIT(d->mth->msdp_data[index]->flags, MSDP_FLAG_REPORTED);
}

// Comment out if you don't want Arachnos Intermud support

void msdp_configure_arachnos(DESCRIPTOR_DATA *d, int index)
{
	char var[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
	char mud_name[MAX_INPUT_LENGTH], mud_host[MAX_INPUT_LENGTH], mud_port[MAX_INPUT_LENGTH];
	char msg_user[MAX_INPUT_LENGTH], msg_time[MAX_INPUT_LENGTH], msg_body[MAX_INPUT_LENGTH];
	char mud_players[MAX_INPUT_LENGTH], mud_uptime[MAX_INPUT_LENGTH], mud_update[MAX_INPUT_LENGTH];
	char *pti, *pto;

	struct tm timeval_tm;
	time_t timeval_t;

	var[0] = val[0] = mud_name[0] = mud_host[0] = mud_port[0] = msg_user[0] = msg_time[0] = msg_body[0] = mud_players[0] = mud_uptime[0] = mud_update[0] = 0;

	pti = d->mth->msdp_data[index]->value;

	while (*pti)
	{
		switch (*pti)
		{
			case MSDP_VAR:
				pti++;
				pto = var;

				while (*pti > MSDP_ARRAY_CLOSE)
				{
					*pto++ = *pti++;
				}
				*pto = 0;
				break;

			case MSDP_VAL:
				pti++;
				pto = val;

				while (*pti > MSDP_ARRAY_CLOSE)
				{
					*pto++ = *pti++;
				}
				*pto = 0;

				if (!strcmp(var, "MUD_NAME"))
				{
					strcpy(mud_name, val);
				}
				else if (!strcmp(var, "MUD_HOST"))
				{
					strcpy(mud_host, val);
				}
				else if (!strcmp(var, "MUD_PORT"))
				{
					strcpy(mud_port, val);
				}
				else if (!strcmp(var, "MSG_USER"))
				{
					strcpy(msg_user, val);
				}
				else if (!strcmp(var, "MSG_TIME"))
				{
					timeval_t = (time_t) atoll(val);
					timeval_tm = *localtime(&timeval_t);
					
					strftime(msg_time, 20, "%T %D", &timeval_tm);
				}
				else if (!strcmp(var, "MSG_BODY"))
				{
					strcpy(msg_body, val);
				}
				else if (!strcmp(var, "MUD_UPTIME"))
				{
					timeval_t = (time_t) atoll(val);
					timeval_tm = *localtime(&timeval_t);
					
					strftime(mud_uptime, 20, "%T %D", &timeval_tm);
				}
				else if (!strcmp(var, "MUD_UPDATE"))
				{
					timeval_t = (time_t) atoll(val);
					timeval_tm = *localtime(&timeval_t);
					
					strftime(mud_update, 20, "%T %D", &timeval_tm);
				}
				else if (!strcmp(var, "MUD_PLAYERS"))
				{
					strcpy(mud_players, val);
				}
				break;

			default:
				pti++;
				break;
		}
	}

	if (*mud_name && *mud_host && *mud_port)
	{
		if (!strcmp(msdp_table[index].name, "ARACHNOS_DEVEL"))
		{
			if (*msg_user && *msg_time && *msg_body)
			{
				arachnos_devel("%s %s@%s:%s devtalks: %s", msg_time, msg_user, mud_host, mud_port, msg_body);
			}
		}
		else if (!strcmp(msdp_table[index].name, "ARACHNOS_MUDLIST"))
		{
			if (*mud_uptime && *mud_update && *mud_players)
			{
				arachnos_mudlist("%18.18s %14.14s %5.5s %17.17s %17.17s %4.4s", mud_name, mud_host, mud_port, mud_update, mud_uptime, mud_players);
			}
		}
	}
}

void msdp_configure_edit_buffer(DESCRIPTOR_DATA *d, int index)
{
//          editorSetWorkingBuf(d, d->mth->msdp_data[index]->value);
}

// This table needs to stay alphabetically sorted

struct msdp_type msdp_table[] =
{
	{    "ALIGNMENT",                     MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "ARACHNOS_DEVEL",            MSDP_FLAG_CONFIGURABLE|MSDP_FLAG_REPORTABLE,    msdp_configure_arachnos },
	{    "ARACHNOS_MUDLIST",                               MSDP_FLAG_CONFIGURABLE,    msdp_configure_arachnos },
	{    "COMMANDS",                             MSDP_FLAG_COMMAND|MSDP_FLAG_LIST,    NULL },
	{    "CONFIGURABLE_VARIABLES",          MSDP_FLAG_CONFIGURABLE|MSDP_FLAG_LIST,    NULL },
//	{    "EDIT_BUFFER",               MSDP_FLAG_CONFIGURABLE|MSDP_FLAG_REPORTABLE,    msdp_configure_edit_buffer },
	{    "EXPERIENCE",                    MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "EXPERIENCE_MAX",                MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "HEALTH",                        MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "HEALTH_MAX",                    MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "LEVEL",                         MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "LIST",                                                MSDP_FLAG_COMMAND,    msdp_command_list },
	{    "LISTS",                                                  MSDP_FLAG_LIST,    NULL },
	{    "MANA",                          MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "MANA_MAX",                      MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "MONEY",                         MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "MOVEMENT",                      MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "MOVEMENT_MAX",                  MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
        {    "REPORT",                                              MSDP_FLAG_COMMAND,    msdp_command_report },
	{    "REPORTABLE_VARIABLES",              MSDP_FLAG_REPORTABLE|MSDP_FLAG_LIST,    NULL },
	{    "REPORTED_VARIABLES",                  MSDP_FLAG_REPORTED|MSDP_FLAG_LIST,    NULL },
	{    "RESET",                                               MSDP_FLAG_COMMAND,    msdp_command_reset },
	{    "ROOM",                                             MSDP_FLAG_REPORTABLE,    NULL },
	{    "ROOM_EXITS",                    MSDP_FLAG_SENDABLE|MSDP_FLAG_REPORTABLE,    NULL },
	{    "SEND",                                                MSDP_FLAG_COMMAND,    msdp_command_send },
	{    "SENDABLE_VARIABLES",                  MSDP_FLAG_SENDABLE|MSDP_FLAG_LIST,    NULL },
	{    "SPECIFICATION",                                      MSDP_FLAG_SENDABLE,    NULL },
	{    "UNREPORT",                                            MSDP_FLAG_COMMAND,    msdp_command_unreport },

	{    "",                                                                    0,    NULL } // End of table marker
};

void write_msdp_to_descriptor(DESCRIPTOR_DATA *d, char *src, int length)
{
	char out[MAX_STRING_LENGTH];

	if (!HAS_BIT(d->mth->comm_flags, COMM_FLAG_GMCP))
	{
		write_to_descriptor(d, src, length);
	}
	else
	{
		length = msdp2json((unsigned char *) src, length, out);

		write_to_descriptor(d, out, length);
	}
}

int msdp2json(unsigned char *src, int srclen, char *out)
{
	char *pto;
	int i, nest, last;

	nest = last = 0;

	pto = out;

	if (src[2] == TELOPT_MSDP)
	{
		pto += sprintf(pto, "%c%c%cMSDP {", IAC, SB, TELOPT_GMCP);
	}

	i = 3;

	while (i < srclen)
	{
		if (src[i] == IAC && src[i+1] == SE)
		{
			break;
		}

		switch (src[i])
		{
			case MSDP_TABLE_OPEN:
				*pto++ = '{';
				nest++;
				last = MSDP_TABLE_OPEN;
				break;

			case MSDP_TABLE_CLOSE:
				if (last == MSDP_VAL || last == MSDP_VAR)
				{
					*pto++ = '"';
				}
				if (nest)
				{
					nest--;
				}
				*pto++ = '}';
				last = MSDP_TABLE_CLOSE;
				break;

			case MSDP_ARRAY_OPEN:
				*pto++ = '[';
				nest++;
				last = MSDP_ARRAY_OPEN;
				break;

			case MSDP_ARRAY_CLOSE:
				if (last == MSDP_VAL || last == MSDP_VAR)
				{
					*pto++ = '"';
				}
				if (nest)
				{
					nest--;
				}
				*pto++ = ']';
				last = MSDP_ARRAY_CLOSE;
				break;

			case MSDP_VAR:
				if (last == MSDP_VAL || last == MSDP_VAR)
				{
					*pto++ = '"';
				}
				if (last == MSDP_VAL || last == MSDP_VAR || last == MSDP_TABLE_CLOSE || last == MSDP_ARRAY_CLOSE)
				{
					*pto++ = ',';
				}
				*pto++ = '"';
				last = MSDP_VAR;
				break;

			case MSDP_VAL:
				if (last == MSDP_VAR)
				{
					*pto++ = '"';
					*pto++ = ':';
				}
				if (last == MSDP_VAL)
				{
					*pto++ = '"';
					*pto++ = ',';
				}

				if (src[i+1] != MSDP_TABLE_OPEN && src[i+1] != MSDP_ARRAY_OPEN)
				{
					*pto++ = '"';
				}
				last = MSDP_VAL;
				break;

			case '\\':
				*pto++ = '\\';
				*pto++ = '\\';
				break;

			case '"':
				*pto++ = '\\';
				*pto++ = '"';
				break;

			default:
				*pto++ = src[i];
				break;
		}
		i++;
	}

	pto += sprintf(pto, "}%c%c", IAC, SE);

	return pto - out;
}

int json2msdp(unsigned char *src, int srclen, char *out)
{
	char *pto;
	int i, nest, last, type, state[100];

	nest = last = 0;

	pto = out;

	if (src[2] == TELOPT_GMCP)
	{
		pto += sprintf(pto, "%c%c%c", IAC, SB, TELOPT_MSDP);
	}

	i = 3;

	if (!strncmp((char *) &src[3], "MSDP {", 6))
	{
		i += 6;
	}

	state[0] = nest = type = 0;

	while (i < srclen && src[i] != IAC && nest < 99)
	{
		switch (src[i])
		{
			case ' ':
				i++;
				break;

			case '{':
				*pto++ = MSDP_TABLE_OPEN;
				i++;
				state[++nest] = 0;
				break;

			case '}':
				nest--;
				i++;
				if (nest < 0)
				{
					pto += sprintf(pto, "%c%c", IAC, SE);
					return pto - out;
				}
				*pto++ = MSDP_TABLE_CLOSE;
				break;

			case '[':
				i++;
				state[++nest] = 1;
				*pto++ = MSDP_ARRAY_OPEN;
				break;

			case ']':
				nest--;
				i++;
				*pto++ = MSDP_ARRAY_CLOSE;
				break;

			case ':':
				*pto++ = MSDP_VAL;
				i++;
				break;

			case ',':
				i++;
				if (state[nest])
				{
					*pto++ = MSDP_VAL;
				}
				else
				{
					*pto++ = MSDP_VAR;
				}
				break;

			case '"':
				i++;
				if (last == 0)
				{
					last = MSDP_VAR;
					*pto++ = MSDP_VAR;
				}
				type = 1;

				while (i < srclen && src[i] != IAC && type)
				{
					switch (src[i])
					{
						case '\\':
							i++;

							if (i < srclen && src[i] == '"')
							{
								*pto++ = src[i++];
							}
							else
							{
								*pto++ = '\\';
							}
							break;

						case '"':
							i++;
							type = 0;
							break;

						default:
							*pto++ = src[i++];
							break;
					}
				}
				break;

			default:
				type = 1;

				while (i < srclen && src[i] != IAC && type)
				{
					switch (src[i])
					{
						case '}':
						case ']':
						case ',':
						case ':':
							type = 0;
							break;

						case ' ':
							i++;
							break;

						default:
							*pto++ = src[i++];
							break;
					}
				}
				break;
		}
	}
	pto += sprintf(pto, "%c%c", IAC, SE);

	return pto - out;
}
