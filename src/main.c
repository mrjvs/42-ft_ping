#include "context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

t_ftp_ctx ctx = {
	.verbose = false,
	.domain = NULL,
	.seq = 0,
	.id = 0,
	.sock = 0,
	.packet_buffer = NULL,
	.packet_size = 0,
	.payload_ptr = NULL,
	.payload_size = 0,
};

void	create_socket() {
	ctx.sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ctx.sock == -1) {
		// TODO exit with proper error: socket creation failed
		perror("socket");
		exit(1);
	}
}

void	populate_context() {
	ctx.id = getpid();
	ctx.packet_size = sizeof(struct icmphdr) + ctx.payload_size;
	ctx.packet_buffer = malloc(ctx.packet_size);
	if (ctx.packet_buffer == NULL) {
		// TODO exit with proper error: ran out of memory
		perror("malloc");
		exit(1);
	}
	ctx.payload_ptr = ctx.packet_buffer + sizeof(struct icmphdr);

	ctx.addr.sin_addr = ctx.ip;
	ctx.addr.sin_port = 0; // TODO parse args
	ctx.addr.sin_family = AF_INET;
}

void	free_context() {
	close(ctx.sock);
	free(ctx.packet_buffer);
}
  
// TODO not copied checksum
unsigned short checksum(void *b, int len)
{    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;
  
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void	send_ping() {
	int ttl = 64;
    // set socket options at ip to TTL and value to 64,
    // change to what you want by setting ttl_val
    if (setsockopt(ctx.sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl)) != 0)
    {
        // TODO exit with proper error: ttl setting failed
		perror("setsockopt");
		exit(1);
    }

	// construct packet
	struct icmphdr packet;
	bzero(&packet, sizeof(packet)); // TODO no bzero
	bzero(ctx.payload_ptr, ctx.payload_size); // TODO no bzero
	packet.un.echo.id = ctx.id;
	packet.un.echo.sequence = ctx.seq;
	packet.type = 8;
	packet.code = 0;
	memcpy(ctx.packet_buffer, &packet, sizeof(packet)); // TODO no memcpy
	for (size_t i = 0; i < ctx.payload_size; i++)
		ctx.payload_ptr[i] = i;
	packet.checksum = checksum(ctx.packet_buffer, ctx.packet_size);
	memcpy(ctx.packet_buffer, &packet, sizeof(packet)); // TODO no memcpy

	printf("SEND -> %li\n", ctx.packet_size);
	for (size_t i = 0; i < ctx.packet_size; i++)
		printf("%02X ", (int)ctx.packet_buffer[i]);
	printf("\n");
	// send packet
	if (sendto(ctx.sock, ctx.packet_buffer, ctx.packet_size, 0, (struct sockaddr*)&(ctx.addr), sizeof(struct sockaddr)) <= 0) {
		// TODO proper exit: packet couldn't be sent
		perror("sendto");
		exit(1);
	}
}

void	recv_ping() {
	struct iovec iov[1];
	unsigned char buff[10000];
	iov[0].iov_base = buff;
	iov[0].iov_len = 10000;
	// iov[0].iov_base = ctx.packet_buffer;
	// iov[0].iov_len = ctx.packet_size;
	char control[1000];
	struct msghdr msg;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = 1000;
	msg.msg_name = &(ctx.addr);
	msg.msg_namelen = sizeof(ctx.addr);
	// TODO MSG_ERRQUEUE flag parsing

	// receive a packet
	size_t received = recvmsg(ctx.sock, &msg, 0);
	if (received <= 0) {
		perror("recvmsg");
		// TODO exit with proper error: failed to receive message
		exit(1);
	}

	// parse packet
	struct icmphdr packet;
	if (received < sizeof(packet)) {
		// TODO not full icmp packet
		puts("not full packet received");
		exit(1);
	}
	// TODO not full payload?
	struct ip *ip_pack = (struct ip *)buff;
	int hdr_len = ip_pack->ip_hl * 4; 
	unsigned char *data = buff + hdr_len;
	size_t data_len = received - hdr_len;
	printf("RECV -> %li\n", received-hdr_len);
	for (size_t i = 0; i < data_len; i++)
		printf("%02X ", (int)data[i]);
	printf("\n");
}

void	ensure_root() {
	uid_t uid = getuid();
	if (uid != 0) {
		// TODO proper error: need to run as root
		exit(1);
	}
}

int		main(int argc, char *argv[]) {
	// TODO parse args
	(void)argc;
	(void)argv;
	ctx.verbose = true;
	ctx.payload_size = 32;
	inet_pton(AF_INET, argv[1], &(ctx.ip));

	// setup
	ensure_root();
	populate_context();
	create_socket();

	// the main event
	int i = 5;
	while (i) {
		send_ping();
		recv_ping();
		ctx.seq++;
		i--; // TODO temp
	}

	// cleanup
	free_context();
	return 0;
}
