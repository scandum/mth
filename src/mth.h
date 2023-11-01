#include <zlib.h>

typedef struct mth_data           MTH_DATA;
typedef struct mud_data           MUD_DATA;

/*
	Utility macros.
*/

#define HAS_BIT(var, bit)       ((var)  & (bit))
#define SET_BIT(var, bit)	((var) |= (bit))
#define DEL_BIT(var, bit)       ((var) &= (~(bit)))
#define TOG_BIT(var, bit)       ((var) ^= (bit))

/*
	Update these to use whatever your MUD uses
*/

#define RESTRING(point, value) \
{ \
	STRFREE(point); \
	point = strdup(value); \
}

#define STRALLOC(point) \
{ \
	point = strdup(value); \
}

#define STRFREE(point) \
{ \
	free(point); \
	point = NULL; \
} 

/*
	Typedefs
*/

typedef struct mth_data           MTH_DATA;
typedef struct mud_data           MUD_DATA;
typedef struct descriptor_data    DESCRIPTOR_DATA;

#define BV00            (0   <<  0)
#define BV01            (1   <<  0)
#define BV02            (1   <<  1)
#define BV03            (1   <<  2)
#define BV04            (1   <<  3)
#define BV05            (1   <<  4)
#define BV06            (1   <<  5)
#define BV07            (1   <<  6)
#define BV08            (1   <<  7)
#define BV09            (1   <<  8)
#define BV10            (1   <<  9)

#define COMM_FLAG_DISCONNECT    BV01
#define COMM_FLAG_PASSWORD      BV02
#define COMM_FLAG_REMOTEECHO    BV03
#define COMM_FLAG_EOR           BV04
#define COMM_FLAG_MSDPUPDATE    BV05
#define COMM_FLAG_256COLORS     BV06
#define COMM_FLAG_UTF8          BV07
#define COMM_FLAG_GMCP          BV08

#define MSDP_FLAG_COMMAND       BV01
#define MSDP_FLAG_LIST          BV02
#define MSDP_FLAG_SENDABLE      BV03
#define MSDP_FLAG_REPORTABLE    BV04
#define MSDP_FLAG_CONFIGURABLE  BV05
#define MSDP_FLAG_REPORTED      BV06
#define MSDP_FLAG_UPDATED       BV07

// As per the MTTS standard

#define MTTS_FLAG_ANSI          BV01
#define MTTS_FLAG_VT100         BV02
#define MTTS_FLAG_UTF8          BV03
#define MTTS_FLAG_256COLORS     BV04
#define MTTS_FLAG_MOUSETRACKING BV05
#define MTTS_FLAG_COLORPALETTE  BV06
#define MTTS_FLAG_SCREENREADER  BV07
#define MTTS_FLAG_PROXY         BV08
#define MTTS_FLAG_TRUECOLOR     BV09

/*
	Mud data, structure containing global variables.
*/

struct mud_data
{
	DESCRIPTOR_DATA     * client;
	int                   server;
	int                   boot_time;
	int                   port;
	int                   total_plr;
	int                   top_area;
	int                   top_help;
	int                   top_mob_index;
	int                   top_obj_index;
	int                   top_room;
	int                   msdp_table_size;
	int                   mccp_len;
	unsigned char       * mccp_buf;
};

struct mth_data
{
	struct msdp_data ** msdp_data;
	char              * proxy;
	char              * terminal_type;
	char                telbuf[MAX_INPUT_LENGTH];
	int                 teltop;
	long long           mtts;
	int                 comm_flags;
	short               cols;
	short               rows;
	z_stream          * mccp2;
	z_stream          * mccp3;
};

extern MUD_DATA *mud;

/*
	telopt.c
*/

int         translate_telopts        ( DESCRIPTOR_DATA *d, unsigned char *src, int srclen, unsigned char *out, int outlen );
void        announce_support         ( DESCRIPTOR_DATA *d );
void        unannounce_support       ( DESCRIPTOR_DATA *d );
void        write_mccp2              ( DESCRIPTOR_DATA *d, char *txt, int length );
void        send_echo_on             ( DESCRIPTOR_DATA *d );
void        send_echo_off            ( DESCRIPTOR_DATA *d );
/*
	mth.c
*/
void        init_mth                 (void);
void        init_mth_socket          ( DESCRIPTOR_DATA *d );
void        uninit_mth_socket        ( DESCRIPTOR_DATA *d );
void        arachnos_devel           ( char *fmt, ... );
void        arachnos_mudlist         ( char *fmt, ... );
int         cat_sprintf              ( char *dest, char *fmt, ... );
void        log_descriptor_printf    ( DESCRIPTOR_DATA *d, char *fmt, ... );

