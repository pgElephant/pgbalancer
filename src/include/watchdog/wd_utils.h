/*-------------------------------------------------------------------------
 *
 * wd_utils.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef WD_UTILS_H
#define WD_UTILS_H

#include <ifaddrs.h>
#include <pthread.h>
#include "parser/pg_list.h"

#define WD_NG (0)
#define WD_OK (1)
#define WD_MAX_PACKET_STRING (256)

/* wd_utils.c*/
extern int	wd_chk_sticky(void);
extern void wd_check_network_command_configurations(void);
extern int	watchdog_thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
extern char *string_replace(const char *string, const char *pattern, const char *replacement);
extern void wd_calc_hash(const char *str, int len, char *buf);
extern int	aes_decrypt_with_password(unsigned char *ciphertext, int ciphertext_len,
									  const char *password, unsigned char *plaintext);
extern int	aes_encrypt_with_password(unsigned char *plaintext, int plaintext_len,
									  const char *password, unsigned char *ciphertext);

/* wd_escalation.c */
extern pid_t fork_escalation_process(void);
extern pid_t fork_plunging_process(void);

/* wd_ping.c */
extern bool wd_is_ip_exists(char *ip);
extern pid_t wd_trusted_server_command(char *hostname);

/* wd_if.c */
extern List *get_all_local_ips(void);
extern int	wd_IP_up(void);
extern int	wd_IP_down(void);
extern char *wd_get_cmd(char *cmd);
extern int	create_monitoring_socket(void);
extern bool read_interface_change_event(int sock, bool *link_event, bool *deleted);
extern bool is_interface_up(struct ifaddrs *ifa);

#endif
