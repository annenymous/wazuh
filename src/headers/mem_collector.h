/*
    mem_collector implements te collection of dynamic memory request
    though different methods and forget about freeing them until all
    the process is done.

    It usage its recommended for highly convoluted memory requests.
    Simple "alloc & free" can be left outside

    It requires 3 x sizeof(pointer) for the specific arquitecture by every
    node, as well as 2 x sizeof(pointer) for the list

    Its implemented with a stack or LIFO behaviour
*/
typedef struct collector_node_t collector_node_t;
typedef struct collector_node_t{
    void *item;
    void(*free)(void *);
    collector_node_t *previous;
}collector_node_t;

typedef struct mem_collector_list_t{
    collector_node_t *first;
    collector_node_t *last;
}mem_collector_list_t;

/*
    Pushes a new object address into the collector stack, along with the freeing function for it
    (a wrapper might be needed).

    collector: collector list.
    address: object address listed to be freed

*/
void * mem_collector_push(mem_collector_list_t *collector, void *address, void(*free)(void *));

/*
    Pops an object from the collector.
    collector: collector list.
*/
void mem_collector_pop(mem_collector_list_t *collector);

/*
    Flushes all objects from memory, using the associated free fuction for each object.

*/
void mem_collector_flush(mem_collector_list_t *collector);
