#include <stdio.h>
#include <asm/types.h>
#include <linux/byteorder/swab.h>
#include <ltt/LTTTypes.h> 
#include "parser.h"
#include <ltt/event.h>

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype_id: get event type id 
 *                            (base id + position of the event)
 *Input params
 *    e                     : an instance of an event type   
 *Return value
 *    unsigned              : event type id
 ****************************************************************************/

unsigned ltt_event_eventtype_id(LttEvent *e)
{
  return (unsigned) e->event_id;
}

/*****************************************************************************
 *Function name
 *    ltt_event_facility : get the facility of the event
 *Input params
 *    e                  : an instance of an event type   
 *Return value
 *    LttFacility *     : the facility of the event
 ****************************************************************************/

LttFacility *ltt_event_facility(LttEvent *e)
{
  LttTrace * trace = e->tracefile->trace;
  unsigned id = e->event_id;
  return ltt_trace_facility_by_id(trace,id);
}

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype : get the event type of the event
 *Input params
 *    e                   : an instance of an event type   
 *Return value
 *    LttEventType *     : the event type of the event
 ****************************************************************************/

LttEventType *ltt_event_eventtype(LttEvent *e)
{
  LttFacility* facility = ltt_event_facility(e);
  if(!facility) return NULL;
  return facility->events[e->event_id - facility->base_id];
}

/*****************************************************************************
 *Function name
 *    ltt_event_field : get the root field of the event
 *Input params
 *    e               : an instance of an event type   
 *Return value
 *    LttField *      : the root field of the event
 ****************************************************************************/

