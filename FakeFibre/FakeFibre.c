
/*
 * An ugly, ugly ugly implementation of fibres using pthreads.
 * Im after windows SwitchToFibre functioality, but existing
 * libraries dont quite fulfill my need because...
 *
 * ucontext - deprecated and not mandatory ( not implemented on Android ).
 * libtask - cant yield to a specific task.
 * libconcurrency - would be perfect.... except it reallocates / moves stacks... cant take the address of stack variables! I need this. )
 **/

/*
 * TODO: use a per-fibre condition variable instead of one global for performance...
 * 	will need to re-think the yield-to-any / exit to any functionality
 */

#include "FakeFibre.h"

#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>

/*** TLS ***/
static pthread_key_t master_key;
static pthread_once_t master_key_once = PTHREAD_ONCE_INIT;
static void make_master_key() {
    pthread_key_create(&master_key, NULL);
}
/***********/

struct master_thread_struct;
typedef struct master_thread_struct master_thread_t;

struct _fake_fibre {

	master_thread_t * master;

	pthread_t thread;

	ff_function start_routine;
	void *data;

	ff_handle exit_to;

	int kill_flag;
};

struct master_thread_struct {

	int ref;
	ff_handle running;
	ff_handle next;

	pthread_mutex_t mutex;
	pthread_cond_t  condition;
};

static int _create_master_struct(master_thread_t ** master) {

	master_thread_t * m = NULL;

	pthread_once(&master_key_once, make_master_key);

	m = calloc(1, sizeof(struct master_thread_struct) );

	m->ref = 1;
	pthread_mutex_init(&m->mutex , NULL);
	pthread_cond_init(&m->condition, NULL);

	pthread_setspecific(master_key, m);

	if(master)
		*master = m;
	return 0;
}

static int _free_master_struct(master_thread_t * master) {

	if(!master)
		return 0;

	master->ref--;
	if(master->ref>0)
		return 0;

	pthread_setspecific(master_key, NULL);

	pthread_cond_destroy(&master->condition);
	pthread_mutex_destroy(&master->mutex);
	free(master);
	return 0;
}

static int _create_ff_struct(ff_handle * f) {

	ff_handle h = calloc(1, sizeof(struct _fake_fibre));
	if(!h)
		return -1;

	*f = h;
	return 0;
}

static void _wait_for_runnable(ff_handle h) {

	for(;;) {

		pthread_cond_wait( &h->master->condition, &h->master->mutex );

		if(!h->master->next || ( h->master->next == h) )
			break;
	}

	h->master->running = h;
	h->master->next = h;

	if(h->kill_flag)
		ff_exit();
		
}

static void * _new_fake_fibre(void* data) {

	void * ret = NULL;

	ff_handle h = (ff_handle)data;

	pthread_detach( pthread_self() );

	pthread_setspecific(master_key, h->master);

	pthread_mutex_lock(&h->master->mutex);

	pthread_cond_broadcast(&h->master->condition);

	_wait_for_runnable(h);

	ret = (*h->start_routine)(h->data);

	ff_exit();

	return ret;
}

int ff_convert_this(ff_handle * f) {

	master_thread_t * m = NULL;

	pthread_once(&master_key_once, make_master_key);

	m = (master_thread_t *)pthread_getspecific(master_key);

	if(m) {
		*f = m->running;
		return 0;
	}

	if( _create_ff_struct(f) == 0) {

		(*f)->thread = pthread_self();

		_create_master_struct(&(*f)->master);

		(*f)->master->running = *f;
		(*f)->master->next = *f;

		pthread_mutex_lock(&(*f)->master->mutex);

		return 0;
	}
	return -1;
}

int ff_create(ff_handle * _f, ff_function start_routine, void * data, ff_handle exit_to) {

	if( _create_ff_struct(_f) == 0) {

		ff_handle f = *_f;

		f->start_routine = start_routine;
		f->data = data;
		f->exit_to = exit_to;
		f->master = (master_thread_t*)pthread_getspecific(master_key);
		f->master->ref++;

		pthread_create( &f->thread, NULL, &_new_fake_fibre, f );

		// WAIT for spawned fabe fibre to sleep in _wait_for_runnable() before returning.
		//  Otherwise, a subsequent yield will broadcast without any wakable fibres.
		pthread_cond_wait( &f->master->condition, &f->master->mutex );

		return 0;
	}

	return -1;
}

static int _fibres_in_my_thread() {

	master_thread_t * m = pthread_getspecific(master_key);

	return m->ref;
}

int ff_yield_to(ff_handle f) {

	master_thread_t * m = pthread_getspecific(master_key);

	ff_handle me = m->running;

	if(me == f)
		return 0; // yield to self.

	if( f && ( f->master != m ) )
		return -1; // cannot migrate fibres

	if(_fibres_in_my_thread() <= 1)
		return -1; // nothing to yield to!

	m->next = f;

	pthread_cond_broadcast(&m->condition);
	_wait_for_runnable(me);

	return 0;
}

int ff_yield() {

	return ff_yield_to(NULL);
}

int ff_set_exit_to(ff_handle h) {

	int err = 0;

	master_thread_t * m = pthread_getspecific(master_key);

	if(!h)
		m->running->exit_to = NULL;
	else if( h->master == m)
		m->running->exit_to = h;
	else {
		err = -1;
	}
	return err;
}

static int _ff_exit_to(ff_handle h) {

	int err = 0;

	master_thread_t * m = pthread_getspecific(master_key);

	if( h && (h->master == m) )
		m->next = h;
	else {
		m->next = NULL;
		if(h)
			err = -1;
	}

	free(m->running);
	m->running = NULL;
	pthread_mutex_unlock(&m->mutex);
	pthread_cond_broadcast(&m->condition);

	_free_master_struct(m);

	pthread_exit(NULL);
	return err;
}

int ff_exit() {

	master_thread_t * m = pthread_getspecific(master_key);

	return _ff_exit_to(m->running->exit_to);
}

int ff_kill(ff_handle f) {

	if(f)  {

		master_thread_t * m = pthread_getspecific(master_key);

		if(f->master->running == f) {
			// suicide... ERROR! use ff_exit() to end callers fibre.
			// caller wont be expecting its 'exit_to' fibre to be woken by this function.
			return -1;
		}
		else if(f->master != m) {
			
			// cant migrate fibres between threads.
			//  in other words.. fibres can only kill other fibers in its own thread... sisters!
			return -1;
		}
		else {
			// murder... 
			// NOTE that pthread_cancel is optional... and not implemented on some platforms ( ANDROID! ).
			// so, for portability sake, just set a kill flag.
			f->kill_flag = 1; 

			// We are about to wake 'f' so it can kill itself...
			//  caller will not expect a different fiber to wake,
			//  so make sure this is the next fibre to run!
			f->exit_to = f->master->running;

			// wake 'f' so it can kill itself.
			return ff_yield_to(f);
		}
	}
	return -1;
}


