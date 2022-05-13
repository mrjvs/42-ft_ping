#include "context.h"
#include "ftp_errors.h"
#include "ftp_methods.h"
#include <arpa/inet.h>
#include <stdio.h>

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

	// the main event
	int i = 5;
	ctx.seq--; // so it starts at zero
	while (i) {
		ctx.seq++;
		i--; // TODO temp
		send_ping(&ctx);
		t_bool rcv_success = recv_ping(&ctx);
		if (!rcv_success) {
			continue;
		}
		enum e_ftp_errors err = validate_response(&ctx);
		if (err != FTP_VALID) {
			printf("Incoming packet invalid: %i\n", err);
			continue;
		}
	}

	// cleanup
	free_context(&ctx);
	return 0;
}
