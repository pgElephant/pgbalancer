/*-------------------------------------------------------------------------
 *
 * pool_shared_types.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_SHARED_TYPES_H
#define POOL_SHARED_TYPES_H

#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#ifndef PGPOOL_ADM
#include "parser/pg_config_manual.h"
#endif

/* returns number of bits in the data type or variable */
#define	BITS_PER_TYPE(type)	(sizeof(type) * BITS_PER_BYTE)

/*
 * startup packet definitions (v2) stolen from PostgreSQL
 */
#define SM_DATABASE		64
#define SM_USER			32
#define SM_OPTIONS		64
#define SM_UNUSED		64
#define SM_TTY			64

/*
 * Maximum hostname length including domain name and "." including NULL
 * terminate.
 */
#define MAX_FDQN_HOSTNAME_LEN	254

#define MAX_NUM_BACKENDS 128
#define MAX_CONNECTION_SLOTS MAX_NUM_BACKENDS
#define MAX_DB_HOST_NAMELEN	 MAX_FDQN_HOSTNAME_LEN
#define MAX_PATH_LENGTH 256

typedef enum
{
	CON_UNUSED,					/* unused slot */
	CON_CONNECT_WAIT,			/* waiting for connection starting */
	CON_UP,						/* up and running */
	CON_DOWN					/* down, disconnected */
} BACKEND_STATUS;

/* backend status name strings */
#define BACKEND_STATUS_CON_UNUSED		"unused"
#define BACKEND_STATUS_CON_CONNECT_WAIT	"waiting"
#define BACKEND_STATUS_CON_UP			"up"
#define BACKEND_STATUS_CON_DOWN			"down"
#define BACKEND_STATUS_QUARANTINE		"quarantine"

/*
 * Backend status record file
 */
typedef struct
{
	BACKEND_STATUS status[MAX_NUM_BACKENDS];
} BackendStatusRecord;

typedef enum
{
	ROLE_MAIN,
	ROLE_REPLICA,
	ROLE_PRIMARY,
	ROLE_STANDBY
} SERVER_ROLE;

/*
 * PostgreSQL backend descriptor. Placed on shared memory area.
 */
typedef struct
{
	char		backend_hostname[MAX_DB_HOST_NAMELEN];	/* backend host name */
	int			backend_port;	/* backend port numbers */
	BACKEND_STATUS backend_status;	/* backend status */
	char		pg_backend_status[NAMEDATALEN]; /* backend status examined by
												 * show pool_nodes and
												 * pcp_node_info */
	time_t		status_changed_time;	/* backend status changed time */
	double		backend_weight; /* normalized backend load balance ratio */
	double		unnormalized_weight;	/* described parameter */
	char		backend_data_directory[MAX_PATH_LENGTH];
	char		backend_application_name[NAMEDATALEN];	/* application_name for
														 * walreceiver */
	unsigned short flag;		/* various flags */
	bool		quarantine;		/* true if node is CON_DOWN because of
								 * quarantine */
	uint64		standby_delay;	/* The replication delay against the primary */
	bool		standby_delay_by_time;	/* true if standby_delay is measured
										 * in microseconds, not bytes */
	SERVER_ROLE role;			/* Role of server. used by pcp_node_info and
								 * failover() to keep track of quarantined
								 * primary node */
	char		pg_role[NAMEDATALEN];	/* backend role examined by show
										 * pool_nodes and pcp_node_info */
	char		replication_state[NAMEDATALEN]; /* "state" from
												 * pg_stat_replication */
	char		replication_sync_state[NAMEDATALEN];	/* "sync_state" from
														 * pg_stat_replication */
} BackendInfo;

typedef struct
{
	sig_atomic_t num_backends;	/* Number of used PostgreSQL backends. This
								 * needs to be a sig_atomic_t type since it is
								 * replaced by a local variable while
								 * reloading pgbalancer.conf. */

	BackendInfo backend_info[MAX_NUM_BACKENDS];
} BackendDesc;

typedef enum
{
	WAIT_FOR_CONNECT,
	COMMAND_EXECUTE,
	IDLE,
	IDLE_IN_TRANS,
	CONNECTING
} ProcessStatus;

/*
 * maximum cancel key length
*/
#define	MAX_CANCELKEY_LENGTH	256

/*
 * Connection pool information. Placed on shared memory area.
 */
