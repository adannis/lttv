#ifndef TYPE_H
#define TYPE_H

#include <ltt/ltt.h>


/* Different types allowed */

typedef enum _ltt_type_enum 
{ LTT_INT, LTT_UINT, LTT_FLOAT, LTT_STRING, LTT_ENUM, LTT_ARRAY, 
  LTT_SEQUENCE, LTT_STRUCT
} ltt_type_enum;


/* All event types, data types and fields belong to their trace and 
   are released at the same time. */

/* Obtain the name, description, facility, facility relative id, global id, 
   type and root field for an eventtype */

char *ltt_eventtype_name(ltt_eventtype *et);

char *ltt_eventtype_description(ltt_eventtype *et);

ltt_facility *ltt_eventtype_facility(ltt_eventtype *et);

unsigned *ltt_eventtype_relative_id(ltt_eventtype *et);

unsigned *ltt_eventtype_id(ltt_eventtype *et);

ltt_type *ltt_eventtype_type(ltt_eventtype *et);

ltt_field *ltt_eventtype_field(ltt_eventtype *et);


/* obtain the type name and size. The size is the number of bytes for
   primitive types (INT, UINT, FLOAT, ENUM), or the size for the unsigned
   integer length count for sequences. */
 
char *ltt_type_name(ltt_type *t);

ltt_type_enum ltt_type_class(ltt_type *t);

unsigned ltt_type_size(ltt_tracefile * tf, ltt_type *t); 


/* The type of nested elements for arrays and sequences. */

ltt_type *ltt_type_element_type(ltt_type *t);


/* The number of elements for arrays. */

unsigned ltt_type_element_number(ltt_type *t);


/* The number of data members for structures. */

unsigned ltt_type_member_number(ltt_type *t);


/* The type of a data member in a structure. */

ltt_type *ltt_type_member_type(ltt_type *t, unsigned i);


/* For enumerations, obtain the symbolic string associated with a value
   (0 to n - 1 for an enumeration of n elements). */

char *ltt_enum_string_get(ltt_type *t, unsigned i);


/* The fields form a tree representing a depth first search of the 
   corresponding event type directed acyclic graph. Fields for arrays and
   sequences simply point to one nested field representing the currently
   selected element among all the (identically typed) elements. For structures,
   a nested field exists for each data member. Each field stores the
   platform/trace specific offset values (for efficient access) and
   points back to the corresponding ltt_type for the rest. */

ltt_field *ltt_field_element(ltt_field *f);

ltt_field *ltt_field_member(ltt_field *f, unsigned i);

ltt_type *ltt_field_type(ltt_field *f);

#endif // TYPE_H
