/*-------------------------------------------------------------------------
 *
 * pool_health_check_stats.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include <stddef.h>
#include "pool.h"
#include "pool_shared_types.h"

/*
 * Returns an array consisting of POOL_HEALTH_CHECK_STATS struct member
 * offsets.  The reason why we have this as a function is the table data needs
 * to be shared by both PCP server and clients.
 * Number of struct members will be stored in *n.
 */
int *
pool_health_check_stats_offsets(int *n)
{
	static int	offsettbl[] = {
		offsetof(POOL_HEALTH_CHECK_STATS, node_id),
		offsetof(POOL_HEALTH_CHECK_STATS, hostname),
		offsetof(POOL_HEALTH_CHECK_STATS, port),
		offsetof(POOL_HEALTH_CHECK_STATS, status),
		offsetof(POOL_HEALTH_CHECK_STATS, role),
		offsetof(POOL_HEALTH_CHECK_STATS, last_status_change),
		offsetof(POOL_HEALTH_CHECK_STATS, total_count),
		offsetof(POOL_HEALTH_CHECK_STATS, success_count),
		offsetof(POOL_HEALTH_CHECK_STATS, fail_count),
		offsetof(POOL_HEALTH_CHECK_STATS, skip_count),
		offsetof(POOL_HEALTH_CHECK_STATS, retry_count),
		offsetof(POOL_HEALTH_CHECK_STATS, average_retry_count),
		offsetof(POOL_HEALTH_CHECK_STATS, max_retry_count),
		offsetof(POOL_HEALTH_CHECK_STATS, max_health_check_duration),
		offsetof(POOL_HEALTH_CHECK_STATS, min_health_check_duration),
		offsetof(POOL_HEALTH_CHECK_STATS, average_health_check_duration),
		offsetof(POOL_HEALTH_CHECK_STATS, last_health_check),
		offsetof(POOL_HEALTH_CHECK_STATS, last_successful_health_check),
		offsetof(POOL_HEALTH_CHECK_STATS, last_skip_health_check),
		offsetof(POOL_HEALTH_CHECK_STATS, last_failed_health_check),
	};

	*n = sizeof(offsettbl) / sizeof(int);
	return offsettbl;
}

/*
 * Returns an array consisting of POOL_REPORT_POOLS struct member offsets.
 * The reason why we have this as a function is the table data needs to be
 * shared by both PCP server and clients.  Number of struct members will be
 * stored in *n.
 */
int *
pool_report_pools_offsets(int *n)
{

	static int	offsettbl[] = {
		offsetof(POOL_REPORT_POOLS, pool_pid),
		offsetof(POOL_REPORT_POOLS, process_start_time),
		offsetof(POOL_REPORT_POOLS, client_connection_count),
		offsetof(POOL_REPORT_POOLS, pool_id),
		offsetof(POOL_REPORT_POOLS, backend_id),
		offsetof(POOL_REPORT_POOLS, database),
		offsetof(POOL_REPORT_POOLS, username),
		offsetof(POOL_REPORT_POOLS, backend_connection_time),
		offsetof(POOL_REPORT_POOLS, client_connection_time),
		offsetof(POOL_REPORT_POOLS, client_disconnection_time),
		offsetof(POOL_REPORT_POOLS, client_idle_duration),
		offsetof(POOL_REPORT_POOLS, pool_majorversion),
		offsetof(POOL_REPORT_POOLS, pool_minorversion),
		offsetof(POOL_REPORT_POOLS, pool_counter),
		offsetof(POOL_REPORT_POOLS, pool_backendpid),
		offsetof(POOL_REPORT_POOLS, pool_connected),
		offsetof(POOL_REPORT_POOLS, status),
		offsetof(POOL_REPORT_POOLS, load_balance_node),
		offsetof(POOL_REPORT_POOLS, client_host),
		offsetof(POOL_REPORT_POOLS, client_port),
		offsetof(POOL_REPORT_POOLS, statement)
	};

	*n = sizeof(offsettbl) / sizeof(int);
	return offsettbl;
}
