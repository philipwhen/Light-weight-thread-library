#ifndef LWT_H
#define LWT_H

// #include <stdlib.h>
// #include <stdio.h>
#include "lwt_list.h"

#include "sl.h"
#include <cos_defkernel_api.h>

/* turn this on will enable error check for user input */
#ifdef DEBUG
#define printd(format, ...)  //printc("%s Line: %05d: " format, __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define printd(format, ...)
#endif


/* define size of stack for created thread */
#define MAX_STACK_SIZE (100 * 1024 * 8)
#define MAX_KTHD_NUM 35
#define MAX_CHL_NUM 35

/* define NULL pointer */
#define LWT_NULL NULL

/* define prediction */
// #define likely(x)   __builtin_expect((x),1)
// #define unlikely(x) __builtin_expect((x),0)

/* attributes of the created thread */
#define LWT_NOJOIN 1

/* Data type redefinition */
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef int t_id;
typedef int t_stat;
typedef int lwt_flags_t;

/* struct declaration */
struct _lwt_cgrp;
typedef struct _lwt_cgrp * lwt_cgrp_t;
/* User level info argument type.  */
enum lwt_info_t
{
    /* The thread is running. */
    LWT_INFO_NTHD_RUNNABLE = 0,
    /* The thread is blocked. */
    LWT_INFO_NTHD_BLOCKED,
    /* This is a zombie thread. */
    LWT_INFO_NTHD_ZOMBIES,
    /* number of threads waiting to be re-used, not freed yet */
    LWT_INFO_NTHD_RECYCLE,
    /* number of threads blocked sedning */
    LWT_INFO_NSNDING,
    /* number of threads blocked receiving */
    LWT_INFO_NRCVING,
    /* number of threads blocked receiving */
    LWT_INFO_NJOINING,
};

/* thread status */
enum lwt_status
{
    /* The thread is running. */
    LWT_RUNNABLE = 0,
    /* The thread is blocked. */
    LWT_BLOCKED,
    /* This is a zombie thread. */
    LWT_ZOMBIES,
    /* This thread is currently running. */
    LWT_RUNNING
};

/* if status is blocked, this shows status for blockage */
enum block_status
{
    /* The thread is blocked. */
    BLOCKED_RECEIVING = 0,
    /* This is a zombie thread. */
    BLOCKED_SENDING,
};

enum chan_type
{
    /* The channel is only used for threads within one kernel threads. */
    LOCAL_CHAN = 0,
    /* This channel is accessed by multiple kernel threads. */
    GLOBAL_CHAN,
};


/* define the context of a thread */
struct _lwt_context
{
    unsigned int ip, sp;
};

/* buffer ring of the channel*/
struct _buffer_ring
{
    int tail, head, count;
    void * data_buffer;
};

/* This structure describes a lwt channel */
struct _lwt_channel
{
    /* channel ID */
    int chan_id;

    /* channel type */
    enum chan_type type;
    
    /* head of linked list, serve as a reference to its sender queue */
    struct list_head sender_queue;

    /* lock */
    int sq_occupied;
    
    /* Linked list node, used to find its position in global channel queue */
    struct list list_node;
    
    /* number of senders have access to this channel */
    int snd_cnt;
    
    /* receiver thread */
    struct _lwt_t * receiver;
    
    /* buffer ring */
    struct _buffer_ring buffer;

    int size;

    /* lock */
    int rb_occupied;
    
    /* channel group need??*/
    lwt_cgrp_t cgroup;
    
    /* if there is a event */
    int ready;
    
    /* mark of the channel*/
    void * mark;

};
typedef struct _lwt_channel * lwt_chan_t;



/* This structure describes a LWT thread. */
struct _lwt_t
{

    struct list list_node;

    /* Thread id */
    t_id lwt_id;

    /* the status of a thread */
    enum lwt_status state;

    /* the reason for blockage */
    enum block_status block_for;

