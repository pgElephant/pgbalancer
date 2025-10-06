/*-------------------------------------------------------------------------
 *
 * pool_process_query.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_process_query_h
#define pool_process_query_h

#define LOCK_COMMENT "/*INSERT LOCK*/"
#define LOCK_COMMENT_SZ (sizeof(LOCK_COMMENT)-1)
#define NO_LOCK_COMMENT "/*NO INSERT LOCK*/"
#define NO_LOCK_COMMENT_SZ (sizeof(NO_LOCK_COMMENT)-1)


extern void reset_variables(void);
extern void reset_connection(void);
extern void per_node_statement_log(POOL_CONNECTION_POOL *backend,
								   int node_id, char *query);
extern int	pool_extract_error_message(bool read_kind, POOL_CONNECTION *backend,
									   int major, bool unread, char **message);
extern POOL_STATUS do_command(POOL_CONNECTION *frontend, POOL_CONNECTION *backend,
							  char *query, int protoMajor, int pid, char *key, int keylen, int no_ready_for_query);
extern void do_query(POOL_CONNECTION *backend, char *query, POOL_SELECT_RESULT **result, int major);
extern void free_select_result(POOL_SELECT_RESULT *result);
extern int	compare(const void *p1, const void *p2);
extern void do_error_execute_command(POOL_CONNECTION_POOL *backend, int node_id, int major);
extern POOL_STATUS pool_discard_packet_contents(POOL_CONNECTION_POOL *cp);
extern void pool_dump_valid_backend(int backend_id);
extern bool pool_push_pending_data(POOL_CONNECTION *backend);


extern void pool_send_frontend_exits(POOL_CONNECTION_POOL *backend);
extern POOL_STATUS ParameterStatus(POOL_CONNECTION *frontend,
								   POOL_CONNECTION_POOL *backend);

extern void pool_send_error_message(POOL_CONNECTION *frontend, int protoMajor,
									char *code,
									char *message,
									char *detail,
									char *hint,
									char *file,
									int line);
extern void pool_send_fatal_message(POOL_CONNECTION *frontend, int protoMajor,
									char *code,
									char *message,
									char *detail,
									char *hint,
									char *file,
									int line);
extern void pool_send_severity_message(POOL_CONNECTION *frontend, int protoMajor,
									   char *code,
									   char *message,
									   char *detail,
									   char *hint,
									   char *file,
									   char *severity,
									   int line);

extern POOL_STATUS SimpleForwardToFrontend(char kind, POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern POOL_STATUS SimpleForwardToBackend(char kind, POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend, int len, char *contents);

extern POOL_STATUS pool_process_query(POOL_CONNECTION *frontend,
									  POOL_CONNECTION_POOL *backend,
									  int reset_request);
extern bool is_backend_cache_empty(POOL_CONNECTION_POOL *backend);
extern void pool_send_readyforquery(POOL_CONNECTION *frontend);

extern char *extract_error_kind(char *message, int major);

#endif							/* pool_process_query_h */
