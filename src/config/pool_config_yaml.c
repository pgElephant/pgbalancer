/*
 * pgbalancer: a language independent connection pool server for PostgreSQL
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2025 pgElephant
 *
 * pool_config_yaml.c: YAML configuration file parser using libyaml
 * 
 * This provides YAML support alongside the traditional .conf format
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
extern char config_file_dir[POOLMAXPATHLEN + 1];

/* Parse YAML configuration file */
int
pool_config_read_yaml(const char *config_file)
{
	FILE *fh;
	yaml_parser_t parser;
	yaml_event_t event;
	int done = 0;
	int error = 0;
	int lineno = 0;
	char *current_key = NULL;
	
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
			case YAML_SCALAR_EVENT:
				{
					const char *value = (const char *)event.data.scalar.value;
					
					if (current_key)
					{
						/* We have a key-value pair */
						lineno = event.start_mark.line + 1;
						
						/* Use existing set_one_config_option function */
						if (!set_one_config_option(current_key, value, CFGCXT_INIT, PGC_S_FILE, ERROR))
						{
							ereport(DEBUG1,
									(errmsg("configuration parameter not set"),
									 errdetail("parameter: %s = %s at line %d", current_key, value, lineno)));
						}
						else
						{
							ereport(DEBUG2,
									(errmsg("YAML config parameter set"),
									 errdetail("%s = %s", current_key, value)));
						}
						
						free(current_key);
						current_key = NULL;
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
				/* Ignore other events (mappings, sequences, etc.) */
				break;
		}
		
		yaml_event_delete(&event);
	}
	
	/* Cleanup */
	if (current_key)
		free(current_key);
	
	yaml_parser_delete(&parser);
	fclose(fh);
	
	if (error)
		return -1;
	
	ereport(LOG,
			(errmsg("YAML configuration file parsed successfully"),
			 errdetail("file: %s", config_file)));
	
	return 0;
}