    /* parent thread */
    struct _lwt_t * parent;

    /* data sending to other thread */
    void * message_data;

    /* initial stack memory pointer, lowest address */
    unsigned int init_sp;

    /* Thread context */
    struct _lwt_context context;
    
    /* default channel used to send last word */
    lwt_chan_t chl;
    lwt_chan_t now_rcving;

    // /* k thd information */
    // struct _lwt_cap * kthd;

    /* thread join state */
    int nojoin;

    /* kernel thread */
    int owner_kthd;
};

typedef struct _lwt_t * lwt_t;

/* channel group */
struct _lwt_cgrp
{
    struct list_head chl_list;
    lwt_t owner;
    int blocked;
};


struct __lwt_wrap
{
    struct list list_node;
    lwt_t thd;
};

/* define a function pointer */
typedef void * (*lwt_fn_t)(void *);
typedef void * (*lwt_chan_fn_t)(lwt_chan_t);

struct _kthd_info
{
    
    /* below variables used to be global but now belongs to kthd */
    int block_counter;
    int zombie_counter;
    int nrcving;
    int nsnding;
    int polling_flag;
    lwt_t current_thread;
    lwt_t main_thread;
    struct list_head run_queue;
    struct list_head recycle_queue;
    struct list_head wakeup_queue;
    int wq_occupied;
};


/* --------------------------- Funciton declaration for lwt thread operation --------------------------- */
lwt_t  lwt_create(lwt_fn_t fn, void * data, lwt_flags_t flags);
void * lwt_join(lwt_t thread_to_wait);
void lwt_die(void *);
int lwt_yield(lwt_t strong_thread);
lwt_t lwt_current();
int lwt_id(lwt_t input_thread);
lwt_t lwt_current();
struct list_head * current_run_queue();
struct list_head * current_recycle_queue();
struct list_head * owner_run_queue();
void lwt_init_cap(struct _lwt_cap *);
void __initiate(thdid_t);

static inline void
cas_lock(int * c)
{
retry:
    if (!ps_cas(c, 0, 1)) goto retry;
    return;
}

static inline void
cas_unlock(int * c)
{
    *c = 0;
}



/* --------------------------- Function declaration for lwt thread channel operation --------------------------- */
lwt_chan_t lwt_chan (int sz);
void lwt_chan_deref (lwt_chan_t c);
int lwt_snd(lwt_chan_t c, void * data);
void *lwt_rcv(lwt_chan_t c);
int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);
lwt_chan_t lwt_rcv_chan(lwt_chan_t c);
lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c);

lwt_cgrp_t lwt_cgrp (void);
int lwt_cgrp_free (lwt_cgrp_t cgrp);
int lwt_cgrp_add (lwt_cgrp_t cgrp, lwt_chan_t chan);
int lwt_cgrp_rem(lwt_cgrp_t cgrp, lwt_chan_t chan);
lwt_chan_t lwt_cgrp_wait (lwt_cgrp_t cgrp);

void lwt_chan_mark_set(lwt_chan_t, void *);
void *lwt_chan_mark_get(lwt_chan_t);
void set_chan_type(lwt_chan_t chan, enum chan_type type);

/* --------------- kernel thread API --------------- */
void lwt_kthd_trampline(void *);
struct sl_thd * lwt_kthd_create(lwt_fn_t fn, lwt_chan_t c, int pri);
int lwt_snd_thd(lwt_chan_t chan, lwt_t sending);
lwt_t lwt_rcv_thd(lwt_chan_t chan);


/* --------------------------- debugging function --------------------------- */
void print_queue_content(enum lwt_info_t);
int lwt_info(enum lwt_info_t t);
extern int printc(char * fmt, ...);
/* --------------- global variable --------------- */
extern struct _kthd_info kthds[];
extern thdid_t current_kthd;
extern struct cos_compinfo *ci;
extern int lwt_counter;
extern int chan_counter;

#endif
