/*-------------------------------------------------------------------------
 *
 * wd_ipc_conn.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef WD_IPC_CONN_H
#define WD_IPC_CONN_H

#include "watchdog/wd_ipc_defines.h"
#include "watchdog/wd_json_data.h"

typedef enum WdCommandResult
{
	CLUSTER_IN_TRANSACTIONING,
	COMMAND_OK,
	COMMAND_FAILED,
	COMMAND_TIMEOUT
} WdCommandResult;


typedef struct WDIPCCmdResult
{
	char		type;
	int			length;
	char	   *data;
} WDIPCCmdResult;


extern void wd_ipc_conn_initialize(void);
extern void wd_set_ipc_address(char *socket_dir, int port);
extern size_t estimate_ipc_socket_addr_len(void);
extern char *get_watchdog_ipc_address(void);

extern WDIPCCmdResult *issue_command_to_watchdog(char type, int timeout_sec, char *data, int data_len, bool blocking);

extern void FreeCmdResult(WDIPCCmdResult *res);

#endif							/* WD_IPC_CONN_H */
