/**
 * pgbalancer YAML config reader
 *
 * Flattens nested YAML into "param = value" pairs and applies them
 * via set_one_config_option. Handles mappings, nested mappings,
 * sequences of mappings, and scalars. Produces names like:
 *
 *   connection_pooling_max_connections = 100
 *   backend_hostname0 = localhost
 *   backend_port0 = 5433
 *
 * Copyright (c) 2025, pgbalancer contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the pgbalancer nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 #include "pool.h"
 #include "pool_config.h"
 #include "pool_config_variables.h"
 #include "utils/elog.h"
 #include "utils/pool_path.h"
 
 #include <yaml.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdint.h>
 
 #define PARAM_NAME_MAX 256
 
 typedef struct KeyStack
 {
	 char	  **items;
	 size_t		len;
	 size_t		cap;
 } KeyStack;
 
 typedef struct SeqCtx
 {
	 int			active;
	 char	   *name;
	 int			index;
	 int			item_depth;
 } SeqCtx;
 
 static void ks_init(KeyStack *ks);
 static void ks_free(KeyStack *ks);
 static int	ks_push(KeyStack *ks, const char *key);
 static void ks_pop(KeyStack *ks);
 static void build_param_name(char *out, size_t outsz,
							  const KeyStack *ks,
							  const SeqCtx *seq,
							  const char *current_key);
 static int	set_param(const char *name, const char *value);
 static int	safe_strlcat(char *dst, const char *src, size_t dsz);
 static int	safe_strlcpy(char *dst, const char *src, size_t dsz);
 
 int
 pool_config_read_yaml(const char *config_file)
 {
	 FILE		   *fh;
	 yaml_parser_t	parser;
	 yaml_event_t	event;
	 int				done;
	 int				error;
	 KeyStack		ks;
	 SeqCtx			seq;
	 char		   *pending_key;
	 int				expect_key;
	 int				mapping_depth;
 
	 ereport(LOG,
			 (errmsg("reading YAML configuration file"),
			  errdetail("file: %s", config_file)));
 
	 fh = fopen(config_file, "r");
	 if (!fh)
	 {
		 ereport(ERROR,
				 (errcode(ERRCODE_FILE_READ_ERROR),
				  errmsg("could not open configuration file \"%s\"",
						 config_file)));
		 return -1;
	 }
 
	 if (!yaml_parser_initialize(&parser))
	 {
		 ereport(ERROR,
				 (errcode(ERRCODE_INTERNAL_ERROR),
				  errmsg("could not initialize YAML parser")));
		 fclose(fh);
		 return -1;
	 }
 
	 yaml_parser_set_input_file(&parser, fh);
 
	 ks_init(&ks);
	 memset(&seq, 0, sizeof(SeqCtx));
	 pending_key = NULL;
	 expect_key = 1;
	 mapping_depth = 0;
	 done = 0;
	 error = 0;
 
	 while (!done)
	 {
		 if (!yaml_parser_parse(&parser, &event))
		 {
			 ereport(ERROR,
					 (errcode(ERRCODE_CONFIG_FILE_ERROR),
					  errmsg("YAML parser error: %s", parser.problem),
					  errdetail("line %d, column %d",
							   (int) parser.problem_mark.line + 1,
							   (int) parser.problem_mark.column + 1)));
			 error = 1;
			 break;
		 }
 
		 switch (event.type)
		 {
			 case YAML_STREAM_START_EVENT:
				 break;
 
			 case YAML_STREAM_END_EVENT:
				 done = 1;
				 break;
 
			 case YAML_DOCUMENT_START_EVENT:
				 break;
 
			 case YAML_DOCUMENT_END_EVENT:
				 break;
 
			 case YAML_MAPPING_START_EVENT:
				 mapping_depth++;
 
				 if (pending_key != NULL)
				 {
					 if (ks_push(&ks, pending_key) != 0)
					 {
						 error = 1;
						 yaml_event_delete(&event);
						 goto out;
					 }
					 free(pending_key);
					 pending_key = NULL;
					 expect_key = 1;
				 }
 
				 if (seq.active && seq.item_depth == 0)
					 seq.item_depth = mapping_depth;
 
				 break;
 
			 case YAML_MAPPING_END_EVENT:
				 if (ks.len > 0 && mapping_depth > 0)
					 ks_pop(&ks);
 
				 mapping_depth--;
 
				 if (seq.active && seq.item_depth != 0 &&
					 mapping_depth < seq.item_depth)
				 {
					 seq.index++;
					 seq.item_depth = 0;
				 }
				 break;
 
			 case YAML_SEQUENCE_START_EVENT:
				 if (pending_key == NULL)
				 {
					 ereport(DEBUG1,
							 (errmsg("sequence start without key")));
				 }
				 else
				 {
					 seq.active = 1;
					 seq.name = pending_key;
					 seq.index = 0;
					 seq.item_depth = 0;
					 pending_key = NULL;
					 expect_key = 1;
				 }
				 break;
 
			 case YAML_SEQUENCE_END_EVENT:
				 seq.active = 0;
				 if (seq.name)
					 free(seq.name);
				 memset(&seq, 0, sizeof(SeqCtx));
				 expect_key = 1;
				 break;
 
			 case YAML_SCALAR_EVENT:
			 {
				 const char *text;
 
				 text = (const char *) event.data.scalar.value;
 
				 if (expect_key)
				 {
					 if (pending_key != NULL)
						 free(pending_key);
 
					 pending_key = strdup(text);
					 if (pending_key == NULL)
					 {
						 error = 1;
						 yaml_event_delete(&event);
						 goto out;
					 }
					 expect_key = 0;
				 }
				 else
				 {
					 char param[PARAM_NAME_MAX];
 
					 memset(param, 0, sizeof(param));
					 build_param_name(param, sizeof(param), &ks, &seq,
									  pending_key);
 
					 if (param[0] != '\0')
					 {
						 if (!set_param(param, text))
						 {
							 ereport(DEBUG1,
									 (errmsg("parameter ignored"),
									  errdetail("parameter: %s = %s",
												param, text)));
						 }
						 else
						 {
							 ereport(LOG,
									 (errmsg("parameter set"),
									  errdetail("parameter: %s = %s",
												param, text)));
						 }
					 }
 
					 free(pending_key);
					 pending_key = NULL;
					 expect_key = 1;
 
					 if (seq.active && seq.item_depth == 0)
						 seq.index++;
				 }
		 }
		 break;

		 case YAML_ALIAS_EVENT:
		 case YAML_NO_EVENT:
			 break;
		 }
 
		 yaml_event_delete(&event);
	 }
 
 out:
	 yaml_parser_delete(&parser);
	 fclose(fh);
 
	 if (pending_key)
		 free(pending_key);
	 ks_free(&ks);
 
	 if (seq.name)
		 free(seq.name);
 
	 if (error)
		 return -1;
 
	 ereport(LOG,
			 (errmsg("YAML configuration file parsed successfully"),
			  errdetail("file: %s", config_file)));
 
	 return 0;
 }
 
 static void
 ks_init(KeyStack *ks)
 {
	 ks->items = NULL;
	 ks->len = 0;
	 ks->cap = 0;
 }
 
 static void
 ks_free(KeyStack *ks)
 {
	 size_t i;
 
	 for (i = 0; i < ks->len; i++)
		 free(ks->items[i]);
 
	 free(ks->items);
	 ks->items = NULL;
	 ks->len = 0;
	 ks->cap = 0;
 }
 
 static int
 ks_push(KeyStack *ks, const char *key)
 {
	 char   *copy;
	 size_t	newcap;
	 char  **newitems;
 
	 copy = strdup(key);
	 if (copy == NULL)
		 return -1;
 
	 if (ks->len == ks->cap)
	 {
		 newcap = ks->cap == 0 ? 8 : ks->cap * 2;
		 newitems = (char **) realloc(ks->items, newcap * sizeof(char *));
		 if (newitems == NULL)
		 {
			 free(copy);
			 return -1;
		 }
		 ks->items = newitems;
		 ks->cap = newcap;
	 }
 
	 ks->items[ks->len++] = copy;
	 return 0;
 }
 
 static void
 ks_pop(KeyStack *ks)
 {
	 if (ks->len == 0)
		 return;
	 free(ks->items[ks->len - 1]);
	 ks->items[ks->len - 1] = NULL;
	 ks->len--;
 }
 
 static void
 build_param_name(char *out, size_t outsz,
				  const KeyStack *ks,
				  const SeqCtx *seq,
				  const char *current_key)
 {
	 size_t i;
 
	 out[0] = '\0';
 
	 if (seq->active)
	 {
		 if (seq->item_depth != 0 && current_key != NULL)
		 {
			 if (strcmp(seq->name, "backends") == 0)
			 {
				 if (safe_strlcpy(out, "backend_", outsz) != 0)
					 return;
				 if (safe_strlcat(out, current_key, outsz) != 0)
					 return;
				 {
					 char idxbuf[32];
 
					 snprintf(idxbuf, sizeof(idxbuf), "%d", seq->index);
					 if (safe_strlcat(out, idxbuf, outsz) != 0)
						 return;
				 }
				 return;
			 }
			 else
			 {
				 if (safe_strlcpy(out, seq->name, outsz) != 0)
					 return;
				 {
					 char idxbuf[32];
 
					 snprintf(idxbuf, sizeof(idxbuf), "%d", seq->index);
					 if (safe_strlcat(out, idxbuf, outsz) != 0)
						 return;
				 }
				 if (safe_strlcat(out, "_", outsz) != 0)
					 return;
				 if (safe_strlcat(out, current_key, outsz) != 0)
					 return;
				 return;
			 }
		 }
		 else if (seq->item_depth == 0 && current_key == NULL)
		 {
			 if (safe_strlcpy(out, seq->name, outsz) != 0)
				 return;
			 {
				 char idxbuf[32];
 
				 snprintf(idxbuf, sizeof(idxbuf), "%d", seq->index);
				 (void) safe_strlcat(out, idxbuf, outsz);
			 }
			 return;
		 }
	 }
 
	 for (i = 0; i < ks->len; i++)
	 {
		 if (i > 0)
		 {
			 if (safe_strlcat(out, "_", outsz) != 0)
				 return;
		 }
		 if (safe_strlcat(out, ks->items[i], outsz) != 0)
			 return;
	 }
 
	 if (current_key != NULL)
	 {
		 if (ks->len > 0)
		 {
			 if (safe_strlcat(out, "_", outsz) != 0)
				 return;
		 }
		 (void) safe_strlcat(out, current_key, outsz);
	 }
 }
 
 static int
 set_param(const char *name, const char *value)
 {
	 if (!set_one_config_option(name, value,
								CFGCXT_INIT, PGC_S_FILE, ERROR))
		 return 0;
	 return 1;
 }
 
 static int
 safe_strlcpy(char *dst, const char *src, size_t dsz)
 {
	 size_t slen;
 
	 if (dsz == 0)
		 return -1;
 
	 slen = strlen(src);
	 if (slen + 1 > dsz)
	 {
		 memcpy(dst, src, dsz - 1);
		 dst[dsz - 1] = '\0';
		 return -1;
	 }
	 memcpy(dst, src, slen + 1);
	 return 0;
 }
 
 static int
 safe_strlcat(char *dst, const char *src, size_t dsz)
 {
	 size_t dlen;
	 size_t slen;
 
	 dlen = strlen(dst);
	 slen = strlen(src);
 
	 if (dlen + slen + 1 > dsz)
	 {
		 size_t copy = dsz - dlen - 1;
 
		 if (copy > 0)
		 {
			 memcpy(dst + dlen, src, copy);
			 dst[dlen + copy] = '\0';
		 }
		 return -1;
	 }
 
	 memcpy(dst + dlen, src, slen + 1);
	 return 0;
 }
 