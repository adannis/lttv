/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2005 Michel Dagenais
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

/*
  read_token

  read_expression
    ( read expr )
    simple expr [ op expr ]

  read_simple_expression
    read_field_path [ rel value ]

  read_field_path
    read_field_component [. field path]

  read_field_component
    name [ \[ value \] ]

  data struct:
  and/or(left/right)
  not(child)
  op(left/right)
  path(component...) -> field
   
  consist in AND, OR and NOT nested expressions, forming a tree with 
  simple relations as leaves. The simple relations test is a field
  in an event is equal, not equal, smaller, smaller or equal, larger, or
  larger or equal to a specified value. 
*/

/*
 *  YET TO BE ANSWERED
 *  - none yet
 */

/*
 *  TODO 
 *  - refine switch of expression in multiple uses functions
 *    - remove the idle expressions in the tree **** 
 *  - add the current simple expression to the tree
 *    * clear the field_path array after use
 */

#include <lttv/filter.h>

/*
GQuark
  LTTV_FILTER_TRACE,
  LTTV_FILTER_TRACESET,
  LTTV_FILTER_TRACEFILE,
  LTTV_FILTER_STATE,
  LTTV_FILTER_EVENT,
  LTTV_FILTER_NAME,
  LTTV_FILTER_CATEGORY,
  LTTV_FILTER_TIME,
  LTTV_FILTER_TSC,
  LTTV_FILTER_PID,
  LTTV_FILTER_PPID,
  LTTV_FILTER_C_TIME,
  LTTV_FILTER_I_TIME,
  LTTV_FILTER_P_NAME,
  LTTV_FILTER_EX_MODE,
  LTTV_FILTER_EX_SUBMODE,
  LTTV_FILTER_P_STATUS,
  LTTV_FILTER_CPU;
*/

/**
 * @fn void lttv_filter_tree_add_node(GPtrArray*,LttvFilterTree*,LttvLogicalOp)
 * 
 * add a node to the current tree
 * FIXME: Might be used to lower coding in lttv_filter_new switch expression
 * @param stack the tree stack
 * @param subtree the subtree if available (pointer or NULL)
 * @param op the logical operator that will form the node
 */
void
lttv_filter_tree_add_node(GPtrArray* stack, LttvFilterTree* subtree, LttvLogicalOp op) {

  LttvFilterTree* t1 = NULL;
  LttvFilterTree* t2 = NULL;

  t1 = (LttvFilterTree*)g_ptr_array_index(stack,stack->len-1);
  while(t1->right != LTTV_TREE_IDLE) t1 = (LttvFilterTree*)t1->r_child.t;
  t2 = lttv_filter_tree_new();
  t2->node = op;
  if(subtree != NULL) {
    t2->left = LTTV_TREE_NODE;
    t2->l_child.t = (LttvFilterTree*)subtree;
    subtree = NULL;
    t1->right = LTTV_TREE_NODE;
    t1->r_child.t = t2;
  } else {
//  a_simple_expression->value = a_field_component->str;
//    a_field_component = g_string_new("");
    t2->left = LTTV_TREE_LEAF;
//    t2->l_child.leaf = a_simple_expression;
//  a_simple_expression = g_new(lttv_simple_expression,1);
    t1->right = LTTV_TREE_NODE;
    t1->r_child.t = t2; 
  }
  
}


/**
 * @fn LttvSimpleExpression* lttv_simple_expression_new()
 * 
 * Constructor for LttvSimpleExpression
 * @return pointer to new LttvSimpleExpression
 */
LttvSimpleExpression* 
lttv_simple_expression_new() {

  LttvSimpleExpression* se = g_new(LttvSimpleExpression,1);

  se->field = LTTV_FILTER_UNDEFINED;
  se->op = NULL;
  se->offset = 0;
  se->value.v_uint64 = NULL;

  return se;
}

/**
 *  @fn gboolean lttv_simple_expression_add_field(GPtrArray*,LttvSimpleExpression*)
 * 
 *  Parse through filtering field hierarchy as specified 
 *  by user.  This function compares each value to 
 *  predetermined quarks
 *  @param fp The field path list
 *  @param se current simple expression
 *  @return success/failure of operation
 */
