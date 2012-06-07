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

#ifndef TRACESET_H
#define TRACESET_H

#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/event.h>
#include <ltt/ltt.h>
#include <lttv/trace.h>
/* A traceset is a set of traces to be analyzed together. */

typedef struct _LttvTraceset LttvTraceset;

typedef struct _LttvTracesetPosition LttvTracesetPosition;

struct bt_context;


//TODO ybrosseau 2012-05-15 put these struct in the .c to make them opaque
struct _LttvTraceset {
	char * filename;
	GPtrArray *traces;		/* Array of pointers to LttvTrace */
	struct bt_context *context;
	LttvAttribute *a;
	LttvHooks *event_hooks;
	struct bt_ctf_iter *iter;
	GPtrArray *state_trace_handle_index;
};

struct _LttvTrace {
	// Trace id for babeltrace
	LttvTraceset *traceset;		/* container traceset */
	gint id;
	LttvAttribute *a;
	guint ref_count;
	LttvTraceState *state;
};

/* In babeltrace, the position concept is an iterator. */
struct _LttvTracesetPosition {
	struct bt_ctf_iter *iter;
};

/* Tracesets may be added to, removed from and their content listed. */

LttvTraceset *lttv_traceset_new(void);

char * lttv_traceset_name(LttvTraceset * s);

#ifdef BABEL_CLEANUP
LttvTrace *lttv_trace_new(LttTrace *t);
#endif

LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig);

LttvTraceset *lttv_traceset_load(const gchar *filename);

struct bt_context *lttv_traceset_get_context(LttvTraceset *s);



gint lttv_traceset_save(LttvTraceset *s);

void lttv_traceset_destroy(LttvTraceset *s);


void lttv_traceset_add(LttvTraceset *s, LttvTrace *t);

/*
 * lttv_trace_create : Add all traces recursively to a traceset from a path
 *
 * ts is the traceset in which will be contained the traces
 *
 * trace_path is the path where to find a set of trace.
 * Traverse the path recursively to add all traces within.
 *
 * return 0 on success or a negative integer on failure
 */
int lttv_traceset_add_path(LttvTraceset *ts, char *path);

unsigned lttv_traceset_number(LttvTraceset *s);

LttvTrace *lttv_traceset_get(LttvTraceset *s, unsigned i);

void lttv_traceset_remove(LttvTraceset *s, unsigned i);

/* An attributes table is attached to the set and to each trace in the set. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s);


#ifdef BABEL_CLEANUP
LttTrace *lttv_trace(LttvTrace *t);
#endif

/* Take a position snapshot */
LttvTracesetPosition *lttv_traceset_create_position(LttvTraceset *traceset);

/* Destroy position snapshot */
void lttv_traceset_destroy_position(LttvTracesetPosition *traceset_pos);

void lttv_traceset_seek_to_position(LttvTracesetPosition *traceset_pos);

guint lttv_traceset_get_cpuid_from_event(LttvEvent *event);

const char *lttv_traceset_get_name_from_event(LttvEvent *event);

#endif // TRACESET_H
