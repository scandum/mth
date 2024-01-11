#include "mud.h"
#include "mth.h"

void init_mth(void)
{
	mud = calloc(1, sizeof(MUD_DATA));

	mud->mccp_len = COMPRESS_BUF_SIZE;
	mud->mccp_buf = calloc(COMPRESS_BUF_SIZE, sizeof(unsigned char));

	// Initialize the MSDP table

	init_msdp_table();
}

void init_mth_socket(DESCRIPTOR_DATA *d)
{
	d->mth = calloc(1, sizeof(MTH_DATA));
	d->mth->proxy = (char *) strdup("");
	d->mth->terminal_type = (char *) strdup("");

	announce_support(d);
}

void uninit_mth_socket(DESCRIPTOR_DATA *d)
{
	unannounce_support(d);

	free(d->mth->proxy);
	free(d->mth->terminal_type);
	free(d->mth);
}



void arachnos_devel(char *fmt, ...)
{
	// mud specific broadcast goes here
}

void arachnos_mudlist(char *fmt, ...)
{
	// mud specific mudlist handler goes here
}


// Utility functions

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


int cat_sprintf(char *dest, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	int size;
	va_list args;

	va_start(args, fmt);
	size = vsprintf(buf, fmt, args);
	va_end(args);

	strcat(dest, buf);

	return size;
}

char *telcmds[] =
{
	"EOF",   "SUSP",  "ABORT", "EOR",   "SE",
	"NOP",   "DMARK", "BRK",   "IP",    "AO",
	"AYT",   "EC",    "EL",    "GA",    "SB",
	"WILL",  "WONT",  "DO",    "DONT",  "IAC"
};


