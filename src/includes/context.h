#ifndef FTPING_CONTEXT
# define FTPING_CONTEXT

#include "ftp_bool.h"
#include <netinet/in.h>
#include <stddef.h>
#include <netinet/ip_icmp.h>

typedef struct s_ftp_ctx {
	t_bool				verbose;
	struct in_addr		ip;
	char				*domain;
	int					sock;
	int					seq;
	unsigned short		id;
	unsigned char		*payload_ptr;
	size_t				payload_size;
	unsigned char		*packet_buffer;
	size_t				packet_size;
	size_t				response_size;
	struct sockaddr_in	addr;
	t_bool				should_exit;
	t_bool				timeout_reached;
} t_ftp_ctx;

#endif