typedef struct
{
	int			backend_id;		/* backend id */
	char		database[SM_DATABASE];	/* Database name */
	char		user[SM_USER];	/* User name */
	int			major;			/* protocol major version */
	int			minor;			/* protocol minor version */
	int			pid;			/* backend process id */
	char		key[MAX_CANCELKEY_LENGTH];	/* cancel key */
	int32		keylen;			/* cancel key length */
	int			counter;		/* used counter */
	time_t		create_time;	/* connection creation time */
	time_t		client_connection_time; /* client connection time */
	time_t		client_disconnection_time;	/* client last disconnection time */
	int			client_idle_duration;	/* client idle duration time (s) */
	int			load_balancing_node;	/* load balancing node */
	char		connected;		/* True if frontend connected. Please note
								 * that we use "char" instead of "bool". Since
								 * 3.1, we need to export this structure so
								 * that PostgreSQL C functions can use.
								 * Problem is, PostgreSQL defines bool itself,
								 * and if we use bool, the size of the
								 * structure might be out of control of
								 * pgbalancer. So we use "char" here. */
	volatile char swallow_termination;

	/*
	 * Flag to mark that if the connection will be terminated by the backend.
	 * it should not be treated as a backend node failure. This flag is used
	 * to handle pg_terminate_backend()
	 */
} ConnectionInfo;

/*
 * process information
 * This object put on shared memory.
 */

#define MAXSTMTLEN	1024

typedef struct
{
	pid_t		pid;			/* OS's process id */
	time_t		start_time;		/* fork() time */
	char		connected;		/* if not 0 this process is already used */
	int			wait_for_connect;	/* waiting time for client connection (s) */
	ConnectionInfo *connection_info;	/* head of the connection info for
										 * this process */
	int			client_connection_count;	/* how many times clients used
											 * this process */
	ProcessStatus status;
	char		client_host[NI_MAXHOST];	/* client host. Only valid if
											 * status != WAIT_FOR_CONNECT */
	char		client_port[NI_MAXSERV];	/* client port. Only valid if
											 * status != WAIT_FOR_CONNECT */
	char		statement[MAXSTMTLEN];	/* the last statement sent to backend */
	uint64		node_ids[2];	/* "statement" is sent to the node id (bitmap) */
	bool		need_to_restart;	/* If non 0, exit this child process as
									 * soon as current session ends. Typical
									 * case this flag being set is failback a
									 * node in streaming replication mode. */
	bool		exit_if_idle;
	int			pooled_connections; /* Total number of pooled connections by
									 * this child */
} ProcessInfo;

/*
 * reporting types
 */
/* some length definitions */
#define POOLCONFIG_MAXIDLEN 4
#define POOLCONFIG_MAXNAMELEN 64
#define POOLCONFIG_MAXVALLEN 512
#define POOLCONFIG_MAXDESCLEN 80
#define POOLCONFIG_MAXIDENTLEN 63
#define POOLCONFIG_MAXPORTLEN 6
#define POOLCONFIG_MAXSTATLEN 12
#define POOLCONFIG_MAXWEIGHTLEN 20
#define POOLCONFIG_MAXDATELEN 128
#define POOLCONFIG_MAXCOUNTLEN 16
#define POOLCONFIG_MAXLONGCOUNTLEN 20
#define POOLCONFIG_MAXPROCESSSTATUSLEN 20

/* config report struct*/
typedef struct
{
	char		name[POOLCONFIG_MAXNAMELEN + 1];
	char		value[POOLCONFIG_MAXVALLEN + 1];
	char		desc[POOLCONFIG_MAXDESCLEN + 1];
} POOL_REPORT_CONFIG;

