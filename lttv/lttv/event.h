/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2012 Yannick Brosseau
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
#ifndef LTTV_EVENT_H
#define LTTV_EVENT_H

#include <lttv/time.h>
#include <lttv/state.h>
/* Forward declaration */
struct bt_ctf_event;
//struct LttvTraceState;
/* 
   Basic event container used through LTTV
*/
typedef struct
{
  struct bt_ctf_event *bt_event;
  LttvTraceState *state;
} LttvEvent;

LttTime lttv_event_get_timestamp(LttvEvent *event);
unsigned long lttv_event_get_long_unsigned(LttvEvent *event, const char* field);
long lttv_event_get_long(LttvEvent *event, const char* field);
/*unsigned int lttv_event_get_int_unsigned(LttvEvent *event, const char* field);
int lttv_event_get_int(LttvEvent *event, const char* field);
*/

char* lttv_event_get_string(LttvEvent *event, const char* field);


#endif /* LTTV_EVENT_H */
