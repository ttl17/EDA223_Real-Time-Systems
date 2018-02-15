
struct call_block;
typedef struct call_block *Caller;

typedef struct call_block {
    Caller	next;	// for use in linked lists
    Object	*obj;
    Method	meth;
} CallBlock;

#define initCallBlock() { 0, 0, 0 }

typedef struct {
    Object	super;
    int		value;
    Caller	queue;
} Semaphore;

void Wait(Semaphore*, int);
void Signal(Semaphore*, int);
