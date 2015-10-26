/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim: set expandtab shiftwidth=4 softtabstop=4 : */
/*
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>

#include <time.h>
#include <llib/lmacros.h>
#include <llib/lcache.h>
#include <llib/lmemory.h>

static char* key[] = { "key1", "key2", "key3", "key4" };
static int vals[] = { 1, 2, 3, 4 };
static int tcount = 0;

#define ret_fail_unless(expr, msg)                                      \
    tcount++;                                                           \
    if (!(expr))                                                        \
    {                                                                   \
        fprintf (stderr, "*** test %d *** %s ***\n", tcount, (msg));    \
        return -tcount;                                                 \
    }

LCache * cache = NULL;

/* Obtain a backtrace and print it to stdout. */
static void
print_stack_trace (void)
{
  void *array[20];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 20);
  strings = backtrace_symbols (array, size);

  fprintf(stderr, "-- Start of crash callstack --\n");
  for (i = 0; i < size; i++)
     fprintf(stderr, "    %s\n", strings[i]);

  fprintf(stderr, "-- End of crash callstack --\n\n");
  free (strings);
}

static void
abnormal_termination_handler (int signo)
{
    fprintf(stderr, "\n\n*** LCACHE[%d] terminated by signal %d: %s\n",
            getpid (), signo, strsignal (signo));

    // print crash callstack
    if (signo == SIGSEGV) {
        print_stack_trace ();
    }

    l_cache_dump(&cache);
    l_cache_destroy(&cache);

    _exit (128 + signo);
}

static void
init_signal_handlers (void)
{
    struct sigaction sa;

    /* Setup the signal handler function */
    sa.sa_handler = &abnormal_termination_handler;

    /* Restart the system call, if at all possible */
    sa.sa_flags = SA_RESTART;

    /* Block every signal during execution of the handler */
    sigfillset(&sa.sa_mask);

    /* Intercept the following signals */
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGSTKFLT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

static lpointer
create_item (lconstpointer k)
{
    int i;
    for (i = 0; i < L_N_ELEMENTS (key); i++) {
        if (!strcmp(k, key[i])) {
            // force crash
            assert(false);
        }
    }
    return (lpointer)k;
}

/*
void
__attribute__ ((constructor))
trace_begin (void)
{
 fp_trace = fopen("trace.out", "w");
}

void
__attribute__ ((destructor))
trace_end (void)
{
 if(fp_trace != NULL) {
 fclose(fp_trace);
 }
}
*/

void
__cyg_profile_func_enter (void *func,  void *caller)
{
 //if(fp_trace != NULL) {
 fprintf(stdout, "E %p %p\n", func, caller );
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

int
main (int argc, char * argv[])
{
    int i;
    lpointer val;
    L_UNUSED_VAR (argc);
    L_UNUSED_VAR (argv);
    init_signal_handlers();

    // creates new object cache with 2 secs item expiration and
    // 5 seconds interval between object cleanups.
    l_cache_new(&cache, 1, 5);
    for (i = 0; i < L_N_ELEMENTS (vals); i++) {
        char* k = key[i];
        l_cache_put(&cache, k, L_INT_TO_PTR (vals[i]));
        val = l_cache_get(&cache, k);
        ret_fail_unless (vals[i] == L_PTR_TO_INT (val),
                                 "l_cache_put failed");
    }

    val = l_cache_get_or_put(&cache, key[0], create_item);
    ret_fail_unless (vals[0] == L_PTR_TO_INT (val), "l_cache_get_or_create failed");
    l_cache_get_or_put(&cache, "testVal", create_item);
    while ( (i = l_cache_get_length(&cache)) > 0 ) {
        l_cache_dump(&cache);
        sleep(2);
    }
    l_cache_destroy(&cache);
    return 0;
}
