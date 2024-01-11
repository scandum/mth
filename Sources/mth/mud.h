#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <zlib.h>
#include <stdarg.h>


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

typedef struct descriptor_data    DESCRIPTOR_DATA;

#define MUD_PORT                           4321
#define MAX_SKILL                           269
#define MAX_CLASS                             8
#define MAX_RACE                             16
#define MAX_LEVEL                            99

#define MAX_INPUT_LENGTH                   2000
#define MAX_STRING_LENGTH                 12000 // Must be at least 6 times larger than max input length.
#define COMPRESS_BUF_SIZE                 10000

#define FALSE                                 0
#define TRUE                                  1

/*
	Descriptor (channel) partial structure.
*/

struct descriptor_data
{
	DESCRIPTOR_DATA   * next;
	struct mth_data   * mth;
	void              * character;
	char              * host;
	short               descriptor;
};

/*
	mud.c
*/

void        log_printf(char *fmt, ...);
void        log_descriptor_printf(DESCRIPTOR_DATA *d, char *fmt, ...);
void        descriptor_printf ( DESCRIPTOR_DATA *d, char *fmt, ...);

int         write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length);

char      * capitalize_all(char *str);

/*
	net.c
*/

int create_port(int port);
void mainloop(void);
void poll_port(void);
void process_port_connections(fd_set *read_set, fd_set *write_set, fd_set *exc_set);
void close_port_connection(void);
int process_port_input(void);
int port_new(int s);

/*
	client.c
*/

int         recv_sb_mssp             ( unsigned char *src, int srclen );

