/***************************************************************************
 * Mud Telopt Handler 1.5 by Igor van den Hoven                  2009-2019 *
 ***************************************************************************/

#include "mud.h"
#include "mth.h"

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>


int create_port(int port)
{
	char hostname[MAX_INPUT_LENGTH];
	struct sockaddr_in sa;
	struct hostent *hp = NULL;
	struct linger ld;
	char *reuse = "1";
	int sock;

	gethostname(hostname, MAX_INPUT_LENGTH);

	hp = gethostbyname(hostname);

	sa.sin_family = hp->h_addrtype;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = 0;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0)
	{
		perror("port_initialize: socket");

		return 0;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reuse, sizeof(reuse));

	ld.l_onoff  = 0;
	ld.l_linger = 100;

	setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld));

	if (fcntl(sock, F_SETFL, O_NDELAY|O_NONBLOCK) == -1)
	{
		perror("port_initialize: fcntl O_NDELAY|O_NONBLOCK");
	}

	if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0)
	{
		log_printf("MTH: port %d is already in use.\n", port);
		
		return 0;
	}

	if (listen(sock, 50) == -1)
	{
		perror("port_initialize: listen:");

		close(sock);
		 
		return 0;
	}

	log_printf("The MTH test server is listening on port %d.", port);

	mud->server = sock;

	return sock;
}

void mainloop(void)
{
	static struct timeval curr_time, wait_time, last_time;
	int usec_loop, usec_wait;
	
	wait_time.tv_sec = 0;

	while (TRUE)
	{
		gettimeofday(&last_time, NULL);

		poll_port();

		gettimeofday(&curr_time, NULL);
		
		if (curr_time.tv_sec == last_time.tv_sec)
		{
			usec_loop = curr_time.tv_usec - last_time.tv_usec;
		}
		else
		{
			usec_loop = 1000000 - last_time.tv_usec + curr_time.tv_usec;
		}

		usec_wait = 1000000 / 10 - usec_loop; // 10 pulses per second

		wait_time.tv_usec = usec_wait;
		
		if (usec_wait > 0)
		{
			select(0, NULL, NULL, NULL, &wait_time);
		}
	}
}

void poll_port(void)
{
	fd_set readfds, writefds, excfds;
	static struct timeval to;
	int rv;

	if (mud->server)
	{
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&excfds);

		FD_SET(mud->server, &readfds);

		// Normally this would loop through all connected descriptors, but MTH has only 1.

		if (mud->client->descriptor)
		{
			// MTH addition

			if (HAS_BIT(mud->client->mth->comm_flags, COMM_FLAG_DISCONNECT))
			{
				close_port_connection();
		
				return;
			}

			FD_SET(mud->client->descriptor, &readfds);
			FD_SET(mud->client->descriptor, &writefds);
			FD_SET(mud->client->descriptor, &excfds);

			// MTH addition

			if (HAS_BIT(mud->client->mth->comm_flags, COMM_FLAG_MSDPUPDATE))
			{
				msdp_send_update(mud->client);
			}
		}

		rv = select(FD_SETSIZE, &readfds, &writefds, &excfds, &to);
		
		if (rv <= 0)
		{
			if (rv == 0 || errno == EINTR)
			{
				return;
			}
			perror("select");
			exit(-1);
		}
		process_port_connections(&readfds, &writefds, &excfds);
	}
}

void process_port_connections(fd_set *read_set, fd_set *write_set, fd_set *exc_set)
{
	if (FD_ISSET(mud->server, read_set))
	{
		port_new(mud->server);
	}

	if (mud->client->descriptor)
	{
		if (FD_ISSET(mud->client->descriptor, exc_set))
		{
			SET_BIT(mud->client->mth->comm_flags, COMM_FLAG_DISCONNECT);
			close_port_connection();
		}
		else if (FD_ISSET(mud->client->descriptor, read_set) && process_port_input() < 0)
		{
			SET_BIT(mud->client->mth->comm_flags, COMM_FLAG_DISCONNECT);
			close_port_connection();
		}
	}
}

void close_port_connection()
{
	log_printf("Closing connection to client D%d", mud->client->descriptor);

	// MTH addition

	uninit_mth_socket(mud->client);

	close(mud->client->descriptor);

	mud->client->descriptor = 0;

	RESTRING(mud->client->host, "");
}

int process_port_input(void)
{
	char input[MAX_INPUT_LENGTH], output[MAX_INPUT_LENGTH], *cmd, *lf;
	int size;

	size = read(mud->client->descriptor, input, MAX_INPUT_LENGTH - 1);

	if (size <= 0)
	{
	 	return -1;
	}
	
	input[size] = 0;

	// Copies input to output, separating commands with a single \n

	size = translate_telopts(mud->client, (unsigned char *) input, size, (unsigned char *) output, 0);

	cmd = output;

	if (size)
	{
		while (cmd)
		{
			lf = strchr(cmd, '\n');

			if (lf)
			{
				*lf++ = 0;

				printf("received command (%s)\n", cmd);
				
				if (*lf == 0)
				{
					break;
				}
			}
			else
			{
				printf("received unterminated command (%s)\n", cmd);
			}
			cmd = lf;
		}
		fflush(stdout);
	}

	return 0;
}

int port_new(int s)
{
	struct sockaddr_in sock;
	socklen_t i;
	int fd;

	i = sizeof(sock);
	
	getsockname(s, (struct sockaddr *) &sock, &i);
	
	if ((fd = accept(s, (struct sockaddr *) &sock, &i)) < 0)
	{
		perror("port_new: accept");
		
		return -1;
	}

	if (fcntl(fd, F_SETFL, O_NDELAY|O_NONBLOCK) == -1)
	{
		perror("port_new: fcntl O_NDELAY|O_NONBLOCK");
	}

	log_printf("New connection: %s D%d.", inet_ntoa(sock.sin_addr), fd);

	if (mud->client->descriptor)
	{
		write(fd, "Sorry, the MTH test server only supports 1 connection at a time.\n", strlen("Sorry, the MTH test server only supports 1 connection at a time.\n") + 1);
		log_printf("The MTH test server only supports 1 connection at a time. Disconnecting %s D%D.", inet_ntoa(sock.sin_addr), fd);

		close(fd);

		return -1;
	}

	RESTRING(mud->client->host, inet_ntoa(sock.sin_addr));
	mud->client->descriptor = fd;

	// MTH addition

	init_mth_socket(mud->client);

	descriptor_printf(mud->client, "Welcome to the MTH test server.\r\n");

	return fd;
}
