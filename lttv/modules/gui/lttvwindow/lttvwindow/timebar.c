/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2010 Yannick Brosseau
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include "timebar.h"

#include "timeentry.h"

#include <gtk/gtkeventbox.h>
#include <gtk/gtkvseparator.h>
#include <lttv/time.h>

enum {
	SIGNAL_START_TIME_CHANGED,
	SIGNAL_END_TIME_CHANGED,
	SIGNAL_CURRENT_TIME_CHANGED,
	LAST_SIGNAL
};

static void timebar_class_init(TimebarClass *klass);
static void timebar_init(Timebar      *ttt);

static guint timebar_signals[LAST_SIGNAL] = { 0 };

static void on_start_time_value_changed(Timeentry *spinbutton,
					gpointer user_data);
static void on_end_time_value_changed(Timeentry *spinbutton,
				gpointer user_data);
static void on_interval_time_value_changed(Timeentry *spinbutton,
					gpointer user_data);
static void on_current_time_value_changed(Timeentry *spinbutton,
					gpointer user_data);

static void update_interval(Timebar *timebar);

static inline LttTime timeentry_get_ltt_time(Timeentry *timeentry)
{
	LttTime time;

	timeentry_get_time(timeentry,
			&time.tv_sec,
			&time.tv_nsec);
	return time;
}

GType timebar_get_type(void)
{
	static GType tb_type = 0;

	if (!tb_type) {
		const GTypeInfo tb_info =
			{
				sizeof (TimebarClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) timebar_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (Timebar),
				0,    /* n_preallocs */
				(GInstanceInitFunc) timebar_init,
			};

		tb_type = g_type_register_static(GTK_TYPE_HBOX,
						"Timebar",
						&tb_info,
						0);
	}

	return tb_type;
}


