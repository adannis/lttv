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

LttvSimpleExpression* 
lttv_simple_expression_new() {

}
/**
 * add a node to the current tree
 * @param stack the tree stack
 * @param subtree the subtree if available (pointer or NULL)
 * @param op the logical operator that will form the node
 */
void
lttv_filter_tree_add_node(GPtrArray* stack, LttvFilterTree* subtree, LttvLogicalOp op) {

  LttvFilterTree* t1 = NULL;
  LttvFilterTree* t2 = NULL;

  t1 = (LttvFilterTree*)g_ptr_array_index(stack,stack->len-1);
  while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
  t2 = lttv_filter_tree_new();
  t2->node = op;
  if(subtree != NULL) {
    t2->left = LTTV_TREE_NODE;
    t2->l_child.t = subtree;
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
 *  Parse through filtering field hierarchy as specified 
 *  by user.  This function compares each value to 
 *  predetermined quarks
 *  @param fp The field path list
 *  @return success/failure of operation
 */
gboolean
parse_field_path(GPtrArray* fp, LttvSimpleExpression* se) {

  GString* f = NULL;
  if(fp->len < 2) return FALSE;
  g_assert(f=g_ptr_array_index(fp,0)); //list_first(fp)->data; 
 
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
  if(g_strcasecmp(f->str,"trace") ) {
    /*
     * Possible values:
     *  trace.name
     */
    f=g_ptr_array_index(fp,1);
    if(g_strcasecmp(f->str,"name")) {}
    else return FALSE;
  } else if(g_strcasecmp(f->str,"traceset") ) {
    /* 
     * FIXME: not yet implemented !
     */
  } else if(g_strcasecmp(f->str,"tracefile") ) {
    /*
     * Possible values:
     *  tracefile.name
     */
    f=g_ptr_array_index(fp,1);
    if(g_strcasecmp(f->str,"name")) {}
    else return FALSE;
  } else if(g_strcasecmp(f->str,"state") ) {
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
    f=g_ptr_array_index(fp,1);
    if(g_strcasecmp(f->str,"pid") ) {}
    else if(g_strcasecmp(f->str,"ppid") ) {}
    else if(g_strcasecmp(f->str,"creation_time") ) {}
    else if(g_strcasecmp(f->str,"insertion_time") ) {}
    else if(g_strcasecmp(f->str,"process_name") ) {}
    else if(g_strcasecmp(f->str,"execution_mode") ) {}
    else if(g_strcasecmp(f->str,"execution_submode") ) {}
    else if(g_strcasecmp(f->str,"process_status") ) {}
    else if(g_strcasecmp(f->str,"cpu") ) {}
    else return FALSE;
  } else if(g_strcasecmp(f->str,"event") ) {
    f=g_ptr_array_index(fp,1);
    if(g_strcasecmp(f->str,"name") ) {}
    else if(g_strcasecmp(f->str,"category") ) {}
    else if(g_strcasecmp(f->str,"time") ) {
      // offset = &((LttEvent*)NULL)->event_time);
    }
    else if(g_strcasecmp(f->str,"tsc") ) {
      // offset = &((LttEvent*)NULL)->event_cycle_count);
    }
    else {  /* core.xml specified options */

    }
  } else {
    g_warning("Unrecognized field in filter string");
    return FALSE;
  }

  return TRUE;  
}

/**
 * 	Add an filtering option to the current tree
 * 	@param expression Current expression to parse
 * 	@return success/failure of operation
 */
gboolean
parse_simple_expression(GString* expression) {
	
	unsigned i;

  
  

}

/**
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_double(double v1, double v2) {}

/**
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_string(char* v1, char* v2) {}

/**
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_double(double v1, double v2) {}

/**
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_string(char* v1, char* v2) {}

/**
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_double(double v1, double v2) {}

/**
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_double(double v1, double v2) {}

/**
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_double(double v1, double v2) {}

/**
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint64(guint64 v1, guint64 v2) {}

/**
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint32(guint32 v1, guint32 v2) {}

/**
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint16(guint16 v1, guint16 v2) {}

/**
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_double(double v1, double v2) {}


/**
 * Makes a copy of the current filter tree
 * @param tree pointer to the current tree
 * @return new copy of the filter tree
 */
LttvFilterTree*
lttv_filter_tree_clone(LttvFilterTree* tree) {

  

}

/**
 * Makes a copy of the current filter
 * @param filter pointer to the current filter
 * @return new copy of the filter
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
 * 	Creates a new lttv_filter
 * 	@param expression filtering options string
 * 	@param t pointer to the current LttvTrace
 * 	@return the current lttv_filter or NULL if error
 */
LttvFilter*
lttv_filter_new(char *expression, LttvTraceState *tcs) {

  g_print("filter::lttv_filter_new()\n");		/* debug */

  unsigned 	
    i, 
    p_nesting=0,	/* parenthesis nesting value */
    b=0;	/* current breakpoint in expression string */

  /* trees */
  LttvFilterTree
    *tree = lttv_filter_tree_new(),   /* main tree */
    *subtree = NULL,                  /* buffer for subtrees */
    *t1,                              /* buffer #1 */
    *t2;                              /* buffer #2 */

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
  GPtrArray *a_field_path = NULL;
    
  LttvSimpleExpression* a_simple_expression = g_new(LttvSimpleExpression,1);
  
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
  
  a_field_path = g_ptr_array_new();
  g_ptr_array_set_size(a_field_path,2);   /* by default, recording 2 field expressions */

  
  for(i=0;i<strlen(expression);i++) {
//    g_print("%s\n",a_field_component->str);
    g_print("%c ",expression[i]);
//    g_print("switch:%c -->subtree:%p\n",expression[i],subtree);
    switch(expression[i]) {
      /*
       *   logical operators
       */
      case '&':   /* and */
        t1 = (LttvFilterTree*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_AND;
        if(subtree != NULL) {
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {
          a_simple_expression->value = a_field_component->str;
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = g_new(LttvSimpleExpression,1);
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2; 
        }
        
        break;
      case '|':   /* or */
        t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_OR;
        if(subtree != NULL) { 
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {
          a_simple_expression->value = a_field_component->str;
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = g_new(LttvSimpleExpression,1);
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2; 
        }
        break;
      case '^':   /* xor */
        t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_XOR;
        if(subtree != NULL) { 
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2;
        } else {
          a_simple_expression->value = a_field_component->str;
          a_field_component = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = g_new(LttvSimpleExpression,1);
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t2; 
        }
        break;
      case '!':   /* not, or not equal (math op) */
        if(expression[i+1] == '=') {  /* != */
          a_simple_expression->op = LTTV_FIELD_NE;
          i++;
          g_ptr_array_add( a_field_path,(gpointer) a_field_component );
          a_field_component = g_string_new("");         
        } else {  /* ! */
        //  g_print("%s\n",a_field_component);
        //  a_field_component = g_string_new("");
          t1 = (LttvFilter*)g_ptr_array_index(tree_stack,tree_stack->len-1);
          while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
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
                     is not valid due to parenthesis incorrect use",expression);	
          return NULL;
        }
        
        g_assert(tree_stack->len>0);
        if(subtree != NULL) { 
          t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
          while(t1->right != LTTV_TREE_IDLE && t1->right != LTTV_TREE_LEAF) {
              g_assert(t1!=NULL && t1->r_child.t != NULL);
              t1 = t1->r_child.t;
          }
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = subtree;
          subtree = g_ptr_array_index(tree_stack,tree_stack->len-1);
          g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        } else {
          a_simple_expression->value = a_field_component->str;
          a_field_component = g_string_new("");
          t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
          while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
          t1->right = LTTV_TREE_LEAF;
          t1->r_child.leaf = a_simple_expression;
          a_simple_expression = g_new(LttvSimpleExpression,1);
          subtree = g_ptr_array_index(tree_stack,tree_stack->len-1);
          g_assert(subtree != NULL);
          g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        }
        break;

      /*	
       *	mathematic operators
       */
      case '<':   /* lower, lower or equal */
        if(expression[i+1] == '=') { /* <= */
          i++;
          a_simple_expression->op = LTTV_FIELD_LE; 
        } else a_simple_expression->op = LTTV_FIELD_LT;
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        a_field_component = g_string_new("");         
        break;
      case '>':   /* higher, higher or equal */
        if(expression[i+1] == '=') {  /* >= */
          i++;
          a_simple_expression->op = LTTV_FIELD_GE;        
        } else a_simple_expression->op = LTTV_FIELD_GT;
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        a_field_component = g_string_new("");         
        break;
      case '=':   /* equal */
        a_simple_expression->op = LTTV_FIELD_EQ;
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        a_field_component = g_string_new("");         
        break;
      /*
       *  Field concatening caracter
       */
      case '.':   /* dot */
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        a_field_component = g_string_new("");
        break;
      default:    /* concatening current string */
        g_string_append_c(a_field_component,expression[i]); 				
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
        is not valid due to parenthesis incorrect use",expression);	
    return NULL;
  }
 
  if(tree_stack->len != 1) /* only root tree should remain */ 
    return NULL;
  
  /*  processing last element of expression   */
  t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
  while(t1->right != LTTV_TREE_IDLE) t1 = t1->r_child.t;
  if(subtree != NULL) {  /* add the subtree */
    t1->right = LTTV_TREE_NODE;
    t1->r_child.t = subtree;
    subtree = NULL;
  } else {  /* add a leaf */
    a_simple_expression->value = a_field_component->str;
    a_field_component = g_string_new("");
    t1->right = LTTV_TREE_LEAF;
    t1->r_child.leaf = a_simple_expression;
    /*
     * FIXME: is it really necessary to reallocate 
     *        LttvSimpleExpression at this point ??
     */
    a_simple_expression = g_new(LttvSimpleExpression,1);
  }
  
  g_assert(tree != NULL);
  g_assert(subtree == NULL); 
  
  lttv_filter_tracefile(tree,NULL); 
  
  return tree;
  
}

void
lttv_filter_destroy(LttvFilter* filter) {

}

/**
 *  Assign a new tree for the current expression
 *  or sub expression
 *  @return pointer of LttvFilterTree
 */
LttvFilterTree* lttv_filter_tree_new() {
  LttvFilterTree* tree;

  tree = g_new(LttvFilter,1);
  tree->node = 0; //g_new(lttv_expression,1);
//  tree->node->type = LTTV_UNDEFINED_EXPRESSION;
  tree->left = LTTV_TREE_IDLE;
  tree->right = LTTV_TREE_IDLE;

  return tree;
}

/**
 *  Destroys the tree and his sub-trees
 *  @param tree Tree which must be destroyed
 */
void lttv_filter_tree_destroy(LttvFilterTree* tree) {
  
  if(tree == NULL) return;

  if(tree->left == LTTV_TREE_LEAF) g_free(tree->l_child.leaf);
  else if(tree->left == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->l_child.t);

  if(tree->right == LTTV_TREE_LEAF) g_free(tree->r_child.leaf);
  else if(tree->right == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->r_child.t);

  g_free(tree->node);
  g_free(tree);
}


/**
 * 	Apply the filter to a specific trace
 * 	@param filter the current filter applied
 * 	@param tracefile the trace to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_tracefile(LttvFilter *filter, LttTracefile *tracefile) {

  LttvFilterTree* t = filter->head;
    
  /*
   *  Each tree is parsed in inorder.
   *  This way, it's possible to apply the left filter of the 
   *  tree, then decide whether or not the right branch should 
   *  be parsed depending on the linking logical operator
   *
   *  As for the filtering structure, since we are trying 
   *  to remove elements from the trace, it might be better 
   *  managing an array of all items to be removed .. 
   */
  
  g_print("node:%p lchild:%p rchild:%p\n",t,t->l_child.t,t->r_child.t);
  g_print("node type%i\n",t->node);
  if(t->left == LTTV_TREE_NODE) lttv_filter_tracefile(t->l_child.t,NULL);
  else if(t->left == LTTV_TREE_LEAF) {
    g_assert(t->l_child.leaf->value != NULL);
    g_print("%p: left is qqch %i %s\n",t,t->l_child.leaf->op,t->l_child.leaf->value);
  }
  if(t->right == LTTV_TREE_NODE) lttv_filter_tracefile(t->r_child.t,NULL);
  else if(t->right == LTTV_TREE_LEAF) {
    g_assert(t->r_child.leaf->value != NULL);
    g_print("%p: right is qqch %i %s\n",t,t->r_child.leaf->op,t->r_child.leaf->value);
  }
  
  /* test */
/*  int i, nb;
  char *f_name, *e_name;

  char* field = "cpu";
   
  LttvTraceHook h;

  LttEventType *et;

  LttType *t;

  GString *fe_name = g_string_new("");

  nb = ltt_trace_eventtype_number(tcs->parent.t);
  g_print("NB:%i\n",nb);
  for(i = 0 ; i < nb ; i++) {
    et = ltt_trace_eventtype_get(tcs->parent.t, i);
    e_name = ltt_eventtype_name(et);
    f_name = ltt_facility_name(ltt_eventtype_facility(et));
    g_string_printf(fe_name, "%s.%s", f_name, e_name);
    g_print("facility:%s and event:%s\n",f_name,e_name);
  }
 */
}

gboolean
lttv_filter_tracestate(LttvFilter *filter, LttvTraceState *tracestate) {

}

/**
 * 	Apply the filter to a specific event
 * 	@param filter the current filter applied
 * 	@param event the event to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_event(LttvFilter *filter, LttEvent *event) {

}

/**
 *  Initializes the filter module and specific values
 */
static void module_init()
{

  /* 
   *  Quarks initialization
   *  for hardcoded filtering options
   *
   *  TODO: traceset has no yet been defined
   */
  
  /* top fields */
//  LTTV_FILTER_EVENT = g_quark_from_string("event"); 
//  LTTV_FILTER_TRACE = g_quark_from_string("trace"); 
//  LTTV_FILTER_TRACESET = g_quark_from_string("traceset"); 
//  LTTV_FILTER_STATE = g_quark_from_string("state"); 
//  LTTV_FILTER_TRACEFILE = g_quark_from_string("tracefile"); 
 
  /* event.name, tracefile.name, trace.name */
//  LTTV_FILTER_NAME = g_quark_from_string("name");

  /* event sub fields */
//  LTTV_FILTER_CATEGORY = g_quark_from_string("category"); 
//  LTTV_FILTER_TIME = g_quark_from_string("time"); 
//  LTTV_FILTER_TSC = g_quark_from_string("tsc"); 

  /* state sub fields */
//  LTTV_FILTER_PID = g_quark_from_string("pid"); 
//  LTTV_FILTER_PPID = g_quark_from_string("ppid"); 
//  LTTV_FILTER_C_TIME = g_quark_from_string("creation_time");  
//  LTTV_FILTER_I_TIME = g_quark_from_string("insertion_time"); 
//  LTTV_FILTER_P_NAME = g_quark_from_string("process_name"); 
//  LTTV_FILTER_EX_MODE = g_quark_from_string("execution_mode");  
//  LTTV_FILTER_EX_SUBMODE = g_quark_from_string("execution_submode");
//  LTTV_FILTER_P_STATUS = g_quark_from_string("process_status");
//  LTTV_FILTER_CPU = g_quark_from_string("cpu");
  
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



