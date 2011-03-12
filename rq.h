#ifndef __RQ_H
#define __RQ_H

#include <event.h>
#include <netdb.h>
#include <expbuf.h>
#include <linklist.h>
#include <risp.h>
#include <rispbuf.h>

// This version indicates the version of the library so that developers of
// services can ensure that the correct version is installed.
// This version number should be incremented with every change that would
// effect logic.
#define LIBRQ_VERSION  0x00010910
#define LIBRQ_VERSION_NAME "v1.09.10"


// include libevent.  If we are using 1.x version of libevent, we need to do 
// things slightly different than if we are using 2.x of libevent.
#include <event.h>
#if ( _EVENT_NUMERIC_VERSION < 0x02000000 )
#define LIBEVENT_OLD_VER  
typedef int evutil_socket_t;
#endif



// #if (LIBEVENT_VERSION_NUMBER < 0x02000200)
// 	#error "Needs libEvent v2.00.02 or higher"
// #endif

#if (EXPBUF_VERSION < 0x00010200)
	#error "Needs libexpbuf v1.2.1 or higher"
#endif

#if (LIBLINKLIST_VERSION < 0x00008000)
	#error "Needs liblinklist v0.80 or higher"
#endif



// Since we will be using a number of bit masks to check for data status's and
// so on, we should include some macros to make it easier.
#define BIT_TEST(arg,val) (((arg) & (val)) == (val))
#define BIT_SET(arg,val) ((arg) |= (val))
#define BIT_CLEAR(arg,val) ((arg) &= ~(val))
#define BIT_TOGGLE(arg,val) ((arg) ^= (val))




#define INVALID_HANDLE -1

// global constants and other things go here.
#define RQ_DEFAULT_PORT      13700

// start out with an 1kb buffer.  Whenever it is full, we will double the
// buffer, so this is just a minimum starting point.
#define RQ_DEFAULT_BUFFSIZE	1024

// The priorities are used to determine which node to send a request to.  A
// priority of NONE indicates taht this node should only receive broadcast
// messages, and no actual requests.
#define RQ_PRIORITY_NONE        0
#define RQ_PRIORITY_LOW        10
#define RQ_PRIORITY_NORMAL     20
#define RQ_PRIORITY_HIGH       30



typedef int queue_id_t;
typedef int msg_id_t;


typedef struct {
	risp_t *risp;
	struct event_base *evbase;

	// linked-list of our connections.  Only the one at the head is likely to be
	// active (although it might not be).  When a connection is dropped or is
	// timed out, it is put at the bottom of the list.
	list_t connlist;		/// rq_conn_t

	// Linked-list of queues that this node is consuming.
	list_t queues;			/// rq_queue_t

	// pool of messages.
	list_t *msg_pool;

	void **msg_list;
	int msg_max;
	int msg_used;
	int msg_next;
} rq_t;





#define RQ_DATA_FLAG_NOREPLY      256

#define RQ_DATA_MASK_PRIORITY     1
#define RQ_DATA_MASK_QUEUEID      2
#define RQ_DATA_MASK_TIMEOUT      4
#define RQ_DATA_MASK_ID           8
#define RQ_DATA_MASK_QUEUE        16
#define RQ_DATA_MASK_PAYLOAD      32


typedef struct {
 	unsigned int mask;
	unsigned int flags;
	
	msg_id_t id;
	queue_id_t qid;
	unsigned short timeout;
	unsigned short priority;
	expbuf_t *payload;
	expbuf_t *queue;
} rq_data_t;


typedef struct {
	evutil_socket_t handle;		// socket handle to the connected controller.
	char active;
	char closing;
	char shutdown;
	struct event *read_event;
	struct event *write_event;
	struct event *connect_event;
	rq_t *rq;
	risp_t *risp;
	char *hostname;
	
	expbuf_t *inbuf, *outbuf, *readbuf, *sendbuf;
	rq_data_t *data;
	
} rq_conn_t;


