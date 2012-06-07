/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/traceset.h>
#include <lttv/iattribute.h>
#include <lttv/state.h>
#include <lttv/hook.h>
#include <stdio.h>
#include <babeltrace/context.h>
#include <babeltrace/iterator.h>
#include <babeltrace/ctf/events.h>

/* To traverse a tree recursively */
#include <fcntl.h>
#include <fts.h>

/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/


LttvTraceset *lttv_traceset_new(void)
{
	LttvTraceset *s;
        struct bt_iter_pos begin_pos;

	s = g_new(LttvTraceset, 1);
	s->filename = NULL;
	s->traces = g_ptr_array_new();
	s->context = bt_context_create();
	s->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
	//TODO remove this when we have really mecanism
	//s->tmpState = g_new(LttvTraceState *, 1);
	//lttv_trace_state_init(s->tmpState,0);
	begin_pos.type = BT_SEEK_BEGIN;

        //s->iter = bt_ctf_iter_create(lttv_traceset_get_context(s),
        //                                &begin_pos,
        //                                NULL);
	s->iter = 0;
        s->event_hooks = lttv_hooks_new();
	s->state_trace_handle_index =  g_ptr_array_new();



	return s;
}

char * lttv_traceset_name(LttvTraceset * s)
{
	return s->filename;
}

#ifdef BABEL_CLEANUP
LttvTrace *lttv_trace_new(LttTrace *t)
{
	LttvTrace *new_trace;

	new_trace = g_new(LttvTrace, 1);
	new_trace->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
	new_trace->id = t;
	new_trace->ref_count = 0;
	return new_trace;
}
#endif

/*
 * lttv_trace_create : Create a trace from a path
 *
 * ts is the traceset in which will be contained the trace
 *
 * path is the path where to find a trace. It is not recursive.
 *
 * This function is static since a trace should always be contained in a
 * traceset.
 *
 * return the created trace or NULL on failure
 */
static LttvTrace *lttv_trace_create(LttvTraceset *ts, const char *path)
{
  int id = bt_context_add_trace(lttv_traceset_get_context(ts),
            path,
            "ctf",
            NULL,
            NULL,
            NULL);
  if (id < 0) {
    return NULL;
  }
  // Create the trace and save the trace handle id returned by babeltrace
  LttvTrace *new_trace;

  new_trace = g_new(LttvTrace, 1);
  new_trace->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  new_trace->id = id;
  new_trace->ref_count = 0;
  new_trace->traceset = ts;
  new_trace->state = g_new(LttvTraceState,1);
  lttv_trace_state_init(new_trace->state,new_trace);

  /* Add the state to the trace_handle to state index */
  g_ptr_array_set_size(ts->state_trace_handle_index,id+1);
  g_ptr_array_index(ts->state_trace_handle_index,id) = new_trace->state;

  return new_trace;
}

/*
 * lttv_trace_create : Create and add a single trace to a traceset
 *
 * ts is the traceset in which will be contained the trace
 *
 * path is the path where to find a trace. It is not recursive.
 *
 * return a positive integer (>=0)on success or -1 on failure
 */
static int lttv_traceset_create_trace(LttvTraceset *ts, const char *path)
{
  LttvTrace *trace = lttv_trace_create(ts, path);
  if (trace == NULL) {
    return -1;
  }
  lttv_traceset_add(ts, trace);
  return 0;
}

LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig) 
{
	guint i;
	LttvTraceset *s;
	LttvTrace * trace;

	s = g_new(LttvTraceset, 1);
	s->filename = NULL;
	s->traces = g_ptr_array_new();
	s->state_trace_handle_index = g_ptr_array_new();
	for(i=0;i<s_orig->traces->len;i++)
	{
		trace = g_ptr_array_index(s_orig->traces, i);
		trace->ref_count++;

		/* WARNING: this is an alias, not a copy. */
		g_ptr_array_add(s->traces, trace);

		g_ptr_array_set_size(s->state_trace_handle_index,trace->id+1);
		g_ptr_array_index(s->state_trace_handle_index,trace->id) = trace->state;
		
	}
	s->context = s_orig->context;
	bt_context_get(s->context);
	s->a = LTTV_ATTRIBUTE(lttv_iattribute_deep_copy(LTTV_IATTRIBUTE(s_orig->a)));
	return s;
}


LttvTraceset *lttv_traceset_load(const gchar *filename)
{
	LttvTraceset *s = g_new(LttvTraceset,1);
	FILE *tf;

	s->filename = g_strdup(filename);
	tf = fopen(filename,"r");

	g_critical("NOT IMPLEMENTED : load traceset data from a XML file");

	fclose(tf);
	return s;
}

gint lttv_traceset_save(LttvTraceset *s)
{
	FILE *tf;

	tf = fopen(s->filename, "w");

	g_critical("NOT IMPLEMENTED : save traceset data in a XML file");

	fclose(tf);
	return 0;
}

void lttv_traceset_destroy(LttvTraceset *s) 
{
	guint i;

	for(i=0;i<s->traces->len;i++) {
		LttvTrace *trace = g_ptr_array_index(s->traces, i);
		lttv_trace_unref(trace);
		// todo mdenis 2012-03-27: uncomment when babeltrace gets fixed
		//bt_context_remove_trace(lttv_traceset_get_context(s), trace->id);
		if(lttv_trace_get_ref_number(trace) == 0)
			lttv_trace_destroy(trace);
	}
	g_ptr_array_free(s->traces, TRUE);
	bt_context_put(s->context);
	g_object_unref(s->a);
	g_free(s);
}