struct telnet_type telnet_table[] =
{
	{    "BINARY",                0 },
	{    "ECHO",                  0 }, /* Local echo */
	{    "RCP",                   0 },
	{    "SUPPRESS GA",           0 }, /* Character mode */
	{    "NAME",                  0 },
	{    "STATUS",                0 },
	{    "TIMING MARK",           0 },
	{    "RCTE",                  0 },
	{    "NAOL",                  0 },
	{    "NAOP",                  0 },
	{    "NAORCD",                0 },
	{    "NAOHTS",                0 },
	{    "NAOHTD",                0 },
	{    "NAOFFD",                0 },
	{    "NAOVTS",                0 },
	{    "NAOVTD",                0 },
	{    "NAOLFD",                0 },
	{    "EXTEND ASCII",          0 },
	{    "LOGOUT",                0 },
	{    "BYTE MACRO",            0 },
	{    "DATA ENTRY TERML",      0 },
	{    "SUPDUP",                0 },
	{    "SUPDUP OUTPUT",         0 },
	{    "SEND LOCATION",         0 },
	{    "TERMINAL TYPE",         ANNOUNCE_DO }, /* Terminal Type */
	{    "EOR",                   0 }, /* End of Record */
	{    "TACACS UID",            0 },
	{    "OUTPUT MARKING",        0 },
	{    "TTYLOC",                0 },
	{    "3270 REGIME",           0 },
	{    "X.3 PAD",               0 },
	{    "NAWS",                  ANNOUNCE_DO }, /* Negotiate About Window Size */
	{    "TSPEED",                0 }, 
	{    "LFLOW",                 0 },
	{    "LINEMODE",              0 },
	{    "XDISPLOC",              0 },
	{    "OLD_ENVIRON",           0 },
	{    "AUTH",                  0 },
	{    "ENCRYPT",               0 },
	{    "NEW_ENVIRON",           ANNOUNCE_DO }, /* Used to detect Win32 telnet */
	{    "TN3270E",               0 },
	{    "XAUTH",                 0 },
	{    "CHARSET",               ANNOUNCE_WILL }, /* Used to detect UTF-8 support */
	{    "RSP",                   0 },
	{    "COM PORT",              0 },
	{    "SLE",                   0 },
	{    "STARTTLS",              0 },
	{    "KERMIT",                0 },
	{    "SEND-URL",              0 },
	{    "FORWARD_X",             0 },
	{    "50",                    0 },
	{    "51",                    0 },
	{    "52",                    0 },
	{    "53",                    0 },
	{    "54",                    0 },
	{    "55",                    0 },
	{    "56",                    0 },
	{    "57",                    0 },
	{    "58",                    0 },
	{    "59",                    0 },
	{    "60",                    0 },
	{    "61",                    0 },
	{    "62",                    0 },
	{    "63",                    0 },
	{    "64",                    0 },
	{    "65",                    0 },
	{    "66",                    0 },
	{    "67",                    0 },
	{    "68",                    0 },
	{    "MSDP",                  ANNOUNCE_WILL }, // Mud Server Data Protocol
	{    "MSSP",                  ANNOUNCE_WILL }, // Mud Server Status Protocol
	{    "71",                    0 },
	{    "72",                    0 },
	{    "73",                    0 },
	{    "74",                    0 },
	{    "75",                    0 },
	{    "76",                    0 },
	{    "77",                    0 },
	{    "78",                    0 },
	{    "79",                    0 },
	{    "80",                    0 },
	{    "81",                    0 },
	{    "82",                    0 },
	{    "83",                    0 },
	{    "84",                    0 },
	{    "MCCP1",                 0 },             /* Obsolete - don't use. */
	{    "MCCP2",                 ANNOUNCE_WILL }, /* Mud Client Compression Protocol v2 */
	{    "MCCP3",                 ANNOUNCE_WILL }, /* Mud Client Compression Protocol v3 */
	{    "88",                    0 },
	{    "89",                    0 },
	{    "MSP",                   0 },
	{    "MXP",                   0 },
	{    "MSP2",                  0 }, /* Unadopted */
	{    "ZMP",                   0 }, /* Unadopted */
	{    "94",                    0 },
	{    "95",                    0 },
	{    "96",                    0 },
	{    "97",                    0 },
	{    "98",                    0 },
	{    "99",                    0 },
	{    "100",                   0 },
	{    "101",                   0 },
	{    "102",                   0 }, /* Obsolete - Aardwolf */
	{    "103",                   0 },
	{    "104",                   0 },
	{    "105",                   0 },
	{    "106",                   0 },
	{    "107",                   0 },
	{    "108",                   0 },
	{    "109",                   0 },
	{    "110",                   0 },
	{    "111",                   0 },
	{    "112",                   0 },
	{    "113",                   0 },
	{    "114",                   0 },
	{    "115",                   0 },
	{    "116",                   0 },
	{    "117",                   0 },
	{    "118",                   0 },
	{    "119",                   0 },
	{    "120",                   0 },
	{    "121",                   0 },
	{    "122",                   0 },
	{    "123",                   0 },
	{    "124",                   0 },
	{    "125",                   0 },
	{    "126",                   0 },
	{    "127",                   0 },
	{    "128",                   0 },
	{    "129",                   0 },
	{    "130",                   0 },
	{    "131",                   0 },
	{    "132",                   0 },
	{    "133",                   0 },
	{    "134",                   0 },
	{    "135",                   0 },
	{    "136",                   0 },
	{    "137",                   0 },
	{    "138",                   0 },
	{    "139",                   0 },
	{    "140",                   0 },
	{    "141",                   0 },
	{    "142",                   0 },
	{    "143",                   0 },
	{    "144",                   0 },
	{    "145",                   0 },
	{    "146",                   0 },
	{    "147",                   0 },
	{    "148",                   0 },
	{    "149",                   0 },
	{    "150",                   0 },
	{    "151",                   0 },
	{    "152",                   0 },
	{    "153",                   0 },
	{    "154",                   0 },
	{    "155",                   0 },
	{    "156",                   0 },
	{    "157",                   0 },
	{    "158",                   0 },
	{    "159",                   0 },
	{    "160",                   0 },
	{    "161",                   0 },
	{    "162",                   0 },
	{    "163",                   0 },
	{    "164",                   0 },
	{    "165",                   0 },
	{    "166",                   0 },
	{    "167",                   0 },
	{    "168",                   0 },
	{    "169",                   0 },
	{    "170",                   0 },
	{    "171",                   0 },
	{    "172",                   0 },
	{    "173",                   0 },
	{    "174",                   0 },
	{    "175",                   0 },
	{    "176",                   0 },
	{    "177",                   0 },
	{    "178",                   0 },
	{    "179",                   0 },
	{    "180",                   0 },
	{    "181",                   0 },
	{    "182",                   0 },
	{    "183",                   0 },
	{    "184",                   0 },
	{    "185",                   0 },
	{    "186",                   0 },
	{    "187",                   0 },
	{    "188",                   0 },
	{    "189",                   0 },
	{    "190",                   0 },
	{    "191",                   0 },
	{    "192",                   0 },
	{    "193",                   0 },
	{    "194",                   0 },
	{    "195",                   0 },
	{    "196",                   0 },
	{    "197",                   0 },
	{    "198",                   0 },
	{    "199",                   0 },
	{    "ATCP",                  0 }, /* Obsolete - Achaea */
	{    "GMCP",                  ANNOUNCE_WILL }, /* Generic Mud Communication Protocol */
	{    "202",                   0 },
	{    "203",                   0 },
	{    "204",                   0 },
	{    "205",                   0 },
	{    "206",                   0 },
	{    "207",                   0 },
	{    "208",                   0 },
	{    "209",                   0 },
	{    "210",                   0 },
	{    "211",                   0 },
	{    "212",                   0 },
	{    "213",                   0 },
	{    "214",                   0 },
	{    "215",                   0 },
	{    "216",                   0 },
	{    "217",                   0 },
	{    "218",                   0 },
	{    "219",                   0 },
	{    "220",                   0 },
	{    "221",                   0 },
	{    "222",                   0 },
	{    "223",                   0 },
	{    "224",                   0 },
	{    "225",                   0 },
	{    "226",                   0 },
	{    "227",                   0 },
	{    "228",                   0 },
	{    "229",                   0 },
	{    "230",                   0 },
	{    "231",                   0 },
	{    "232",                   0 },
	{    "233",                   0 },
	{    "234",                   0 },
	{    "235",                   0 },
	{    "236",                   0 },
	{    "237",                   0 },
	{    "238",                   0 },
	{    "239",                   0 },
	{    "240",                   0 },
	{    "241",                   0 },
	{    "242",                   0 },
	{    "243",                   0 },
	{    "244",                   0 },
	{    "245",                   0 },
	{    "246",                   0 },
	{    "247",                   0 },
	{    "248",                   0 },
	{    "249",                   0 },
	{    "250",                   0 },
	{    "251",                   0 },
	{    "252",                   0 },
	{    "253",                   0 },
	{    "254",                   0 },
	{    "255",                   0 }
};
