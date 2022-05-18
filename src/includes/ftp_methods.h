#ifndef FTP_METHODS
# define FTP_METHODS

#include "context.h"

void				send_ping(t_ftp_ctx *ctx);
t_bool				recv_ping(t_ftp_ctx *ctx);
enum e_ftp_errors	validate_response(t_ftp_ctx *ctx, t_bool *is_match);
t_bool				loop_til_response(t_ftp_ctx *ctx);

void				create_socket(t_ftp_ctx *ctx);
void				populate_context(t_ftp_ctx *ctx);
void				free_context(t_ftp_ctx *ctx);
void				ensure_root();

void				handle_interupt(int sig);
void				handle_alarm(int sig);
void				setup_handlers();

#endif
