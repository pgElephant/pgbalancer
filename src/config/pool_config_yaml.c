/*-------------------------------------------------------------------------
 *
 * pool_config_yaml.c
 *      YAML configuration file parser for pgbalancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <yaml.h>

#include "pool_config.h"
#include "pool_config_variables.h"
#include "utils/elog.h"
#include "utils/pool_path.h"

extern POOL_CONFIG g_pool_config;
extern char config_file_dir[POOLMAXPATHLEN + 1];

#define MAX_YAML_DEPTH  32
#define NAMEBUF         1024

/* Parse state */
typedef struct
{
    char *keys[MAX_YAML_DEPTH];
    int   depth;
    int   in_sequence;
    int   sequence_index;
    char *sequence_key;
    int   sequence_item_depth;
} YAMLParseState;

/* Organizational sections that should be skipped in parameter names */
static const char *organizational_sections[] = {
    "Network",
    "Logging",
    "ConnectionPool",
    "MemoryCache",
    "HealthCheck",
    "LoadBalancing",
    "Authentication",
    "Clustering",
    "Failover",
    "Watchdog",
    "Replication",
    NULL
};

static bool
is_organizational_section(const char *key)
{
    for (int i = 0; organizational_sections[i] != NULL; i++)
    {
        if (strcmp(key, organizational_sections[i]) == 0)
            return true;
    }
    return false;
}

static void
append_part(char *dst, size_t dstsz, const char *src)
{
    size_t used = strlen(dst);
    if (used >= dstsz)
        return;
    (void)snprintf(dst + used, dstsz - used, "%s", src);
}

static void
init_parse_state(YAMLParseState *state)
{
    for (int i = 0; i < MAX_YAML_DEPTH; i++)
        state->keys[i] = NULL;
    state->depth = 0;
    state->in_sequence = 0;
    state->sequence_index = 0;
    state->sequence_key = NULL;
    state->sequence_item_depth = -1;
}

static void
free_parse_state(YAMLParseState *state)
{
    for (int i = 0; i < MAX_YAML_DEPTH; i++)
    {
        if (state->keys[i])
        {
            free(state->keys[i]);
            state->keys[i] = NULL;
        }
    }
    if (state->sequence_key)
    {
        free(state->sequence_key);
        state->sequence_key = NULL;
    }
}

/*
 * build_parameter_name
 * Build parameter name from path, skipping organizational sections
 *
 * Examples:
 *   ["Logging", "pid_file_name"] → "pid_file_name"
 *   ["backends", "hostname"] (index=0) → "backends0_hostname"
 */
static char *
build_parameter_name(const YAMLParseState *state)
{
    char buf[NAMEBUF];
    buf[0] = '\0';
    bool first = true;

    if (state->depth == 0)
        return strdup("");

    for (int i = 0; i < state->depth; i++)
    {
        const char *k = state->keys[i];

        /* Skip organizational sections */
        if (is_organizational_section(k))
            continue;

        if (!first)
            append_part(buf, sizeof(buf), "_");
        first = false;

        if (state->in_sequence &&
            state->sequence_key &&
            strcmp(k, state->sequence_key) == 0)
        {
            char tmp[64];
            (void)snprintf(tmp, sizeof(tmp), "%s%d", k, state->sequence_index);
            append_part(buf, sizeof(buf), tmp);
        }
        else
        {
            append_part(buf, sizeof(buf), k);
        }
    }

    return strdup(buf);
}

/*
 * translate_param_inplace
 * Translate YAML naming to pgbalancer parameter names
 *
 * backendsN_xxx → backend_xxxN
 * watchdog_nodesN_xxx → wd_othernodesN_xxx
 */
static void
translate_param_inplace(char *dst, size_t dstsz, const char *src)
{
    const char *p = src;

    /* backendsN_xxx -> backend_xxxN */
    if (strncmp(p, "backends", 8) == 0 && isdigit((unsigned char)p[8]))
    {
        int idx = 0;
        const char *q = p + 8;
        while (*q && isdigit((unsigned char)*q))
            idx = idx * 10 + (*q++ - '0');

        if (*q == '_' && q[1] != '\0')
        {
            char rest[NAMEBUF];
            snprintf(rest, sizeof(rest), "%s", q + 1);
            snprintf(dst, dstsz, "backend_%s%d", rest, idx);
            return;
        }
    }

    /* watchdog_nodesN_xxx -> wd_othernodesN_xxx */
    if (strncmp(p, "watchdog_nodes", 14) == 0 && isdigit((unsigned char)p[14]))
    {
        int idx = 0;
        const char *q = p + 14;
        while (*q && isdigit((unsigned char)*q))
            idx = idx * 10 + (*q++ - '0');

        if (*q == '_' && q[1] != '\0')
        {
            char rest[NAMEBUF];
            snprintf(rest, sizeof(rest), "%s", q + 1);
            snprintf(dst, dstsz, "wd_othernodes%d_%s", idx, rest);
            return;
        }
    }

    /* heartbeat_destinationsN_xxx -> wd_heartbeat_destinationN_xxx */
    if (strncmp(p, "heartbeat_destinations", 22) == 0 && isdigit((unsigned char)p[22]))
    {
        int idx = 0;
        const char *q = p + 22;
        while (*q && isdigit((unsigned char)*q))
            idx = idx * 10 + (*q++ - '0');

        if (*q == '_' && q[1] != '\0')
        {
            char rest[NAMEBUF];
            snprintf(rest, sizeof(rest), "%s", q + 1);
            snprintf(dst, dstsz, "wd_heartbeat_destination%d_%s", idx, rest);
            return;
        }
    }

    /* Default: copy as-is */
    snprintf(dst, dstsz, "%s", src);
}

/*
 * pool_config_read_yaml
 * Parse YAML configuration file using libyaml's full event-driven API
 */
