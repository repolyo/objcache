/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim: set expandtab shiftwidth=4 softtabstop=4 : */
/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>

#include <assert.h>
#include <llib/lmacros.h>
#include <llib/lstack.h>
#include <llib/lslist.h>
#include <llib/lmemory.h>


#include <string.h>
#include <dlfcn.h>

static int vals[] = { 1, 2, 3, 4 };
static int csums[] = { 1, 3, 6, 10 };
static int tcount = 0;
static FILE *fp_trace;

#define ret_fail_unless(expr, msg)                                      \
    tcount++;                                                           \
    if (!(expr))                                                        \
    {                                                                   \
        fprintf (stderr, "*** test %d *** %s ***\n", tcount, (msg));    \
        return -tcount;                                                 \
    }

static void
print_stack_trace (FILE * out)
{
  void *array[20];
  size_t size;
  char **strings;

  size = backtrace (array, 20);
  strings = backtrace_symbols (array, size);

  fprintf(out, "    %s\n", strings[1]);

  free (strings);
}

void
__attribute__ ((constructor))
trace_begin (void)
{
    fprintf(stdout, "----------------- %s: called....\n", __func__);
    fp_trace = fopen("trace.out", "w");
}

void
__attribute__ ((destructor))
trace_end (void)
{
    fprintf(stdout, "----------------- %s: called....\n", __func__);
    if(fp_trace != NULL) {
        fclose(fp_trace);
     }
}

void
__cyg_profile_func_enter (void *func,  void *caller)
{
 //if(fp_trace != NULL) {
    fprintf(fp_trace, "e %p %p\n", func, caller );
    print_stack_trace(fp_trace);
 //}
}

void
__cyg_profile_func_exit (void *func, void *caller)
{
    L_UNUSED_VAR(func);
    L_UNUSED_VAR(caller);
 //if(fp_trace != NULL) {
 //fprintf(stdout, "x %p %p %lu\n", func, caller, time(NULL));
 //}
}

static bool
summation (lpointer value, lpointer data)
{
    int val = L_PTR_TO_INT (value);
    *((int*)data) += val;
    return false;
}

static int
checksum_stack (LStack ** stack)
{
    int      csum = 0;
    l_stack_foreach(stack, summation, &csum);
    return csum;
}

static size_t free_func_times = 0;

static void
free_func (lpointer data)
{
    L_UNUSED_VAR (data);
    free_func_times++;
}

unsigned int
test_l_stack_free (LStack ** stack)
{
    int i;
    while ( !l_stack_is_empty(stack) ) {
        l_stack_pop(stack);
    }
    for (i = 0; i < L_N_ELEMENTS (vals); i++) {
        l_stack_push(stack, L_INT_TO_PTR (vals[i]));
    }
    l_stack_free( stack, free_func);
    assert( L_N_ELEMENTS (vals) == free_func_times );
    assert( NULL == *stack );
    return free_func_times;
}

int
test_l_stack_push (LStack ** stack)
{
    int      i;

    for (i = 0; i < L_N_ELEMENTS (vals); i++)
    {
        l_stack_push(stack, L_INT_TO_PTR (vals[i]));

        ret_fail_unless (checksum_stack (stack) == csums[i],
                         "l_stack_push failed");
    }
    assert( L_N_ELEMENTS( vals ) == l_stack_get_depth( stack ) );
    return i;
}


int
test_l_stack_pop (LStack ** stack)
{
    int      i;
    int      val;

    // remove remaining items from previous tests
    // to start with an empty stack.
    while ( !l_stack_is_empty(stack) ) {
        l_stack_pop(stack);
    }

    for (i = 0; i < L_N_ELEMENTS (vals); i++) {
        l_stack_push(stack, L_INT_TO_PTR (vals[i]));
    }

    while ( !l_stack_is_empty(stack) ) {
        val = L_PTR_TO_INT (l_stack_pop(stack));
        ret_fail_unless (val == vals[--i],
                                 "l_stack_pop failed");
    }
    assert( 0 == l_stack_get_depth( stack ) );
    return 0;
}

int
main (int argc, char * argv[])
{
    LStack * stack = NULL;

    L_UNUSED_VAR (argc);
    L_UNUSED_VAR (argv);
    test_l_stack_push(&stack);
    test_l_stack_pop(&stack);
    test_l_stack_free(&stack);

    return 0;
}