static void timebar_class_init(TimebarClass *klass)
{
	timebar_signals[SIGNAL_START_TIME_CHANGED] = g_signal_new("start-time-changed",
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET(TimebarClass, timebar),
					NULL,
					NULL,               
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);

	timebar_signals[SIGNAL_END_TIME_CHANGED] = g_signal_new("end-time-changed",
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET(TimebarClass, timebar),
					NULL,
					NULL,               
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	timebar_signals[SIGNAL_CURRENT_TIME_CHANGED] = g_signal_new("current-time-changed",
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET(TimebarClass, timebar),
					NULL,
					NULL,               
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}


static void timebar_init(Timebar *timebar)
{
	/* Title label */
	timebar->title_eventbox = gtk_event_box_new();
	gtk_widget_show(timebar->title_eventbox);

	timebar->title_label = gtk_label_new("Time Frame ");
	gtk_widget_show(timebar->title_label);
	gtk_container_add(GTK_CONTAINER(timebar->title_eventbox), timebar->title_label);

	/* Start time entry */
	timebar->start_timeentry = timeentry_new("Start: ");
	gtk_widget_show(timebar->start_timeentry);

	/* End time entry */
	timebar->end_timeentry = timeentry_new("End: ");
	gtk_widget_show(timebar->end_timeentry);

	/* Interval time entry */
	timebar->interval_timeentry = timeentry_new("Time Interval: ");
	gtk_widget_show(timebar->interval_timeentry);

	/* Current time entry */
	timebar->current_timeentry = timeentry_new("Current Time: ");
	gtk_widget_show(timebar->current_timeentry);

	/* Pack all the widget in the timebar box */
	GtkWidget *temp_widget;
  
	gtk_box_pack_start (GTK_BOX(timebar), timebar->title_eventbox, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX(timebar), timebar->start_timeentry, FALSE, FALSE, 0);

	temp_widget = gtk_vseparator_new();
	gtk_widget_show(temp_widget);
	gtk_box_pack_start (GTK_BOX(timebar), temp_widget, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX(timebar), timebar->end_timeentry, FALSE, FALSE, 0);

	temp_widget = gtk_vseparator_new();
	gtk_widget_show(temp_widget);
	gtk_box_pack_start (GTK_BOX(timebar), temp_widget, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX(timebar), timebar->interval_timeentry, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX(timebar), timebar->current_timeentry, FALSE, FALSE, 0);
	temp_widget = gtk_vseparator_new();
	gtk_widget_show(temp_widget);
	gtk_box_pack_end (GTK_BOX(timebar), temp_widget, FALSE, FALSE, 2);

	/* Set an initial time */
	timebar_set_minmax_time(timebar, &ltt_time_zero, &ltt_time_one);

	/* Connect signals */
	g_signal_connect ((gpointer) timebar->start_timeentry, "time-changed",
			G_CALLBACK(on_start_time_value_changed),
			timebar);
	g_signal_connect ((gpointer) timebar->end_timeentry, "time-changed",
			G_CALLBACK(on_end_time_value_changed),
			timebar);
	timebar->interval_handler_id =
		g_signal_connect ((gpointer) timebar->interval_timeentry, "time-changed",
				G_CALLBACK (on_interval_time_value_changed),
				timebar);
	g_signal_connect ((gpointer) timebar->current_timeentry, "time-changed",
			G_CALLBACK(on_current_time_value_changed),
			timebar);
}

GtkWidget *timebar_new(void)
{
	return GTK_WIDGET(g_object_new (TIMEBAR_TYPE, NULL));
}

void timebar_set_current_time(Timebar *timebar, const LttTime* time)
{
	if (time == NULL) {
		return;
	}

	timeentry_set_time(TIMEENTRY(timebar->current_timeentry),
					time->tv_sec,
					time->tv_nsec);
}

void timebar_set_start_time(Timebar *timebar, const LttTime* time)
{
	if (time == NULL) {
		return;
	}

	timeentry_set_time(TIMEENTRY(timebar->start_timeentry),
					time->tv_sec,
					time->tv_nsec);

	update_interval(timebar);
}

void timebar_set_end_time(Timebar *timebar, const LttTime* time)
{
	if (time == NULL) {
		return;
	}

	timeentry_set_time(TIMEENTRY(timebar->end_timeentry),
					time->tv_sec,
					time->tv_nsec);
	update_interval(timebar);
}

void timebar_set_minmax_time(Timebar *timebar,
			const LttTime *min_time,
			const LttTime *max_time)
{
	LttTime new_interval_length;
	LttTime start_max_time;
	LttTime end_min_time;

	/* Need to set both min_time and max_time */
	if (min_time == NULL || max_time == NULL) {
		return;
	}
	/* Do nothing if there is no change */
	if (ltt_time_compare(timebar->min_time, *min_time) == 0 &&
		ltt_time_compare(timebar->max_time, *max_time) == 0
		) {
		return;
	}
	/* null-checked already */
	timebar->min_time = *min_time;
	timebar->max_time = *max_time;

	if (ltt_time_compare(timebar->min_time, timebar->max_time) == 0) {

		/* If the min and max are equal set the same values, which will
		   disable all the widgets of the timebar */
		new_interval_length.tv_sec = 0;
		new_interval_length.tv_nsec = 1;

		start_max_time.tv_sec = timebar->max_time.tv_sec;
		start_max_time.tv_nsec = timebar->max_time.tv_nsec;

		end_min_time.tv_sec = timebar->min_time.tv_sec;
		end_min_time.tv_nsec = timebar->min_time.tv_nsec;

	} else {
		/* Special minmax (to keep a minimum interval of 1 nsec */
		/* start max time is max minus 1 nsec */
		if (timebar->max_time.tv_nsec == 0) {
			start_max_time.tv_sec = timebar->max_time.tv_sec - 1;
			start_max_time.tv_nsec = NANOSECONDS_PER_SECOND - 1;
		} else {
			start_max_time.tv_sec = timebar->max_time.tv_sec;
			start_max_time.tv_nsec = timebar->max_time.tv_nsec - 1;
		}

		/* end min time is min plus 1 nsec */
		if (timebar->min_time.tv_nsec + 1 == NANOSECONDS_PER_SECOND) {
			end_min_time.tv_sec = timebar->min_time.tv_sec + 1;
			end_min_time.tv_nsec = 0;
		} else {
			end_min_time.tv_sec = timebar->min_time.tv_sec;
			end_min_time.tv_nsec = timebar->min_time.tv_nsec + 1;
		}

		/* Compute max interval */
		new_interval_length = ltt_time_sub(timebar->max_time,
						timebar->min_time);
	}


	/* Update widgets */
	timeentry_set_minmax_time(TIMEENTRY(timebar->start_timeentry),
				timebar->min_time.tv_sec,
				timebar->min_time.tv_nsec,
				start_max_time.tv_sec,
				start_max_time.tv_nsec);
	timeentry_set_minmax_time(TIMEENTRY(timebar->end_timeentry),
				end_min_time.tv_sec,
				end_min_time.tv_nsec,
				timebar->max_time.tv_sec,
				timebar->max_time.tv_nsec);
	timeentry_set_minmax_time(TIMEENTRY(timebar->current_timeentry),
				timebar->min_time.tv_sec,
				timebar->min_time.tv_nsec,
				timebar->max_time.tv_sec,
				timebar->max_time.tv_nsec);


	timeentry_set_minmax_time(TIMEENTRY(timebar->interval_timeentry),
				0,
				1,
				new_interval_length.tv_sec,
				new_interval_length.tv_nsec);	
}

LttTime timebar_get_start_time(Timebar *timebar)
{
	return timeentry_get_ltt_time(TIMEENTRY(timebar->start_timeentry));
}

LttTime timebar_get_end_time(Timebar *timebar)
{
	return timeentry_get_ltt_time(TIMEENTRY(timebar->end_timeentry));
}

LttTime timebar_get_current_time(Timebar *timebar)
{
	return timeentry_get_ltt_time(TIMEENTRY(timebar->current_timeentry));
}

static void update_interval(Timebar *timebar)
{
	LttTime start_time = timeentry_get_ltt_time(TIMEENTRY(timebar->start_timeentry));
	LttTime end_time = timeentry_get_ltt_time(TIMEENTRY(timebar->end_timeentry));
	LttTime new_interval;

	/* Compute max interval */
	new_interval = ltt_time_sub(end_time,
				start_time);
	
	/* Don't trigger the signal when we update the interval */
	g_signal_handler_block(timebar->interval_timeentry,
			timebar->interval_handler_id);

	timeentry_set_time(TIMEENTRY(timebar->interval_timeentry),
				new_interval.tv_sec,
				new_interval.tv_nsec);

	g_signal_handler_unblock(timebar->interval_timeentry,
			timebar->interval_handler_id);
}

static void on_start_time_value_changed(Timeentry *timeentry,
					gpointer user_data)
{
	Timebar *timebar = (Timebar *)user_data;

	update_interval(timebar);

	g_signal_emit(timebar,
		timebar_signals[SIGNAL_START_TIME_CHANGED], 0);
}

static void on_end_time_value_changed(Timeentry *timeentry,
				gpointer user_data)
{
	Timebar *timebar = (Timebar *)user_data;

	update_interval(timebar);

	g_signal_emit(timebar,
		timebar_signals[SIGNAL_END_TIME_CHANGED], 0);
}

static void on_interval_time_value_changed (Timeentry *timeentry,
				gpointer user_data)
{
	Timebar *timebar = (Timebar *)user_data;

	LttTime new_interval = timeentry_get_ltt_time(TIMEENTRY(timebar->interval_timeentry));

	LttTime start_time = timebar_get_start_time(timebar);

	LttTime new_end_time;

	gboolean need_interval_update = FALSE;

	/* Lock the start and change the end */
	new_end_time = ltt_time_add(start_time, new_interval);

	/* We cannot push further the max end */
	if (ltt_time_compare(new_end_time, timebar->max_time) > 0) {
		/* Set the end to the max and pull on the start */
		new_end_time = timebar->max_time;
		LttTime new_start_time = ltt_time_sub(new_end_time, new_interval);

		/* We cannot pull before the min start */
		if (ltt_time_compare(new_start_time, timebar->min_time) < 0) {
			/* Set the interval to the max */
			new_start_time = timebar->min_time;
			need_interval_update = TRUE;
		}
		timebar_set_start_time(timebar, &new_start_time);
	}
	timebar_set_end_time(timebar, &new_end_time);

	if (need_interval_update) {
		update_interval(timebar);
	}
}

static void on_current_time_value_changed(Timeentry *timeentry,
					gpointer user_data)
{
	Timebar *timebar = (Timebar *)user_data;

	g_signal_emit(timebar, 
		timebar_signals[SIGNAL_CURRENT_TIME_CHANGED], 0);
}


