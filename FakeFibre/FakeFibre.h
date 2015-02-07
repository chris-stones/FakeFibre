
/*
 * An ugly, ugly ugly implementation of fibres using pthreads.
 * I'm after windows SwitchToFibre functionality, but existing
 * libraries don't quite fulfil my need because...
 *
 * ucontext - deprecated and not mandatory ( not implemented on Android ).
 * libtask - can't yield to a specific task.
 * libconcurrency - would be perfect.... except it reallocates / moves stacks... can't take the address of stack variables! I need this. )
 **/


#ifdef __cplusplus
extern "C" {
#endif

struct _fake_fibre;
typedef struct _fake_fibre * ff_handle;

typedef void* (*ff_function)(void*);

// Convert a thread to a fibre.
// Main thread, or threads manually created with pthread_create MUST call ff_convert_this
// before using any other ff_* functions.
// When you manually create a new thread, and convert it to a fibre, you create multiple master threads.
// DO NOT MIX FIBRES FROM DIFFERENT MASTER THREADS!
int ff_convert_this(ff_handle * f);

// Create a fibre to share the the calling master thread with.
int ff_create(ff_handle * f, ff_function start_routine, void * data, ff_handle exit_to);

// Yield to a specific fibre.
//  Passing NULL will yield to ANY other fibres in the callers master thread.
int ff_yield_to(ff_handle f);

// Same as ff_yield(NULL)
int ff_yield();

// When calling fibre exits naturally, by returning from its start_routine or calling ff_exit()
//	yield to the given fibre.
//  IF the given fibre is no-longer running when falling fibre exits normally,
//  act as-if set_exit_to had not been called. ( Exit to any other fibre in the master thread ).
int ff_set_exit_to(ff_handle f);

// Instantly kill given fibre.
//  calling fibre retains control. Any set_exit_to is ignored.
int ff_kill(ff_handle f);

// Wait for given fibre to exit naturally.
int ff_wait(ff_handle f);

// Exit calling fibre. If a set_exit_to was set, that fibre is next to run.
//	otherwise, any other fibre in this master thread is run.
int ff_exit();

// Test if given thread is running.
// returns '1' is running, '0' is not, '-1' on error.
int ff_is_running(ff_handle f);

// Free resources held he a fibre/
// fibre MUST have exited, or been killed.
// after falling free, 'f' is considered a dangling pointer. don't use it. NULL it out!
// be aware that you MAY have set this fibre as an set_exit_to().
int ff_free(ff_handle f);

#ifdef __cplusplus
} // extern "C" {
#endif


