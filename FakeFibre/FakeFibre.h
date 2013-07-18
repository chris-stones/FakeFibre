
/*
 * An ugly, ugly ugly implementation of fibres using pthreads.
 * Im after windows SwitchToFibre functioality, but existing
 * libraries dont quite fulfill my need because...
 *
 * ucontext - deprecated and not mandatory ( not implemented on Android ).
 * libtask - cant yield to a specific task.
 * libconcurrency - would be perfect.... except it reallocates / moves stacks... cant take the address of stack variables! I need this. )
 **/


#ifdef __cplusplus
extern "C" {
#endif

struct _fake_fibre;
typedef struct _fake_fibre * ff_handle;

typedef void* (*ff_function)(void*);

int ff_convert_this(ff_handle * f);
int ff_create(ff_handle * f, ff_function start_routine, void * data, ff_handle exit_to);
int ff_yield_to(ff_handle f);
int ff_yield();
int ff_set_exit_to(ff_handle f);
int ff_exit();

#ifdef __cplusplus
} // extern "C" {
#endif


