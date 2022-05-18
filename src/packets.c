#include "context.h"
#include "ftp_bool.h"
#include "ftp_errors.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/time.h>
  
// TODO not copied checksum
static unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
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

void	send_ping(t_ftp_ctx *ctx) {
    // setting TTL for sending packet
	int ttl = 64;
    if (setsockopt(ctx->sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl)) != 0)
    {
        // TODO exit with proper error: ttl setting failed
		perror("setsockopt");
		exit(1);
    }

	// construct base packet
	struct icmphdr packet;
	bzero(&packet, sizeof(packet)); // TODO no bzero
	bzero(ctx->payload_ptr, ctx->payload_size); // TODO no bzero
	packet.un.echo.id = ctx->id;
	packet.un.echo.sequence = ctx->seq;
	packet.type = 8;
	packet.code = 0;
	memcpy(ctx->packet_buffer, &packet, sizeof(packet)); // TODO no memcpy
	for (size_t i = 0; i < ctx->payload_size; i++)
		ctx->payload_ptr[i] = i;


	// add checksum
	packet.checksum = checksum(ctx->packet_buffer, ctx->payload_size+sizeof(packet));
	memcpy(ctx->packet_buffer, &packet, sizeof(packet)); // TODO no memcpy

	// set send time
	gettimeofday(&(ctx->send_time), NULL); // TODO handle errors
	ctx->stats.sent_count++;

	// send packet
	if (sendto(ctx->sock, ctx->packet_buffer, ctx->payload_size+sizeof(packet), 0, (struct sockaddr*)&(ctx->addr), sizeof(struct sockaddr)) <= 0) {
		// TODO proper exit: packet couldn't be sent
		perror("sendto");
		exit(1);
	}
}

t_bool	recv_ping(t_ftp_ctx *ctx) {
	// build packet
	struct iovec iov[1];
	iov[0].iov_base = ctx->packet_buffer;
	iov[0].iov_len = ctx->packet_size;
	char control[1000];
	struct msghdr msg;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = 1000;
	msg.msg_name = &(ctx->addr);
	msg.msg_namelen = sizeof(ctx->addr);
	// TODO MSG_ERRQUEUE flag parsing

	// wait for a packet
	ssize_t received = recvmsg(ctx->sock, &msg, 0);
	if (received <= 0) {
		// TODO exit with proper error: failed to receive message
		return false;
	}

	ctx->response_size = received;
	return true;
}

enum e_ftp_errors	validate_response(t_ftp_ctx *ctx, t_bool *is_match) {
	*is_match = false;
	// parse ip packet
	if (ctx->response_size < IP_HDR_MIN)
		return FTP_IVLD_IP;
	struct ip *ip_pack = (struct ip *)ctx->packet_buffer;
	int hdr_len = ip_pack->ip_hl * 4;

	// parse icmp packet
	if (ctx->response_size - hdr_len < sizeof(struct icmphdr))
		return FTP_IVLD_ICMP;
	struct icmphdr *packet = (struct icmphdr *)(ctx->packet_buffer + hdr_len);

	// validate icmp packet details
	if (packet->type != 0)
		return FTP_IVLD_ICMP_TYPE;
	if (packet->un.echo.id != ctx->id)
		return FTP_ID_MISMATCH;
	if (packet->un.echo.sequence != ctx->seq)
		return FTP_SEQ_MISMATCH;
	if (packet->code != 0)
		return FTP_IVLD_ICMP_CODE;
	*is_match = true;

	// validate payload
	if (ctx->response_size - hdr_len < sizeof(struct icmphdr) + ctx->payload_size)
		return FTP_PAYLOAD_MISMATCH;
	ctx->payload_ptr = ((unsigned char *)packet) + sizeof(struct icmphdr);
	// TODO validate payload by doing memcmp

	// validate checksum
	unsigned short incoming_checksum = packet->checksum;
	packet->checksum = 0;
	unsigned short actual_checksum = checksum(packet, ctx->payload_size+sizeof(struct icmphdr));
	if (incoming_checksum != actual_checksum)
		return FTP_CHECKSUM_MISMATCH;
	
	return FTP_VALID;
}

t_bool	loop_til_response(t_ftp_ctx *ctx) {
	t_bool	matches_packet = false;
	enum e_ftp_errors err;
	t_bool	found_packet = false;
	t_bool	valid_found_packet;

	// timeout
	ctx->timeout_reached = false;
	alarm(1); // TODO temp number

	// loop till timeout
	while (true) {
		t_bool rcv_success = recv_ping(ctx);
		if (!rcv_success) {
			if (ctx->timeout_reached)
				break;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			return false;
		}

		err = validate_response(ctx, &matches_packet);
		if (matches_packet && !found_packet) {
			// handle valid packet
			found_packet = true;
			valid_found_packet = err == FTP_VALID;
			if (!valid_found_packet) {
				printf("Incoming packet invalid: %i\n", err);
			} else {
				struct timeval time = {0,0};
				gettimeofday(&time, NULL); // TODO handle errors
				long long usec = 0;
				usec += (time.tv_sec - ctx->send_time.tv_sec) * 1000;
				long long tmp = time.tv_usec;
				if (tmp < ctx->send_time.tv_usec)
					tmp += 1000000; // if usec has rolled over, add 1 second so math stays correct
				usec += tmp - ctx->send_time.tv_usec;

				if (usec > ctx->stats.u_sec_max_rtt)
					ctx->stats.u_sec_max_rtt = usec;
				if (usec < ctx->stats.u_sec_min_rtt || ctx->stats.u_sec_min_rtt == -1)
					ctx->stats.u_sec_min_rtt = usec;
				ctx->stats.u_sec_rtt_sum += usec;
				ctx->stats.success_count++;
				printf("%i bytes from %s: icmp_seq=%i ttl=%i\n", -1, "yoink", ctx->seq, -1);
			}
		}
	}

	// timeout handling
	if (!found_packet) {
		printf("timed out\n");
		return false;
	}
	return valid_found_packet;
}
