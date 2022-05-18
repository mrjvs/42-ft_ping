#include "context.h"
#include "ftp_errors.h"
#include "ftp_methods.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

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
	.response_size = 0,
	.should_exit = false,
	.timeout_reached = false,
	.stats = {
		.sent_count = 0,
		.success_count = 0,
		.u_sec_max_rtt = 0,
		.u_sec_min_rtt = -1,
		.u_sec_rtt_sum = 0,
	}
};

int		main(int argc, char *argv[]) {
	// TODO parse args
	(void)argc;
	(void)argv;
	ctx.verbose = true;
	ctx.payload_size = 32;
	inet_pton(AF_INET, argv[1], &(ctx.ip));

	// setup
	ensure_root(&ctx);
	populate_context(&ctx);
	create_socket(&ctx);
	setup_handlers();

	// the main event
	printf("PING %s (%s) %i(%i) bytes of data.\n", "yoink", "yoink", -1, -1);
	ctx.seq--; // so it starts at zero
	while (!ctx.should_exit) {
		ctx.seq++;
		send_ping(&ctx);
		loop_til_response(&ctx);
	}

	// print stats and exit
	printf("\n--- %s ping statistics ---\n", "yoink");
	printf("%i packets transmitted, %i received, %i%% packet loss, time %ims\n", ctx.stats.sent_count, ctx.stats.success_count, -1, -1);
	if (ctx.stats.success_count)
		printf("rtt min/avg/max/mdev = %lli/%lli/%lli/%i ms TODO temp\n", ctx.stats.u_sec_min_rtt, ctx.stats.u_sec_rtt_sum/ctx.stats.success_count, ctx.stats.u_sec_max_rtt, -1);
	free_context(&ctx);
	return 0;
}

void	handle_interupt(int sig) {
	(void)sig;
	ctx.should_exit = true;
	ctx.timeout_reached = true;
}

void	handle_alarm(int sig) {
	(void)sig;
	ctx.timeout_reached = true;
}