gboolean
lttv_simple_expression_assign_field(GPtrArray* fp, LttvSimpleExpression* se) {

  GString* f = NULL;
  
  if(fp->len < 2) return FALSE;
  g_assert(f=g_ptr_array_remove_index(fp,0)); 
  
  /*
   * Parse through the specified 
   * hardcoded fields.
   *
   * Take note however that the 
   * 'event' subfields might change 
   * depending on values specified 
   * in core.xml file.  Hence, if 
   * none of the subfields in the 
   * array match the hardcoded 
   * subfields, it will be considered 
   * as a dynamic field
   */
  if(!g_strcasecmp(f->str,"trace") ) {
    /*
     * Possible values:
     *  trace.name
     */
    g_string_free(f,TRUE);
    f=g_ptr_array_remove_index(fp,0);
    if(!g_strcasecmp(f->str,"name")) {
      se->field = LTTV_FILTER_TRACE_NAME;    
    }
  } else if(!g_strcasecmp(f->str,"traceset") ) {
    /* 
     * FIXME: not yet implemented !
     */
  } else if(!g_strcasecmp(f->str,"tracefile") ) {
    /*
     * Possible values:
     *  tracefile.name
     */
    g_string_free(f,TRUE);
    f=g_ptr_array_remove_index(fp,0);
    if(!g_strcasecmp(f->str,"name")) {
      se->field = LTTV_FILTER_TRACEFILE_NAME;
    }
  } else if(!g_strcasecmp(f->str,"state") ) {
    /*
     * Possible values:
     *  state.pid
     *  state.ppid
     *  state.creation_time
     *  state.insertion_time
     *  state.process_name
     *  state.execution_mode
     *  state.execution_submode
     *  state.process_status
     *  state.cpu
     */
    g_string_free(f,TRUE);
    f=g_ptr_array_remove_index(fp,0);
    if(!g_strcasecmp(f->str,"pid") ) { 
      se->field = LTTV_FILTER_STATE_PID; 
    }
    else if(!g_strcasecmp(f->str,"ppid") ) { 
      se->field = LTTV_FILTER_STATE_PPID; 
    }
    else if(!g_strcasecmp(f->str,"creation_time") ) {
      se->field = LTTV_FILTER_STATE_CT;
    }
    else if(!g_strcasecmp(f->str,"insertion_time") ) {
      se->field = LTTV_FILTER_STATE_IT;
    }
    else if(!g_strcasecmp(f->str,"process_name") ) {
      se->field = LTTV_FILTER_STATE_P_NAME;
    }
    else if(!g_strcasecmp(f->str,"execution_mode") ) {
      se->field = LTTV_FILTER_STATE_EX_MODE;
    }
    else if(!g_strcasecmp(f->str,"execution_submode") ) {
      se->field = LTTV_FILTER_STATE_EX_SUBMODE;
    }
    else if(!g_strcasecmp(f->str,"process_status") ) {
      se->field = LTTV_FILTER_STATE_P_STATUS;
    }
    else if(!g_strcasecmp(f->str,"cpu") ) {
      se->field = LTTV_FILTER_STATE_CPU;
    }
  } else if(!g_strcasecmp(f->str,"event") ) {
    /*
     * Possible values:
     *  event.name
     *  event.category
     *  event.time
     *  event.tsc
     */
    g_string_free(f,TRUE);
    f=g_ptr_array_remove_index(fp,0);
    if(!g_strcasecmp(f->str,"name") ) {
      se->field = LTTV_FILTER_EVENT_NAME;
    }
    else if(!g_strcasecmp(f->str,"category") ) {
      /*
       * FIXME: Category not yet functional in lttv
       */
      se->field = LTTV_FILTER_EVENT_CATEGORY;
    }
    else if(!g_strcasecmp(f->str,"time") ) {
      se->field = LTTV_FILTER_EVENT_TIME;
    }
    else if(!g_strcasecmp(f->str,"tsc") ) {
      se->field = LTTV_FILTER_EVENT_TSC;
    }
    else {  /* core.xml specified options */
      se->field = LTTV_FILTER_EVENT_FIELD;
    }
  } else {
    g_warning("Unrecognized field in filter string");
  }

  /* free memory for last string */
  g_string_free(f,TRUE);

  /* array should be empty */
  g_assert(fp->len == 0);
 
  g_print("field: %i\n",se->field);
  if(se->field == LTTV_FILTER_UNDEFINED) {
    g_warning("The specified field was not recognized !");
    return FALSE;
  }  
  return TRUE;  
}

/**
 *  @fn gboolean lttv_simple_expression_assign_operator(LttvSimpleExpression*,LttvExpressionOp)
 * 
 *  Sets the function pointer for the current
 *  Simple Expression
 *  @param se current simple expression
 *  @return success/failure of operation
 */
gboolean lttv_simple_expression_assign_operator(LttvSimpleExpression* se, LttvExpressionOp op) {
     
//  g_print("se->field = %i\n",se->field);
//  g_print("se->offset = %i\n",se->offset);
//  g_print("se->op = %p\n",se->op);
//  g_print("se->value = %s\n",se->value);
   
  switch(se->field) {
     /* 
      * string
      */
     case LTTV_FILTER_TRACE_NAME:
     case LTTV_FILTER_TRACEFILE_NAME:
     case LTTV_FILTER_STATE_P_NAME:
     case LTTV_FILTER_EVENT_NAME:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_string;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_string;
           break;
         default:
           g_warning("Error encountered in operator assignment = or != expected");
           return FALSE;
       }
       break;
     /* 
      * integer
      */
     case LTTV_FILTER_STATE_PID:
     case LTTV_FILTER_STATE_PPID:
     case LTTV_FILTER_STATE_EX_MODE:
     case LTTV_FILTER_STATE_EX_SUBMODE:
     case LTTV_FILTER_STATE_P_STATUS:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_uint64;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_uint64;
           break;
         case LTTV_FIELD_LT:
           se->op = lttv_apply_op_lt_uint64;
           break;
         case LTTV_FIELD_LE:
           se->op = lttv_apply_op_le_uint64;
           break;
         case LTTV_FIELD_GT:
           se->op = lttv_apply_op_gt_uint64;
           break;
         case LTTV_FIELD_GE:
           se->op = lttv_apply_op_ge_uint64;
           break;
         default:
           g_warning("Error encountered in operator assignment");
           return FALSE;
       }
       break;
     /*
      * double
      */
     case LTTV_FILTER_STATE_CT:
     case LTTV_FILTER_STATE_IT:
     case LTTV_FILTER_EVENT_TIME:
     case LTTV_FILTER_EVENT_TSC:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_double;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_double;
           break;
         case LTTV_FIELD_LT:
           se->op = lttv_apply_op_lt_double;
           break;
         case LTTV_FIELD_LE:
           se->op = lttv_apply_op_le_double;
           break;
         case LTTV_FIELD_GT:
           se->op = lttv_apply_op_gt_double;
           break;
         case LTTV_FIELD_GE:
           se->op = lttv_apply_op_ge_double;
           break;
         default:
           g_warning("Error encountered in operator assignment");
           return FALSE;
       }
       break;
     default:
       g_warning("Error encountered in operator assignation ! Field type:%i",se->field);
       return FALSE;
   }
  
  return TRUE;

}