struct bt_context *lttv_traceset_get_context(LttvTraceset *s)
{
	return s->context;
}

LttvTraceset *lttv_trace_get_traceset(LttvTrace *trace)
{
	return trace->traceset;
}

LttvHooks *lttv_traceset_get_hooks(LttvTraceset *s)
{
	return s->event_hooks;
}

void lttv_trace_destroy(LttvTrace *t) 
{
	g_object_unref(t->a);
	g_free(t);
}


void lttv_traceset_add(LttvTraceset *s, LttvTrace *t) 
{
	t->ref_count++;
	g_ptr_array_add(s->traces, t);
}

int lttv_traceset_add_path(LttvTraceset *ts, char *trace_path)
{
	FTS *tree;
	FTSENT *node;
	char * const paths[2] = { trace_path, NULL };
	int ret = -1;

	tree = fts_open(paths, FTS_NOCHDIR | FTS_LOGICAL, 0);
	if (tree == NULL) {
		g_warning("Cannot traverse \"%s\" for reading.\n",
				trace_path);
		return ret;
	}

	int dirfd, metafd;
	while ((node = fts_read(tree))) {

		if (!(node->fts_info & FTS_D))
			continue;

		dirfd = open(node->fts_accpath, 0);
		if (dirfd < 0) {
			g_warning("Unable to open trace "
					"directory file descriptor : %s.", node->fts_accpath);
			ret = dirfd;
			goto error;
		}

		// Check if a metadata file exists in the current directory
		metafd = openat(dirfd, "metadata", O_RDONLY);
		if (metafd < 0) {
			ret = close(dirfd);
			if (ret < 0) {
				g_warning("Unable to open metadata "
						"file descriptor : %s.", node->fts_accpath);
				goto error;
			}
		} else {
			ret = close(metafd);
			if (ret < 0) {
				g_warning("Unable to close metadata "
						"file descriptor : %s.", node->fts_accpath);
				goto error;
			}
			ret = close(dirfd);
			if (ret < 0) {
				g_warning("Unable to close trace "
						"directory file descriptor : %s.", node->fts_accpath);
				goto error;
			}

			ret = lttv_traceset_create_trace(ts, node->fts_accpath);
			if (ret < 0) {
				g_warning("Opening trace \"%s\" from %s "
						"for reading.", node->fts_accpath, trace_path);
				goto error;
			}
		}
	}

error:
	ret = fts_close(tree);
	if (ret < 0) {
		g_warning("Unable to close tree  "
				"file descriptor : %s.", trace_path);
	}
	return ret;
}

unsigned lttv_traceset_number(LttvTraceset *s) 
{
	return s->traces->len;
}


LttvTrace *lttv_traceset_get(LttvTraceset *s, unsigned i) 
{
	g_assert(s->traces->len > i);
	return ((LttvTrace *)s->traces->pdata[i]);
}


void lttv_traceset_remove(LttvTraceset *s, unsigned i) 
{
	LttvTrace * t;
	g_assert(s->traces->len > i);
	t = (LttvTrace *)s->traces->pdata[i];
	t->ref_count--;
	bt_context_remove_trace(lttv_traceset_get_context(s), t->id);
	g_ptr_array_remove_index(s->traces, i);
}


/* A set of attributes is attached to each trace set, trace and tracefile
	 to store user defined data as needed. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s) 
{
	return s->a;
}


LttvAttribute *lttv_trace_attribute(LttvTrace *t)
{
	return t->a;
}

#ifdef BABEL_CLEANUP
LttTrace *lttv_trace(LttvTrace *t)
{
	return t->t;
}
#endif

gint lttv_trace_get_id(LttvTrace *t)
{
  return t->id;
}

guint lttv_trace_get_ref_number(LttvTrace * t)
{
	// todo mdenis: adapt to babeltrace
	return t->ref_count;
}

guint lttv_trace_ref(LttvTrace * t)
{
	t->ref_count++;

	return t->ref_count;
}

guint lttv_trace_unref(LttvTrace * t)
{
	if(likely(t->ref_count > 0))
		t->ref_count--;

	return t->ref_count;
}

guint lttv_trace_get_num_cpu(LttvTrace *t)
{
#warning "TODO - Set the right number of CPU"
	return 24;
}

LttvTracesetPosition *lttv_traceset_create_position(LttvTraceset *traceset)
{
#warning "TODO"
	return NULL;
}

void lttv_traceset_destroy_position(LttvTracesetPosition *traceset_pos)
{
#warning "TODO"
	return NULL;
}

void lttv_traceset_seek_to_position(LttvTracesetPosition *traceset_pos)
{
#warning "TODO"
}

guint lttv_traceset_get_cpuid_from_event(LttvEvent *event)
{
	struct definition *scope;
	unsigned long timestamp;
	unsigned int cpu_id;
	
	struct bt_ctf_event *ctf_event = event->bt_event;
	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		return 0;
	}
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_STREAM_PACKET_CONTEXT);
	if (bt_ctf_field_get_error()) {
		return 0;
	}
	cpu_id = bt_ctf_get_uint64(bt_ctf_get_field(ctf_event, scope, "cpu_id"));
	if (bt_ctf_field_get_error()) {
		return 0;
	} else {
		return cpu_id;
	}
}

const char *lttv_traceset_get_name_from_event(LttvEvent *event)
{
  	return bt_ctf_event_name(event->bt_event);
}