/* nodes report struct */
typedef struct
{
	char		node_id[POOLCONFIG_MAXIDLEN + 1];
	char		hostname[MAX_DB_HOST_NAMELEN + 1];
	char		port[POOLCONFIG_MAXPORTLEN + 1];
	char		status[POOLCONFIG_MAXSTATLEN + 1];
	char		pg_status[POOLCONFIG_MAXSTATLEN + 1];
	char		lb_weight[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		role[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		pg_role[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		select[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		load_balance_node[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		delay[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		rep_state[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		rep_sync_state[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		last_status_change[POOLCONFIG_MAXDATELEN];
} POOL_REPORT_NODES;

/* processes report struct */
typedef struct
{
	char		pool_pid[POOLCONFIG_MAXCOUNTLEN + 1];
	char		process_start_time[POOLCONFIG_MAXDATELEN + 1];
	char		client_connection_count[POOLCONFIG_MAXCOUNTLEN + 1];
	char		database[POOLCONFIG_MAXIDENTLEN + 1];
	char		username[POOLCONFIG_MAXIDENTLEN + 1];
	char		backend_connection_time[POOLCONFIG_MAXDATELEN + 1];
	char		pool_counter[POOLCONFIG_MAXCOUNTLEN + 1];
	char		status[POOLCONFIG_MAXPROCESSSTATUSLEN + 1];
} POOL_REPORT_PROCESSES;

/* pools reporting struct */
typedef struct
{
	char		pool_pid[POOLCONFIG_MAXCOUNTLEN + 1];
	char		process_start_time[POOLCONFIG_MAXDATELEN + 1];
	char		client_connection_count[POOLCONFIG_MAXCOUNTLEN + 1];
	char		pool_id[POOLCONFIG_MAXCOUNTLEN + 1];
	char		backend_id[POOLCONFIG_MAXCOUNTLEN + 1];
	char		database[POOLCONFIG_MAXIDENTLEN + 1];
	char		username[POOLCONFIG_MAXIDENTLEN + 1];
	char		backend_connection_time[POOLCONFIG_MAXDATELEN + 1];
	char		client_connection_time[POOLCONFIG_MAXDATELEN + 1];
	char		client_disconnection_time[POOLCONFIG_MAXDATELEN + 1];
	char		client_idle_duration[POOLCONFIG_MAXDATELEN + 1];
	char		pool_majorversion[POOLCONFIG_MAXCOUNTLEN + 1];
	char		pool_minorversion[POOLCONFIG_MAXCOUNTLEN + 1];
	char		pool_counter[POOLCONFIG_MAXCOUNTLEN + 1];
	char		pool_backendpid[POOLCONFIG_MAXCOUNTLEN + 1];
	char		pool_connected[POOLCONFIG_MAXCOUNTLEN + 1];
	char		status[POOLCONFIG_MAXPROCESSSTATUSLEN + 1];
	char		load_balance_node[POOLCONFIG_MAXPROCESSSTATUSLEN + 1];
	char		client_host[NI_MAXHOST];
	char		client_port[NI_MAXSERV];
	char		statement[MAXSTMTLEN];
} POOL_REPORT_POOLS;

/* version struct */
typedef struct
{
	char		version[POOLCONFIG_MAXVALLEN + 1];
} POOL_REPORT_VERSION;

/* health check statistics report struct */
typedef struct
{
	char		node_id[POOLCONFIG_MAXIDLEN + 1];
	char		hostname[MAX_DB_HOST_NAMELEN + 1];
	char		port[POOLCONFIG_MAXPORTLEN + 1];
	char		status[POOLCONFIG_MAXSTATLEN + 1];
	char		role[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		last_status_change[POOLCONFIG_MAXDATELEN];
	char		total_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		success_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		fail_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		skip_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		retry_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		average_retry_count[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		max_retry_count[POOLCONFIG_MAXCOUNTLEN + 1];
	char		max_health_check_duration[POOLCONFIG_MAXCOUNTLEN + 1];
	char		min_health_check_duration[POOLCONFIG_MAXCOUNTLEN + 1];
	char		average_health_check_duration[POOLCONFIG_MAXLONGCOUNTLEN + 1];
	char		last_health_check[POOLCONFIG_MAXDATELEN];
	char		last_successful_health_check[POOLCONFIG_MAXDATELEN];
	char		last_skip_health_check[POOLCONFIG_MAXDATELEN];
	char		last_failed_health_check[POOLCONFIG_MAXDATELEN];
} POOL_HEALTH_CHECK_STATS;

/* show backend statistics report struct */
typedef struct
{
	char		node_id[POOLCONFIG_MAXIDLEN + 1];
	char		hostname[MAX_DB_HOST_NAMELEN + 1];
	char		port[POOLCONFIG_MAXPORTLEN + 1];
	char		status[POOLCONFIG_MAXSTATLEN + 1];
	char		role[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		select_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		insert_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		update_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		delete_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		ddl_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		other_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		panic_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		fatal_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
	char		error_cnt[POOLCONFIG_MAXWEIGHTLEN + 1];
} POOL_BACKEND_STATS;

extern char *role_to_str(SERVER_ROLE role);

#endif /* POOL_SHARED_TYPES_H */
