/*-------------------------------------------------------------------------
 *
 * pool_config_yaml.c
 *      PostgreSQL connection pooler and load balancer
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
#include <strings.h>
#include <yaml.h>
#include <errno.h>

#include "pool_config.h"
#include "pool_config_variables.h"
#include "utils/elog.h"
#include "utils/pool_path.h"

/* Extern declarations */
extern POOL_CONFIG g_pool_config;
extern char config_file_dir[POOLMAXPATHLEN + 1];

/* Maximum nesting depth for YAML */
#define MAX_YAML_DEPTH 10

/* State for nested YAML parsing */
typedef struct {
	char *keys[MAX_YAML_DEPTH];
	int depth;
	int in_sequence;
	int sequence_index;
	char *sequence_key;
} YAMLParseState;

/* Initialize parse state */
static void
init_parse_state(YAMLParseState *state)
{
	int i;
	for (i = 0; i < MAX_YAML_DEPTH; i++)
		state->keys[i] = NULL;
	state->depth = 0;
	state->in_sequence = 0;
	state->sequence_index = 0;
	state->sequence_key = NULL;
}

/* Free parse state */
static void
free_parse_state(YAMLParseState *state)
{
	int i;
	for (i = 0; i < MAX_YAML_DEPTH; i++)
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

/* Build full parameter name from nested keys */
static char *
build_parameter_name(YAMLParseState *state)
{
	char buffer[1024];
	int i;
	
	buffer[0] = '\0';
	
	if (state->in_sequence && state->sequence_key)
	{
		/* For sequences, use indexed parameters like backend_hostname0 */
		snprintf(buffer, sizeof(buffer), "%s%d", 
				 state->sequence_key, state->sequence_index);
	}
	else if (state->depth > 0)
	{
		/* For nested mappings, concatenate with underscore */
		for (i = 0; i < state->depth; i++)
		{
			if (i > 0)
				strcat(buffer, "_");
			strcat(buffer, state->keys[i]);
		}
	}
	
	return strdup(buffer);
}

/* Parse YAML configuration file */
int
pool_config_read_yaml(const char *config_file)
{
	FILE *fh;
	yaml_parser_t parser;
	yaml_event_t event;
	int done = 0;
	int error = 0;
	YAMLParseState state;
	char *current_key = NULL;
	
	init_parse_state(&state);
	
	ereport(LOG,
			(errmsg("reading YAML configuration file"),
			 errdetail("file: %s", config_file)));
	
	/* Open configuration file */
	fh = fopen(config_file, "r");
	if (!fh)
	{
		ereport(ERROR,
				(errmsg("could not open configuration file"),
				 errdetail("file: %s", config_file),
				 errhint("Ensure the file exists and is readable: %s", strerror(errno))));
		return -1;
	}
	
	/* Extract config file directory */
	strlcpy(config_file_dir, config_file, sizeof(config_file_dir));
	get_parent_directory(config_file_dir);
	
	/* Initialize YAML parser */
	if (!yaml_parser_initialize(&parser))
	{
		fclose(fh);
		ereport(ERROR,
				(errmsg("failed to initialize YAML parser")));
		return -1;
	}
	
	yaml_parser_set_input_file(&parser, fh);
	
	/* Parse YAML events */
	while (!done)
	{
		if (!yaml_parser_parse(&parser, &event))
		{
			ereport(ERROR,
					(errmsg("YAML parse error"),
					 errdetail("line %d: %s", (int)parser.problem_mark.line + 1, parser.problem),
					 errhint("Check YAML syntax in configuration file")));
			error = 1;
			break;
		}
		
		switch (event.type)
		{
			case YAML_MAPPING_START_EVENT:
				/* Start of a mapping (key-value pairs) */
				if (current_key && state.depth < MAX_YAML_DEPTH)
				{
					/* Nested mapping - push key to stack */
					state.keys[state.depth++] = current_key;
					current_key = NULL;
				}
				break;
				
			case YAML_MAPPING_END_EVENT:
				/* End of a mapping */
				if (state.depth > 0)
				{
					state.depth--;
					if (state.keys[state.depth])
					{
						free(state.keys[state.depth]);
						state.keys[state.depth] = NULL;
					}
				}
				break;
				
			case YAML_SEQUENCE_START_EVENT:
				/* Start of a sequence (array) */
				if (current_key)
				{
					state.in_sequence = 1;
					state.sequence_index = 0;
					state.sequence_key = current_key;
					current_key = NULL;
				}
				break;
				
			case YAML_SEQUENCE_END_EVENT:
				/* End of a sequence */
				state.in_sequence = 0;
				state.sequence_index = 0;
				if (state.sequence_key)
				{
					free(state.sequence_key);
					state.sequence_key = NULL;
				}
				break;
				
			case YAML_SCALAR_EVENT:
				{
					const char *value = (const char *)event.data.scalar.value;
					
					if (current_key)
					{
						/* We have a key-value pair */
						char *param_name = build_parameter_name(&state);
						
						if (param_name && strlen(param_name) > 0)
						{
							/* Use existing set_one_config_option function */
							if (!set_one_config_option(param_name, value, CFGCXT_INIT, PGC_S_FILE, ERROR))
							{
								ereport(DEBUG1,
										(errmsg("configuration parameter not set"),
										 errdetail("parameter: %s = %s", param_name, value)));
							}
							else
							{
								ereport(DEBUG2,
										(errmsg("YAML config parameter set"),
										 errdetail("%s = %s", param_name, value)));
							}
							free(param_name);
						}
						
						free(current_key);
						current_key = NULL;
						
						/* Increment sequence index if in sequence */
						if (state.in_sequence)
							state.sequence_index++;
					}
					else
					{
						/* This is a key */
						current_key = strdup(value);
					}
				}
				break;
				
			case YAML_STREAM_END_EVENT:
				done = 1;
				break;
				
			default:
				/* Ignore other events */
				break;
		}
		
		yaml_event_delete(&event);
	}
	
	/* Cleanup */
	if (current_key)
		free(current_key);
	
	free_parse_state(&state);
	yaml_parser_delete(&parser);
	fclose(fh);
	
	if (error)
		return -1;
	
	ereport(LOG,
			(errmsg("YAML configuration file parsed successfully"),
			 errdetail("file: %s", config_file)));
	
	return 0;
}