/*
	msdp.c
*/

void        init_msdp_table               ( void );
void        process_msdp_varval           ( DESCRIPTOR_DATA *d, char *var, char *val );
void        msdp_send_update              ( DESCRIPTOR_DATA *d );
void        msdp_update_var               ( DESCRIPTOR_DATA *d, char *var, char *fmt, ... );
void        msdp_update_var_instant       ( DESCRIPTOR_DATA *d, char *var, char *fmt, ... );

void        msdp_configure_arachnos       ( DESCRIPTOR_DATA *d, int index );
void        msdp_configure_pluginid       ( DESCRIPTOR_DATA *d, int index );

void        write_msdp_to_descriptor      ( DESCRIPTOR_DATA *d, char *src, int length );
int         msdp2json                     ( unsigned char *src, int srclen, char *out );
int         json2msdp                     ( unsigned char *src, int srclen, char *out );

/*
	tables.c
*/

#define ANNOUNCE_WILL   BV01
#define ANNOUNCE_DO     BV02

extern char *telcmds[];

struct telnet_type
{
	char      *name;
	int       flags;
};

extern struct telnet_type telnet_table[];

typedef void MSDP_FUN (struct descriptor_data *d, int index);

struct msdp_type
{
	char     *name;
	int       flags;
	MSDP_FUN *fun;
};

struct msdp_data
{
	char     *value;
	int      flags;
};

extern struct msdp_type msdp_table[];

/*
	telnet protocol
*/

#define     IAC                 255
#define     DONT                254
#define     DO                  253
#define     WONT                252
#define     WILL                251
#define     SB                  250
#define     GA                  249   /* used for prompt marking */
#define     EL                  248
#define     EC                  247
#define     AYT                 246
#define     AO                  245
#define     IP                  244
#define     BREAK               243
#define     DM                  242
#define     NOP                 241   /* used for keep alive messages */
#define     SE                  240
#define     EOR                 239   /* used for prompt marking */
#define     ABORT               238
#define     SUSP                237
#define     xEOF                236

/*
	telnet options
*/

#define     TELOPT_ECHO           1   /* used to toggle local echo */
#define     TELOPT_SGA            3   /* used to toggle character mode */
#define     TELOPT_TTYPE         24   /* used to send client terminal type */
#define     TELOPT_EOR           25   /* used to toggle eor mode */
#define     TELOPT_NAWS          31   /* used to send client window size */
#define     TELOPT_NEW_ENVIRON   39   /* used to send information */
#define     TELOPT_CHARSET       42   /* used to detect UTF-8 support */
#define     TELOPT_MSDP          69   /* used to send mud server data */
#define     TELOPT_MSSP          70   /* used to send mud server information */
#define     TELOPT_MCCP2         86   /* used to toggle Mud Client Compression Protocol v2 */
#define     TELOPT_MCCP3         87   /* used to toggle Mud Client Compression Protocol v3 */
#define     TELOPT_MSP           90   /* used to toggle Mud Sound Protocol */
#define     TELOPT_MXP           91   /* used to toggle Mud Extention Protocol */
#define     TELOPT_GMCP         201

#define	    ENV_IS                0   /* option is... */
#define	    ENV_SEND              1   /* send option */
#define	    ENV_INFO              2   /* not sure */

#define	    ENV_VAR               0
#define	    ENV_VAL               1
#define	    ENV_ESC               2
#define     ENV_USR               3

#define     CHARSET_REQUEST       1
#define     CHARSET_ACCEPTED      2
#define     CHARSET_REJECTED      3

#define     MSSP_VAR              1
#define     MSSP_VAL              2

#define     MSDP_VAR              1
#define     MSDP_VAL              2
#define     MSDP_TABLE_OPEN       3
#define     MSDP_TABLE_CLOSE      4
#define     MSDP_ARRAY_OPEN       5
#define     MSDP_ARRAY_CLOSE      6

#define     TELCMD_OK(c)    ((unsigned char) (c) >= xEOF)
#define     TELCMD(c)       (telcmds[(unsigned char) (c) - xEOF])
#define     TELOPT(c)       (telnet_table[(unsigned char) (c)].name)
