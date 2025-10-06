/*-------------------------------------------------------------------------
 *
 * pool_config_yaml.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_CONFIG_YAML_H
#define POOL_CONFIG_YAML_H

/*
 * Read and parse YAML configuration file
 * Returns 0 on success, -1 on error
 */
extern int pool_config_read_yaml(const char *config_file);

/*
 * Initialize configuration with default values
 */
extern void pool_config_init_defaults(void);

#endif /* POOL_CONFIG_YAML_H */

