/*
 * =====================================================================================
 *
 *       Filename:  event_dispatcher.h
 *
 *    Description: This file defines the data structures for Event Dispatcher Design 
 *
 *        Version:  1.0
 *        Created:  10/20/2020 08:47:29 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef EVENT_DISPATCHER
#define EVENT_DISPATCHER

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include "../gluethread/glthread.h"

#define ENABLE_EVENT_DISPATCHER
//#undef ENABLE_EVENT_DISPATCHER
#define EV_DIS_NAME_LEN 32

typedef struct event_dispatcher_ event_dispatcher_t;
typedef struct task_ task_t;
typedef struct pkt_q_ pkt_q_t;

typedef void (*event_cbk)(event_dispatcher_t *, void *, uint32_t );

typedef enum {

	TASK_ONE_SHOT,
	TASK_PKT_Q_JOB,
	TASK_BG
} task_type_t;

typedef enum {

	TASK_CLI,	/* Is this Task created due to USER CLI interaction */
	TASK_TIMER,	/* Is this Task created by timer expiration */
	TASK_PKT_THREAD, /* Is this Task created for pkt processing */
	TASK_FORKED_TASK, /* Is this Task forked by other task */
	TASK_DEFAULT
} task_src_t;

typedef enum {

	TASK_PRIORITY_FIRST,
	TASK_PRIORITY_HIGH = TASK_PRIORITY_FIRST,
	TASK_PRIORITY_PKT_PROCESSING = TASK_PRIORITY_HIGH,
	TASK_PRIORITY_OPERATIONAL_CLI =  TASK_PRIORITY_HIGH,
	TASK_PRIORITY_TIMERS_CBK = TASK_PRIORITY_HIGH,

	TASK_PRIORITY_MEDIUM,
	TASK_PRIORITY_COMPUTE = TASK_PRIORITY_MEDIUM,
	
	TASK_PRIORITY_LOW,
	TASK_PRIORITY_CONFIG_CLI =  TASK_PRIORITY_LOW,
	TASK_PRIORITY_GARBAGE_COLLECTOR = TASK_PRIORITY_LOW,
	TASK_PRIORITY_MAX
} task_priority_t;

struct task_{

	void *data;
	uint32_t data_size;
	event_cbk ev_cbk;
	uint32_t no_of_invocations;
	task_type_t task_type;
	bool re_schedule;
	task_priority_t priority;
	pthread_cond_t *app_cond_var; /* For synchronous Schedules */
	glthread_t glue;
};
GLTHREAD_TO_STRUCT(glue_to_task,
	task_t, glue);

#define PKT_Q_MAX_QUEUE_SIZE	500

struct pkt_q_{

	glthread_t q_head;
	uint32_t pkt_count;
	uint32_t drop_count;
	pthread_mutex_t q_mutex;
	task_t *task;
	glthread_t glue;
	event_dispatcher_t *ev_dis;
};
GLTHREAD_TO_STRUCT(glue_to_pkt_q,
	pkt_q_t, glue);

typedef enum {

	EV_DIS_IDLE,
	EV_DIS_TASK_FIN_WAIT,
} EV_DISPATCHER_STATE;

struct event_dispatcher_{

	unsigned char name[EV_DIS_NAME_LEN];
	pthread_mutex_t ev_dis_mutex;

	glthread_t task_array_head[TASK_PRIORITY_MAX];	
	uint32_t pending_task_count;

	glthread_t pkt_queue_head;

	EV_DISPATCHER_STATE ev_dis_state;

	pthread_cond_t ev_dis_cond_wait;
	bool signal_sent;
	uint32_t signal_sent_cnt;
	uint32_t signal_recv_cnt;
	pthread_t *thread;	
	uint64_t n_task_exec;
	/* Some optional app data */
	void *app_data;
	task_t *current_task;
	struct timeval current_task_start_time;
};

#define EV_DIS_LOCK(ev_dis_ptr)		\
	(pthread_mutex_lock(&((ev_dis_ptr)->ev_dis_mutex)))

#define EV_DIS_UNLOCK(ev_dis_ptr)	\
	(pthread_mutex_unlock(&((ev_dis_ptr)->ev_dis_mutex)))

/* To be used by applications */
task_t *
eve_dis_get_current_task(event_dispatcher_t *ev_dis);

bool
event_dispatcher_should_suspend (event_dispatcher_t *ev_dis);

void
event_dispatcher_init(event_dispatcher_t *ev_dis, const char *name);

void
event_dispatcher_run(event_dispatcher_t *ev_dis);


task_t *
task_create_new_job(
	event_dispatcher_t *ev_dis,
    void *data,
    event_cbk cbk,
	task_type_t task_type,
	task_priority_t priority);

task_t *
task_create_new_job_synchronous(
	event_dispatcher_t *ev_dis,
    void *data,
    event_cbk cbk,
	task_type_t task_type,
	task_priority_t priority);

void
task_cancel_job(event_dispatcher_t *ev_dis, task_t *task);

void
init_pkt_q(event_dispatcher_t *ev_dis, pkt_q_t *pkt_q, event_cbk cbk);

bool
pkt_q_enqueue(event_dispatcher_t *ev_dis, 
			   pkt_q_t *pkt_q,
			  char *pkt, uint32_t pkt_size);

char *
task_get_next_pkt(event_dispatcher_t *ev_dis, uint32_t *pkt_size);

void
task_schedule_again(event_dispatcher_t *ev_dis, task_t *task);

static inline uint32_t 
ptk_q_drop_count (pkt_q_t *pkt_q){

	uint32_t rc;
	pthread_mutex_lock(&pkt_q->q_mutex);
	rc = pkt_q->drop_count;
	pthread_mutex_unlock(&pkt_q->q_mutex);
	return rc;
}

#endif /* EVENT_DISPATCHER  */
