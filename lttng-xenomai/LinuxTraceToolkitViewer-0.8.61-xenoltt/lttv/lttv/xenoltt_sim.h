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

#ifndef XENOLTT_SIM_H
#define XENOLTT_SIM_H

#include <glib.h>
#include <lttv/state.h>
#include <lttv/stats.h>

// Hook wrapper. call_data is a trace context.
gboolean lttv_xenoltt_sim_hook_add_event_hooks(void *hook_data, void *call_data);
void lttv_xenoltt_sim_add_event_hooks(LttvTracesetStats *self);

// Hook wrapper. call_data is a trace context.
gboolean lttv_xenoltt_sim_hook_remove_event_hooks(void *hook_data, void *call_data);
void lttv_xenoltt_sim_remove_event_hooks(LttvTracesetStats *self);


typedef struct _ThreadEventData {
  gulong address;
  guint prio;
  guint period;
  LttTime creation_time;
  GQuark name;
  GArray* event_list;
  GArray* new_event_list;
} ThreadEventData;


typedef struct _EventData {
  LttTime event_time;
  GQuark name;
  LttEvent* event;
} EventData;


typedef struct _RunningThread {
  ThreadEventData* thread;
  LttTime begin_time;
  LttTime end_time;
} RunningThread;

static GArray *thread_event_list;
static GArray *running_thread;

void compute_simulation(guint index,guint period,FILE *a_file);

GArray* get_thread_list();

#endif // XENOLTT_SIM_H