/**
 *  @fn void lttv_simple_expression_assign_value(LttvSimpleExpression*,char*)
 *
 *  Assign the value field to the current LttvSimpleExpression
 *  @param se pointer to the current LttvSimpleExpression
 *  @param value string value for simple expression
 */
gboolean lttv_simple_expression_assign_value(LttvSimpleExpression* se, char* value) {

  g_print("se->value:%s\n",value);
  
  switch(se->field) {
     /* 
      * string
      */
     case LTTV_FILTER_TRACE_NAME:
     case LTTV_FILTER_TRACEFILE_NAME:
     case LTTV_FILTER_STATE_P_NAME:
     case LTTV_FILTER_EVENT_NAME:
       se->value.v_string = value;
       break;
     /* 
      * integer
      */
     case LTTV_FILTER_STATE_PID:
     case LTTV_FILTER_STATE_PPID:
     case LTTV_FILTER_STATE_EX_MODE:
     case LTTV_FILTER_STATE_EX_SUBMODE:
     case LTTV_FILTER_STATE_P_STATUS:
       se->value.v_uint64 = atoi(value);
       g_free(value);
       break;
     /*
      * double
      */
     case LTTV_FILTER_STATE_CT:
     case LTTV_FILTER_STATE_IT:
     case LTTV_FILTER_EVENT_TIME:
     case LTTV_FILTER_EVENT_TSC:
       se->value.v_double = atof(value);
       g_free(value);
       break;
     default:
       g_warning("Error encountered in value assignation ! Field type = %i",se->field);
       return FALSE;
   }
  
  return TRUE;
  
}

/**
 *  @fn void lttv_simple_expression_destroy(LttvSimpleExpression*)
 *
 *  Disallocate memory for the current 
 *  simple expression
 *  @param se pointer to the current LttvSimpleExpression
 */
void
lttv_simple_expression_destroy(LttvSimpleExpression* se) {
  
 // g_free(se->value);
  switch(se->field) {
     case LTTV_FILTER_TRACE_NAME:
     case LTTV_FILTER_TRACEFILE_NAME:
     case LTTV_FILTER_STATE_P_NAME:
     case LTTV_FILTER_EVENT_NAME:
       g_free(se->value.v_string);
       break;
  }
  g_free(se);

}

/**
 *  @fn gint lttv_struct_type(gint)
 * 
 *  Finds the structure type depending 
 *  on the fields in parameters
 *  @params ft Field of the current structure
 *  @return LttvStructType enum or -1 for error
 */