LttField *ltt_event_field(LttEvent *e)
{
  LttEventType * event_type = ltt_event_eventtype(e);
  if(!event_type) return NULL;
  return event_type->root_field;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the time of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttTime       : the time of the event
 ****************************************************************************/

LttTime ltt_event_time(LttEvent *e)
{
  return e->event_time;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the cycle count of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttCycleCount  : the cycle count of the event
 ****************************************************************************/

LttCycleCount ltt_event_cycle_count(LttEvent *e)
{
  return e->event_cycle_count;
}

/*****************************************************************************
 *Function name
 *    ltt_event_cpu_i: get the cpu id where the event happens
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    unsigned       : the cpu id
 ****************************************************************************/

unsigned ltt_event_cpu_id(LttEvent *e)
{  
  return (unsigned)atoi(e->tracefile->name);
}

/*****************************************************************************
 *Function name
 *    ltt_event_data : get the raw data for the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    void *         : pointer to the raw data for the event
 ****************************************************************************/

void *ltt_event_data(LttEvent *e)
{
  return e->data;
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_number
 *                   : The number of elements in a sequence field is specific
 *                     to each event. This function returns the number of 
 *                     elements for an array or sequence field in an event.
 *Input params
 *    e              : an instance of an event type   ????
 *    f              : a field of the instance
 *Return value
 *    unsigned       : the number of elements for an array/sequence field
 ****************************************************************************/

unsigned ltt_event_field_element_number(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return 0;

  return f->field_type->element_number;
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_select
 *                   : Set the currently selected element for a sequence or
 *                     array field
 *Input params
 *    e              : an instance of an event type   ????
 *    f              : a field of the instance
 *    i              : the ith element
 ****************************************************************************/

void ltt_event_field_element_select(LttEvent *e, LttField *f, unsigned i)
{
   if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
     return ;
   
   if(f->field_type->element_number < i || i == 0) return;

   f->current_element = i - 1;
}

/*****************************************************************************
 * These functions extract data from an event after architecture specific
 * conversions
 ****************************************************************************/

unsigned ltt_event_get_unsigned(LttEvent *e, LttField *f)
{
  LttArchSize rSize = e->tracefile->trace->system_description->size;
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;
  LttTypeEnum t = f->field_type->type_class;

  if(t != LTT_UINT || t != LTT_ENUM)
    g_error("The type of the field is not unsigned int\n");

  if(rSize == LTT_LP32){
    if(f->field_size != 2)
      g_error("The type of the field is not unsigned int: uint16_t\n");
    else{
      uint16_t x = *(uint16_t *)(e->data + f->offset_root);
      return (unsigned) (revFlag ? BREV16(x) : x); 
    }
  }else if(rSize == LTT_ILP32 || rSize == LTT_LP64){
    if(f->field_size != 4)
      g_error("The type of the field is not unsigned int: uint32_t\n");
    else{
      uint32_t x = *(uint32_t *)(e->data + f->offset_root);
      return (unsigned) (revFlag ? BREV32(x): x);
    }
  }else if(rSize == LTT_ILP64){
    if(f->field_size != 8)
      g_error("The type of the field is not unsigned int: uint64_t\n");
    else{
      uint64_t x = *(uint64_t *)(e->data + f->offset_root);
      return (unsigned) (revFlag ? BREV64(x): x);
    }
  }
}

int ltt_event_get_int(LttEvent *e, LttField *f)
{
  LttArchSize rSize = e->tracefile->trace->system_description->size;
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_INT)
    g_error("The type of the field is not int\n");

  if(rSize == LTT_LP32){
    if(f->field_size != 2)
      g_error("The type of the field is not int: int16_t\n");
    else{
      int16_t x = *(int16_t *)(e->data + f->offset_root);
      return (int) (revFlag ? BREV16(x) : x); 
    }
  }else if(rSize == LTT_ILP32 || rSize == LTT_LP64){
    if(f->field_size != 4)
      g_error("The type of the field is not int: int32_t\n");
    else{
      int32_t x = *(int32_t *)(e->data + f->offset_root);
      return (int) (revFlag ? BREV32(x): x);
    }
  }else if(rSize == LTT_ILP64){
    if(f->field_size != 8)
      g_error("The type of the field is not int: int64_t\n");
    else{
      int64_t x = *(int64_t *)(e->data + f->offset_root);
      return (int) (revFlag ? BREV64(x): x);
    }
  }
}

unsigned long ltt_event_get_long_unsigned(LttEvent *e, LttField *f)
{
  LttArchSize rSize = e->tracefile->trace->system_description->size;
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;
  LttTypeEnum t = f->field_type->type_class;

  if(t != LTT_UINT || t != LTT_ENUM)
    g_error("The type of the field is not unsigned long\n");

  if(rSize == LTT_LP32 || rSize == LTT_ILP32 ){
    if(f->field_size != 4)
      g_error("The type of the field is not unsigned long: uint32_t\n");
    else{
      uint32_t x = *(uint32_t *)(e->data + f->offset_root);
      return (unsigned long) (revFlag ? BREV32(x) : x); 
    }
  }else if(rSize == LTT_LP64 || rSize == LTT_ILP64){
    if(f->field_size != 8)
      g_error("The type of the field is not unsigned long: uint64_t\n");
    else{
      uint64_t x = *(uint64_t *)(e->data + f->offset_root);
      return (unsigned long) (revFlag ? BREV64(x): x);
    }
  }
}

long int ltt_event_get_long_int(LttEvent *e, LttField *f)
{
  LttArchSize rSize = e->tracefile->trace->system_description->size;
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if( f->field_type->type_class != LTT_INT)
    g_error("The type of the field is not long int\n");

  if(rSize == LTT_LP32 || rSize == LTT_ILP32 ){
    if(f->field_size != 4)
      g_error("The type of the field is not long int: int32_t\n");
    else{
      int32_t x = *(int32_t *)(e->data + f->offset_root);
      return (long) (revFlag ? BREV32(x) : x); 
    }
  }else if(rSize == LTT_LP64 || rSize == LTT_ILP64){
    if(f->field_size != 8)
      g_error("The type of the field is not long int: int64_t\n");
    else{
      int64_t x = *(int64_t *)(e->data + f->offset_root);
      return (long) (revFlag ? BREV64(x): x);
    }
  }
}

float ltt_event_get_float(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_FLOAT || 
     (f->field_type->type_class == LTT_FLOAT && f->field_size != 4))
    g_error("The type of the field is not float\n");

  if(revFlag == 0) return *(float *)(e->data + f->offset_root);
  else{
    uint32_t aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 4);
    aInt = ___swab32(aInt);
    return *((float*)&aInt);
  }
}

double ltt_event_get_double(LttEvent *e, LttField *f)
{
  int revFlag = e->tracefile->trace->my_arch_endian == 
                e->tracefile->trace->system_description->endian ? 0:1;

  if(f->field_type->type_class != LTT_FLOAT || 
     (f->field_type->type_class == LTT_FLOAT && f->field_size != 8))
    g_error("The type of the field is not double\n");

  if(revFlag == 0) return *(double *)(e->data + f->offset_root);
  else{
    uint64_t aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 8);
    aInt = ___swab64(aInt);
    return *((double *)&aInt);
  }
}

/*****************************************************************************
 * The string obtained is only valid until the next read from
 * the same tracefile. ????
 ****************************************************************************/

char *ltt_event_get_string(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_STRING)
    g_error("The field contains no string\n");
  return (char*)g_strdup((char*)(e->data + f->offset_root));
}