typedef struct __rq_message_t {
	msg_id_t    id;
	msg_id_t    src_id;
	char        broadcast;
	char        noreply;
	expbuf_t   *data;
	const char *queue;
	rq_t       *rq;
	rq_conn_t  *conn;
	enum {
		rq_msgstate_new,
		rq_msgstate_delivering,
		rq_msgstate_delivered,
		rq_msgstate_replied
	} state;
	void (*reply_handler)(struct __rq_message_t *msg);
	void (*fail_handler)(struct __rq_message_t *msg);
	void *arg;
} rq_message_t;

typedef struct {
	char *queue;
	queue_id_t qid;
	char exclusive;
	short int max;
	unsigned char priority;
	
	void (*handler)(rq_message_t *msg, void *arg);
	void (*accepted)(char *queue, queue_id_t qid, void *arg);
	void (*dropped)(char *queue, queue_id_t qid, void *arg);
	
	void *arg;
} rq_queue_t;


void rq_set_maxconns(int maxconns);
int  rq_new_socket(struct addrinfo *ai);
void rq_daemon(const char *username, const char *pidfile, const int noclose);

void rq_init(rq_t *rq);
void rq_shutdown(rq_t *rq);
void rq_cleanup(rq_t *rq);
void rq_setevbase(rq_t *rq, struct event_base *base);

// add a controller to the list, and it should attempt to connect to one of
// them.   Callback functions can be provided so that actions can be performed
// when there are no connections to the controllers.
void rq_addcontroller(
	rq_t *rq,
	char *host,
	void *connect_handler,
	void *dropped_handler,
	void *arg);

// start consuming a queue.
void rq_consume(
	rq_t *rq,
	char *queue,
	int max,
	int priority,
	int exclusive,
	void (*handler)(rq_message_t *msg, void *arg),
	void (*accepted)(char *queue, queue_id_t qid, void *arg),
	void (*dropped)(char *queue, queue_id_t qid, void *arg),
	void *arg);


rq_message_t * rq_msg_new(rq_t *rq, rq_conn_t *conn);
void rq_msg_clear(rq_message_t *msg);
void rq_msg_setqueue(rq_message_t *msg, const char *queue);
void rq_msg_setbroadcast(rq_message_t *msg);
void rq_msg_setnoreply(rq_message_t *msg);


// macros to add RISP commands to the message buffer.   This is better than
// addng commands to a seperate buffer and then copying it to the message
// buffer.
#define rq_msg_addcmd(m,c)              (addCmd((m)->data,(c)))
#define rq_msg_addcmd_shortint(m,c,v)   (addCmdShortInt((m)->data,(c), (v)))
#define rq_msg_addcmd_int(m,c,v)        (addCmdInt((m)->data,(c),(v)))
#define rq_msg_addcmd_largeint(m,c,v)   (addCmdLargeInt((m)->data,(c), (v)))
#define rq_msg_addcmd_shortstr(m,c,l,s) (addCmdShortStr((m)->data,(c), (l), (s)))
#define rq_msg_addcmd_str(m,c,l,s)      (addCmdStr((m)->data,(c), (l), (s)))
#define rq_msg_addcmd_largestr(m,c,l,s) (addCmdLargeStr((m)->data,(c), (l), (s)))



// For situations where RISP messaging is not used, raw data can be supplied
// instead.  The contents will be copied into a buffer.
void rq_msg_setdata(rq_message_t *msg, int length, char *data);


void rq_send(
	rq_message_t *msg,
	void (*reply_handler)(rq_message_t *reply),
	void (*fail_handler)(rq_message_t *msg),
	void *arg);

void rq_resend(rq_message_t *msg);
void rq_reply(rq_message_t *msg, int length, char *data);


// This value is the number of elements we pre-create for the message list.
// When the system is running, it should always assume that there is at least
// something in the list.   The list will grow as the need arises, so this
// number doesn't really matter much, except to maybe tune it a little better.
#define DEFAULT_MSG_ARRAY 10


#endif