gint
lttv_struct_type(gint ft) {
  
    switch(ft) {
        case LTTV_FILTER_TRACE_NAME:
            return LTTV_FILTER_TRACE;
            break;
        case LTTV_FILTER_TRACEFILE_NAME:
            return LTTV_FILTER_TRACEFILE;
            break;
        case LTTV_FILTER_STATE_PID:
        case LTTV_FILTER_STATE_PPID:
        case LTTV_FILTER_STATE_CT:
        case LTTV_FILTER_STATE_IT:
        case LTTV_FILTER_STATE_P_NAME:
        case LTTV_FILTER_STATE_EX_MODE:
        case LTTV_FILTER_STATE_EX_SUBMODE:
        case LTTV_FILTER_STATE_P_STATUS:
        case LTTV_FILTER_STATE_CPU:
            return LTTV_FILTER_STATE;
            break;
        case LTTV_FILTER_EVENT_NAME:
        case LTTV_FILTER_EVENT_CATEGORY:
        case LTTV_FILTER_EVENT_TIME:
        case LTTV_FILTER_EVENT_TSC:
        case LTTV_FILTER_EVENT_FIELD:
            return LTTV_FILTER_EVENT;
            break;
        default:
            return -1;
    }
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint64(gpointer v1, LttvFieldValue v2) {

  guint64* r = (guint64*) v1;
  return (*r == v2.v_uint64);
  
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r == v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r == v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_eq_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r == v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_eq_string(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_string(gpointer v1, LttvFieldValue v2) {
  char* r = (char*) v1;
  return (!g_strcasecmp(r,v2.v_string));
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint64(gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r != v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r != v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r != v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_ne_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r != v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_ne_string(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_string(gpointer v1, LttvFieldValue v2) {
  char* r = (char*) v1;
  return (g_strcasecmp(r,v2.v_string));
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint64(gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r < v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r < v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r < v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_lt_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r < v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint64(gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r <= v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r <= v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r <= v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_le_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r <= v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint64(gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r > v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r > v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r > v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_gt_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r > v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint64(gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r >= v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint32(gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r >= v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint16(gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r >= v2.v_uint16);
}

/**
  *  @fn gboolean lttv_apply_op_ge_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_double(gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r >= v2.v_double);
}


/**
 *  @fn LttvFilterTree* lttv_filter_tree_clone(LttvFilterTree*)
 * 
 *  Makes a copy of the current filter tree
 *  @param tree pointer to the current tree
 *  @return new copy of the filter tree
 */
LttvFilterTree*
lttv_filter_tree_clone(LttvFilterTree* tree) {

  LttvFilterTree* newtree = lttv_filter_tree_new();  

  newtree->node = tree->node;
 
  newtree->left = tree->left;
  if(newtree->left == LTTV_TREE_NODE) {
    newtree->l_child.t = lttv_filter_tree_clone(tree->l_child.t);
  } else if(newtree->left == LTTV_TREE_LEAF) {
    newtree->l_child.leaf = lttv_simple_expression_new();
    newtree->l_child.leaf->field = tree->l_child.leaf->field;
    newtree->l_child.leaf->offset = tree->l_child.leaf->offset;
    newtree->l_child.leaf->op = tree->l_child.leaf->op;
    /* FIXME: special case for string copy ! */
    newtree->l_child.leaf->value = tree->l_child.leaf->value;
  }
 
  newtree->right = tree->right;
  if(newtree->right == LTTV_TREE_NODE) {
    newtree->r_child.t = lttv_filter_tree_clone(tree->r_child.t);
  } else if(newtree->right == LTTV_TREE_LEAF) {
    newtree->r_child.leaf = lttv_simple_expression_new();
    newtree->r_child.leaf->field = tree->r_child.leaf->field;
    newtree->r_child.leaf->offset = tree->r_child.leaf->offset;
    newtree->r_child.leaf->op = tree->r_child.leaf->op;
    newtree->r_child.leaf->value = tree->r_child.leaf->value;
  }
  
  return newtree;
  
}

/**
 *  @fn LttvFilter* lttv_filter_clone(LttvFilter*)
 * 
 *  Makes a copy of the current filter
 *  @param filter pointer to the current filter
 *  @return new copy of the filter
 */
LttvFilter*
lttv_filter_clone(LttvFilter* filter) {

    
  LttvFilter* newfilter = g_new(LttvFilter,1); 

  // newfilter->expression = g_new(char,1)
  strcpy(newfilter->expression,filter->expression);

  newfilter->head = lttv_filter_tree_clone(filter->head);
  
  return newfilter;
    
}


/**
 *  @fn LttvFilter* lttv_filter_new()
 * 
 * 	Creates a new lttv_filter
 * 	@param expression filtering options string
 * 	@param t pointer to the current LttvTrace
 * 	@return the current lttv_filter or NULL if error
 */
LttvFilter*
lttv_filter_new() {

  LttvFilter* filter = g_new(LttvFilter,1);
  filter->expression = NULL;
  filter->head = NULL;
    
}

/**
 *  @fn gboolean lttv_filter_update(LttvFilter*)
 * 
 *  Updates the current LttvFilter by building 
 *  its tree based upon the expression string
 *  @param filter pointer to the current LttvFilter
 *  @return Failure/Success of operation
 */
gboolean
lttv_filter_update(LttvFilter* filter) {
    
  g_print("filter::lttv_filter_new()\n");		/* debug */
  
  if(filter->expression == NULL) return FALSE;
  
  int	
    i, 
    p_nesting=0;	/* parenthesis nesting value */

  /* trees */
  LttvFilterTree
    *tree = lttv_filter_tree_new(),   /* main tree */
    *subtree = NULL,                  /* buffer for subtrees */
    *t1,                              /* buffer #1 */
    *t2;                              /* buffer #2 */

  /* 
   * the filter
   * If the tree already exists, 
   * destroy it and build a new one
   */
  if(filter->head != NULL) lttv_filter_tree_destroy(filter->head);
  filter->head = NULL;    /* will be assigned at the end */
 
  /*
   * Tree Stack
   * each element of the list
   * is a sub tree created 
   * by the use of parenthesis in the 
   * global expression.  The final tree 
   * will be the one left at the root of 
   * the list
   */
  GPtrArray *tree_stack = g_ptr_array_new();
  g_ptr_array_add( tree_stack,(gpointer) tree );
  
  /* temporary values */
  GString *a_field_component = g_string_new(""); 
  GPtrArray *a_field_path = g_ptr_array_new(); 
  
  /* simple expression buffer */
  LttvSimpleExpression* a_simple_expression = lttv_simple_expression_new(); 
  
  /*
   *	Parse entire expression and construct
   *	the binary tree.  There are two steps 
   *	in browsing that string
   *	  1. finding boolean ops " &,|,^,! " and parenthesis " {,(,[,],),} "
   *	  2. finding simple expressions
   *	    - field path ( separated by dots )
   *	    - op ( >, <, =, >=, <=, !=)
   *	    - value ( integer, string ... )
   *	To spare computing time, the whole 
   *	string is parsed in this loop for a 
   *	O(n) complexity order.
   *
   *  When encountering logical op &,|,^
   *    1. parse the last value if any
   *    2. create a new tree
   *    3. add the expression (simple exp, or exp (subtree)) to the tree
   *    4. concatenate this tree with the current tree on top of the stack
   *  When encountering math ops >,>=,<,<=,=,!=
   *    1. add to op to the simple expression
   *    2. concatenate last field component to field path
   *  When encountering concatening ops .
   *    1. concatenate last field component to field path
   *  When encountering opening parenthesis (,{,[
   *    1. create a new subtree on top of tree stack
   *  When encountering closing parenthesis ),},]
   *    1. add the expression on right child of the current tree
   *    2. the subtree is completed, allocate a new subtree
   *    3. pop the tree value from the tree stack
   */
  
  g_print("expression: %s\n",filter->expression);
  g_print("strlen(expression): %i\n",strlen(filter->expression));
  for(i=0;i<strlen(filter->expression);i++) {
    // debug
    g_print("%c ",filter->expression[i]);
    switch(filter->expression[i]) {
      /*
       *   logical operators
       */
      case '&':   /* and */
   
        t1 = (LttvFilterTree*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_AND;
        if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {  /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
         // a_simple_expression->value = g_string_free(a_field_component,FALSE);
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
          g_print("t1:%p t1->child:%p\n",t1,t1->r_child.t);
        }
        break;
      
      case '|':   /* or */
      
        t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
         while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_OR;
        if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {    /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          //a_simple_expression->value = g_string_free(a_field_component,FALSE);
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new();
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2; 
        }
        break;
      
      case '^':   /* xor */
        
        t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_XOR;
        if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {    /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          //a_simple_expression->value = g_string_free(a_field_component,FALSE);
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2; 
        }
        break;
      
      case '!':   /* not, or not equal (math op) */
        
        if(filter->expression[i+1] == '=') {  /* != */
          g_ptr_array_add( a_field_path,(gpointer) a_field_component );
          lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
          a_field_component = g_string_new("");         
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_NE);
          i++;
        } else {  /* ! */
          t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
          while(t1->right != LTTV_TREE_IDLE) {
             g_assert(t1->right == LTTV_TREE_NODE);
             t1 = t1->r_child.t;
          }
          t2 = lttv_filter_tree_new();
          t2->node = LTTV_LOGICAL_NOT;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        }
        break;
      
      case '(':   /* start of parenthesis */
      case '[':
      case '{':
        
        p_nesting++;      /* incrementing parenthesis nesting value */
        t1 = lttv_filter_tree_new();
        g_ptr_array_add( tree_stack,(gpointer) t1 );
        break;
      
      case ')':   /* end of parenthesis */
      case ']':
      case '}':
        
        p_nesting--;      /* decrementing parenthesis nesting value */
        if(p_nesting<0 || tree_stack->len<2) {
          g_warning("Wrong filtering options, the string\n\"%s\"\n\
                     is not valid due to parenthesis incorrect use",filter->expression);	
          return FALSE;
        }
  
        /* there must at least be the root tree left in the array */
        g_assert(tree_stack->len>0);
        
        if(subtree != NULL) {   /* append subtree to current tree */
          t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
          while(t1->right != LTTV_TREE_IDLE) {
             g_assert(t1->right == LTTV_TREE_NODE);
             t1 = t1->r_child.t;
          }
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = subtree;
          subtree = g_ptr_array_index(tree_stack,tree_stack->len-1);
          g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        } else {    /* assign subtree as current tree */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          //a_simple_expression->value = g_string_free(a_field_component,FALSE);
          a_field_component = g_string_new("");
          t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
          g_print("here\n");
          while(t1->right != LTTV_TREE_IDLE) {
             g_print("while right:%i %p->child:%p\n",t1->right,t1,t1->r_child.t);
             g_assert(t1->right == LTTV_TREE_NODE);
             g_assert(t1->r_child.t != NULL);
             t1 = t1->r_child.t;
          }
          g_print("here2\n");
          t1->right = LTTV_TREE_LEAF;
          t1->r_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
          subtree = g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        }
        break;

      /*	
       *	mathematic operators
       */
      case '<':   /* lower, lower or equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        if(filter->expression[i+1] == '=') { /* <= */
          i++;
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_LE);
        } else lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_LT);
       break;
      
      case '>':   /* higher, higher or equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );   
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        if(filter->expression[i+1] == '=') {  /* >= */
          i++;
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_GE);
        } else lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_GT);
       break;
      
      case '=':   /* equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_EQ);
        break;
        
      /*
       *  Field concatening caracter
       */
      case '.':   /* dot */
        
        /*
         * divide field expression into elements 
         * in a_field_path array.
         */
        /* FIXME: check for double values */
//        if(a_simple_expression->op == NULL) {
          g_ptr_array_add( a_field_path,(gpointer) a_field_component );
          a_field_component = g_string_new("");
//        }
        break;
//    case ' ':
//    case '\n':
//      ignore
      default:    /* concatening current string */
        g_string_append_c(a_field_component,filter->expression[i]);
    }
  }

  g_print("subtree:%p, tree:%p, t1:%p, t2:%p\n",subtree,tree,t1,t2);
  g_print("stack size: %i\n",tree_stack->len);

  /*
   * Preliminary check to see
   * if tree was constructed correctly
   */
  if( p_nesting>0 ) { 
    g_warning("Wrong filtering options, the string\n\"%s\"\n\
        is not valid due to parenthesis incorrect use",filter->expression);	
    return FALSE;
  }
 
  if(tree_stack->len != 1) /* only root tree should remain */ 
    return FALSE;
  
  /*  processing last element of expression   */
  t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
  while(t1->right != LTTV_TREE_IDLE) {
    g_assert(t1->right == LTTV_TREE_NODE);
    t1 = t1->r_child.t;
  }
  if(subtree != NULL) {  /* add the subtree */
    t1->right = LTTV_TREE_NODE;
    t1->r_child.t = subtree;
    subtree = NULL;
  } else {  /* add a leaf */
    lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
    //a_simple_expression->value = g_string_free(a_field_component,FALSE);
    a_field_component = NULL;
    t1->right = LTTV_TREE_LEAF;
    t1->r_child.leaf = a_simple_expression;
    a_simple_expression = NULL;
  }
  
  
  /* free the pointer array */
  g_assert(a_field_path->len == 0);
  g_ptr_array_free(a_field_path,TRUE);

  /* free the tree stack -- but keep the root tree */
 // g_ptr_array_free(tree_stack,FALSE);
  filter->head = g_ptr_array_remove_index(tree_stack,0);
  g_ptr_array_free(tree_stack,TRUE);
  
  /* free the field buffer if allocated */
  if(a_field_component != NULL) g_string_free(a_field_component,TRUE); 
 
  /* free the simple expression buffer if allocated */
  if(a_simple_expression != NULL) lttv_simple_expression_destroy(a_simple_expression);
  
  g_assert(filter->head != NULL); /* tree should exist */
  g_assert(subtree == NULL); /* remaining subtree should be included in main tree */
  
  /* debug */
  g_print("+++++++++++++++ BEGIN PRINT ++++++++++++++++\n");
  lttv_print_tree(filter->head) ;
  g_print("+++++++++++++++ END PRINT ++++++++++++++++++\n");
  
  /* success */
  return TRUE;

}

/**
 *  @fn void lttv_filter_destroy(LttvFilter*)
 * 
 *  Destroy the current LttvFilter
 *  @param filter pointer to the current LttvFilter
 */
void
lttv_filter_destroy(LttvFilter* filter) {
  
  g_free(filter->expression);
  lttv_filter_tree_destroy(filter->head);
  g_free(filter);
  
}

/**
 *  LttvFilterTree* lttv_filter_tree_new()
 * 
 *  Assign a new tree for the current expression
 *  or sub expression
 *  @return pointer of LttvFilterTree
 */
LttvFilterTree* 
lttv_filter_tree_new() {
  LttvFilterTree* tree;

  tree = g_new(LttvFilter,1);
  tree->node = 0; //g_new(lttv_expression,1);
  tree->left = LTTV_TREE_IDLE;
  tree->right = LTTV_TREE_IDLE;
  tree->r_child.t = NULL;
  tree->l_child.t = NULL;
  
  return tree;
}

/**
 *  @fn void lttv_filter_append_expression(LttvFilter*,char*)
 * 
 *  Append a new expression to the expression 
 *  defined in the current filter
 *  @param filter pointer to the current LttvFilter
 *  @param expression string that must be appended
 *  @return Success/Failure of operation
 */
gboolean lttv_filter_append_expression(LttvFilter* filter, char *expression) {

  if(expression == NULL) return FALSE;
  if(filter == NULL) {
    filter = lttv_filter_new(); 
    filter->expression = expression;
  } else if(filter->expression == NULL) {
    filter->expression = expression;
  } else {
    filter->expression = g_strconcat(filter->expression,"&",expression);
  }

  return lttv_filter_update(filter);
  
}

/**
 *  @fn void lttv_filter_clear_expression(LttvFilter*)
 * 
 *  Clear the filter expression from the 
 *  current filter and sets its pointer to NULL
 *  @param filter pointer to the current LttvFilter
 */
void lttv_filter_clear_expression(LttvFilter* filter) {
  
  if(filter->expression != NULL) {
    g_free(filter->expression);
    filter->expression = NULL;
  }
  
}

/**
 *  @fn void lttv_filter_tree_destroy(LttvFilterTree*)
 * 
 *  Destroys the tree and his sub-trees
 *  @param tree Tree which must be destroyed
 */
void 
lttv_filter_tree_destroy(LttvFilterTree* tree) {
 
  if(tree == NULL) return;

  if(tree->left == LTTV_TREE_LEAF) lttv_simple_expression_destroy(tree->l_child.leaf);
  else if(tree->left == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->l_child.t);

  if(tree->right == LTTV_TREE_LEAF) lttv_simple_expression_destroy(tree->r_child.leaf);
  else if(tree->right == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->r_child.t);

  g_free(tree->node);
  g_free(tree);
}

/**
 *  gboolean lttv_filter_tree_parse(LttvFilterTree*,LttEvent,LttTracefile,LttTrace,LttvProcessState)
 * 
 *  Global parsing function for the current
 *  LttvFilterTree
 *  @param tree pointer to the current LttvFilterTree
 *  @param event current LttEvent, NULL if not used
 *  @param tracefile current LttTracefile, NULL if not used
 *  @param trace current LttTrace, NULL if not used
 *  @param state current LttvProcessState, NULL if not used
 */
gboolean
lttv_filter_tree_parse(
        LttvFilterTree* t,
        LttEvent* event,
        LttTracefile* tracefile,
        LttTrace* trace,
        LttvProcessState* state
        /*,...*/) 
{

   /*
   *  Each tree is parsed in inorder.
   *  This way, it's possible to apply the left filter of the 
   *  tree, then decide whether or not the right branch should 
   *  be parsed depending on the linking logical operator
   *
   *  Each node consists in a
   *  1. logical operator
   *  2. left child ( node or simple expression )
   *  3. right child ( node or simple expression )
   *  
   *  When the child is a simple expression, we must 
   *  before all determine if the expression refers to 
   *  a structure which is whithin observation ( not NULL ). 
   *    -If so, the expression is evaluated.
   *    -If not, the result is set to TRUE since this particular 
   *     operation does not interfere with the lttv structure
   *
   *  The result of each simple expression will directly 
   *  affect the next branch.  This way, depending on 
   *  the linking logical operator, the parser will decide 
   *  to explore or not the next branch.
   *  1. AND OPERATOR
   *     -If result of left branch is 0 / FALSE
   *      then don't explore right branch and return 0;
   *     -If result of left branch is 1 / TRUE then explore
   *  2. OR OPERATOR
   *     -If result of left branch is 1 / TRUE
   *      then don't explore right branch and return 1;
   *     -If result of left branch is 0 / FALSE then explore
   *  3. XOR OPERATOR
   *     -Result of left branch will not affect exploration of 
   *      right branch
   */
  g_print("filter::lttv_parse_tree(...)\n");
    
  gboolean lresult = FALSE, rresult = FALSE;
  
  /*
   * Parse left branch
   */
  if(t->left == LTTV_TREE_NODE) lresult = lttv_filter_tree_parse(t->l_child.t,event,tracefile,trace,state);
  else if(t->left == LTTV_TREE_LEAF) {
    //g_print("%p: left is %i %p %s\n",t,t->l_child.leaf->field,t->l_child.leaf->op,t->l_child.leaf->value);
    LttvFieldValue v;
    v = t->l_child.leaf->value;
    switch(t->l_child.leaf->field) {
        
        case LTTV_FILTER_TRACE_NAME:
            if(trace == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)ltt_trace_name(trace),v);
            break;
        case LTTV_FILTER_TRACEFILE_NAME:
            if(tracefile == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)ltt_tracefile_name(tracefile),v);
            break;
        case LTTV_FILTER_STATE_PID:
            if(state == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)&state->pid,v);
            break;
        case LTTV_FILTER_STATE_PPID:
            if(state == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)&state->ppid,v);
            break;
        case LTTV_FILTER_STATE_CT:
            if(state == NULL) lresult = TRUE;
            else {
              double val = ltt_time_to_double(state->creation_time);
              lresult = t->l_child.leaf->op((gpointer)&val,v);
            }
            break;
        case LTTV_FILTER_STATE_IT:
            if(state == NULL) lresult = TRUE;
            else {
              double val = ltt_time_to_double(state->insertion_time);
              lresult = t->l_child.leaf->op((gpointer)&val,v);
            }
            break;
        case LTTV_FILTER_STATE_P_NAME:
            /*
             * FIXME: Yet to be done ( I think ? )
             */
            lresult = TRUE;
            break;
        case LTTV_FILTER_STATE_EX_MODE:
            if(state == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)&state->state->t,v);
            break;
        case LTTV_FILTER_STATE_EX_SUBMODE:
            if(state == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)&state->state->n,v);
            break;
        case LTTV_FILTER_STATE_P_STATUS:
            if(state == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)&state->state->s,v);
            break;
        case LTTV_FILTER_STATE_CPU:
            /*
             * FIXME: What is the comparison value ?
             */
            lresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_NAME:
            if(event == NULL) lresult = TRUE;
            else lresult = t->l_child.leaf->op((gpointer)ltt_event_eventtype(event),v);
            break;
            
        case LTTV_FILTER_EVENT_CATEGORY:
            /*
             * FIXME: Not yet implemented
             */
            lresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_TIME:
//            if(event == NULL) lresult = TRUE;
//            else {
//                double val = ltt_time_to_double(event->event_time);
//                lresult = t->l_child.leaf->op((gpointer)&val,v);
//            }
            lresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_TSC:
//          if(event == NULL) lresult = TRUE;
//          else {
//            double val = ltt_time_to_double(event->event_time);
//            lresult = t->l_child.leaf->op((gpointer)&val,v);
//          }
            /*
             * FIXME: Where is event.tsc
             */
            lresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_FIELD:
            /*
             * TODO: Use the offset to 
             * find the dynamic field 
             * in the event struct
             */
            lresult = TRUE; 
        default:
            /*
             * This case should never be 
             * parsed, if so, the whole 
             * filtering is cancelled
             */
            g_warning("Error while parsing the filter tree");
            return TRUE;
    }
  }
 
  /*
   * Parse linking operator
   * make a cutoff if possible
   */
  if((t->node & LTTV_LOGICAL_OR) && lresult == TRUE) return TRUE;
  if((t->node & LTTV_LOGICAL_AND) && lresult == FALSE) return FALSE;

  /*
   * Parse right branch
   */
  if(t->right == LTTV_TREE_NODE) rresult = lttv_filter_tree_parse(t->r_child.t,event,tracefile,trace,state);
  else if(t->right == LTTV_TREE_LEAF) {
    //g_print("%p: right is %i %p %s\n",t,t->r_child.leaf->field,t->r_child.leaf->op,t->r_child.leaf->value);
    LttvFieldValue v;
    v = t->r_child.leaf->value;
    switch(t->r_child.leaf->field) {
        
        case LTTV_FILTER_TRACE_NAME:
            if(trace == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)ltt_trace_name(trace),v);
            break;
        case LTTV_FILTER_TRACEFILE_NAME:
            if(tracefile == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)ltt_tracefile_name(tracefile),v);
            break;
        case LTTV_FILTER_STATE_PID:
            if(state == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)&state->pid,v);
            break;
        case LTTV_FILTER_STATE_PPID:
            if(state == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)&state->ppid,v);
            break;
        case LTTV_FILTER_STATE_CT:
            if(state == NULL) rresult = TRUE;
            else {
              double val = ltt_time_to_double(state->creation_time);
              rresult = t->r_child.leaf->op((gpointer)&val,v);
            }
            break;
        case LTTV_FILTER_STATE_IT:
            if(state == NULL) rresult = TRUE;
            else {
              double val = ltt_time_to_double(state->insertion_time);
              rresult = t->r_child.leaf->op((gpointer)&val,v);
            }
            break;
        case LTTV_FILTER_STATE_P_NAME:
            /*
             * FIXME: Yet to be done ( I think ? )
             */
            rresult = TRUE;
            break;
        case LTTV_FILTER_STATE_EX_MODE:
            if(state == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)&state->state->t,v);
            break;
        case LTTV_FILTER_STATE_EX_SUBMODE:
            if(state == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)&state->state->n,v);
            break;
        case LTTV_FILTER_STATE_P_STATUS:
            if(state == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)&state->state->s,v);
            break;
        case LTTV_FILTER_STATE_CPU:
            /*
             * FIXME: What is the comparison value ?
             */
            rresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_NAME:
            if(event == NULL) rresult = TRUE;
            else rresult = t->r_child.leaf->op((gpointer)ltt_event_eventtype(event),v);
            break;
            
        case LTTV_FILTER_EVENT_CATEGORY:
            /*
             * FIXME: Not yet implemented
             */
            rresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_TIME:
//            if(event == NULL) rresult = TRUE;
//            else {
//                double val = ltt_time_to_double(event->event_time);
//                rresult = t->r_child.leaf->op((gpointer)&val,v);
//            }
            rresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_TSC:
//          if(event == NULL) rresult = TRUE;
//          else {
//            double val = ltt_time_to_double(event->event_time);
//            rresult = t->r_child.leaf->op((gpointer)&val,v);
//          }
            /*
             * FIXME: Where is event.tsc
             */
            rresult = TRUE;
            break;
        case LTTV_FILTER_EVENT_FIELD:
            /*
             * TODO: Use the offset to 
             * find the dynamic field 
             * in the event struct
             */
            rresult = TRUE; 
        default:
            /*
             * This case should never be 
             * parsed, if so, this subtree
             * is cancelled !
             */
            g_warning("Error while parsing the filter tree");
            return TRUE;
    }
  }
   
  /*
   * Apply and return the 
   * logical link between the 
   * two operation
   */
  switch(t->node) {
    case LTTV_LOGICAL_OR: return (lresult | rresult);
    case LTTV_LOGICAL_AND: return (lresult & rresult);
    case LTTV_LOGICAL_NOT: return (!rresult);
    case LTTV_LOGICAL_XOR: return (lresult ^ rresult);
    default: 
      /*
       * This case should never be 
       * parsed, if so, this subtree
       * is cancelled !
       */
      return TRUE;
  }
  
}