int
pool_config_read_yaml(const char *config_file)
{
    FILE *fh = NULL;
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;
    int error = 0;
    YAMLParseState state;
    char *pending_key = NULL;

    init_parse_state(&state);

    fh = fopen(config_file, "r");
    if (!fh)
    {
        ereport(ERROR,
                (errmsg("could not open YAML configuration file"),
                 errdetail("file: %s, error: %s", config_file, strerror(errno))));
        return -1;
    }

    /* Get config file directory */
    char buf[POOLMAXPATHLEN + 1];
    strlcpy(buf, config_file, sizeof(buf));
    get_parent_directory(buf);
    strlcpy(config_file_dir, buf, sizeof(config_file_dir));

    if (!yaml_parser_initialize(&parser))
    {
        ereport(ERROR, (errmsg("failed to initialize YAML parser")));
        fclose(fh);
        return -1;
    }

    yaml_parser_set_input_file(&parser, fh);

    while (!done && !error)
    {
        if (!yaml_parser_parse(&parser, &event))
        {
            ereport(ERROR,
                    (errmsg("YAML parsing error"),
                     errdetail("line %zu: %s", parser.problem_mark.line + 1, parser.problem)));
            error = 1;
            break;
        }

        switch (event.type)
        {
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
                break;

            case YAML_MAPPING_START_EVENT:
                if (pending_key)
                {
                    if (state.depth >= MAX_YAML_DEPTH)
                    {
                        ereport(ERROR, (errmsg("YAML nesting too deep")));
                        error = 1;
                        yaml_event_delete(&event);
                        goto out;
                    }
                    state.keys[state.depth++] = pending_key;
                    pending_key = NULL;
                }
                if (state.in_sequence && state.sequence_item_depth < 0)
                    state.sequence_item_depth = state.depth;
                break;

            case YAML_MAPPING_END_EVENT:
                if (state.in_sequence && state.sequence_item_depth == state.depth)
                {
                    state.sequence_index++;
                    state.sequence_item_depth = -1;
                }
                if (state.depth > 0)
                {
                    int i = --state.depth;
                    free(state.keys[i]);
                    state.keys[i] = NULL;
                }
                break;

            case YAML_SEQUENCE_START_EVENT:
                if (pending_key)
                {
                    state.in_sequence = 1;
                    state.sequence_index = 0;
                    state.sequence_key = pending_key;
                    pending_key = NULL;

                    if (state.depth >= MAX_YAML_DEPTH)
                    {
                        ereport(ERROR, (errmsg("YAML nesting too deep")));
                        error = 1;
                        yaml_event_delete(&event);
                        goto out;
                    }
                    state.keys[state.depth++] = strdup(state.sequence_key);
                    state.sequence_item_depth = -1;
                }
                break;

            case YAML_SEQUENCE_END_EVENT:
                state.sequence_item_depth = -1;

                if (state.depth > 0)
                {
                    int i = --state.depth;
                    free(state.keys[i]);
                    state.keys[i] = NULL;
                }

                state.in_sequence = 0;
                state.sequence_index = 0;
                free(state.sequence_key);
                state.sequence_key = NULL;
                break;

            case YAML_SCALAR_EVENT:
            {
                const char *text = (const char *) event.data.scalar.value;

                /* Scalar sequence items (e.g., trusted_servers: [a, b, c]) */
                if (state.in_sequence && state.sequence_item_depth < 0 && pending_key == NULL)
                {
                    char *param = build_parameter_name(&state);
                    if (param && *param)
                    {
                        char keybuf[NAMEBUF];
                        translate_param_inplace(keybuf, sizeof(keybuf), param);

                        if (!set_one_config_option(keybuf, text, CFGCXT_INIT, PGC_S_FILE, ERROR))
                        {
                            ereport(DEBUG1,
                                    (errmsg("configuration parameter not set"),
                                     errdetail("parameter: %s = %s (from YAML: %s)", keybuf, text, param)));
                        }
                    }
                    free(param);

                    state.sequence_index++;
                    break;
                }

                /* Key or value? */
                if (pending_key == NULL)
                {
                    pending_key = strdup(text);
                }
                else
                {
                    /* We have a key-value pair */
                    if (state.depth >= MAX_YAML_DEPTH)
                    {
                        ereport(ERROR, (errmsg("YAML nesting too deep")));
                        error = 1;
                        yaml_event_delete(&event);
                        goto out;
                    }

                    state.keys[state.depth++] = pending_key;
                    pending_key = NULL;

                    char *param = build_parameter_name(&state);
                    if (param && *param)
                    {
                        char keybuf[NAMEBUF];
                        translate_param_inplace(keybuf, sizeof(keybuf), param);

                        if (!set_one_config_option(keybuf, text, CFGCXT_INIT, PGC_S_FILE, ERROR))
                        {
                            ereport(DEBUG1,
                                    (errmsg("configuration parameter not set"),
                                     errdetail("parameter: %s = %s", keybuf, text)));
                        }
                    }
                    free(param);

                    {
                        int i = --state.depth;
                        free(state.keys[i]);
                        state.keys[i] = NULL;
                    }
                }
            }
            break;

            case YAML_STREAM_END_EVENT:
                done = 1;
                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

out:
    if (pending_key)
        free(pending_key);

    free_parse_state(&state);
    yaml_parser_delete(&parser);
    fclose(fh);

    if (error)
        return -1;

    ereport(LOG,
            (errmsg("YAML configuration file parsed successfully"),
             errdetail("file: %s", config_file)));

    /* Run post-processor once after all parameters loaded */
    extern bool config_post_processor(ConfigContext context, int elevel);
    if (!config_post_processor(CFGCXT_INIT, ERROR))
        return -1;

    return 0;
}