/**
 *  @fn void lttv_print_tree(LttvFilterTree*)
 *
 *  Debug
 *  @param t the pointer to the current LttvFilterTree
 */
void
lttv_print_tree(LttvFilterTree* t) {

  g_print("node:%p lchild:%p rchild:%p\n",t, //t->l_child.t,t->r_child.t);
          (t->left==LTTV_TREE_NODE)?t->l_child.t:NULL,
          (t->right==LTTV_TREE_NODE)?t->r_child.t:NULL);
  g_print("node type: %i / [left] %i / [right] %i\n",t->node,t->left,t->right);
  if(t->left == LTTV_TREE_NODE) lttv_print_tree(t->l_child.t);
  else if(t->left == LTTV_TREE_LEAF) {
//    g_assert(t->l_child.leaf->value != NULL);
    g_print("%p: left is %i %p value\n",t,t->l_child.leaf->field,t->l_child.leaf->op);
  }
  if(t->right == LTTV_TREE_NODE) lttv_print_tree(t->r_child.t);
  else if(t->right == LTTV_TREE_LEAF) {
//    g_assert(t->r_child.leaf->value != NULL);
    g_print("%p: right is %i %p value\n",t,t->r_child.leaf->field,t->r_child.leaf->op);
  }
 
}

/**
 *  gboolean lttv_filter_tracefile(LttvFilter*, LttTracefile*)
 * 
 * 	Apply the filter to a specific trace
 * 	@param filter the current filter applied
 * 	@param tracefile the trace to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_tracefile(LttvFilter *filter, LttTracefile *tracefile) {

  return lttv_filter_tree_parse(filter->head,NULL,tracefile,NULL,NULL);
  
}

/**
 *  @fn gboolean lttv_filter_tracestate(LttvFilter*,LttvTraceState*)
 *  
 *  Parse the current tracestate
 *  @param filter pointer to the current LttvFilter
 *  @param tracestate pointer to the current tracestate
 */
gboolean
lttv_filter_tracestate(LttvFilter *filter, LttvTraceState *tracestate) {

}

/**
 *  @fn gboolean lttv_filter_event(LttvFilter*,LttEvent*)
 * 
 * 	Apply the filter to a specific event
 * 	@param filter the current filter applied
 * 	@param event the event to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_event(LttvFilter *filter, LttEvent *event) {

}

/**
 *  @fn static void module_init()
 * 
 *  Initializes the filter module and specific values
 */
static void module_init()
{

}

/**
 *  Destroys the filter module and specific values
 */
static void module_destroy() 
{

}


LTTV_MODULE("filter", "Filters traceset and events", \
    "Filters traceset and events specifically to user input", \
    module_init, module_destroy)



