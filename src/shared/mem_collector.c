#include <stddef.h>
#include "shared.h"
#include "mem_collector.h"
/*
void * mem_collector_push(mem_collector_list_t *collector, void *address, void(*free)(void *)){
    if(!address)
        return NULL;

    if(!collector){
        free(address);
        return (address = NULL);
    }

    return address;
}

void mem_collector_pop(mem_collector_list_t *collector){
    collector_node_t *current;

    if(!collector)
        return;
    
    current = collector->last;
    collector->last = collector->last->previous;

    current->free(current->item);
    os_free(current);
}

void mem_collector_flush(mem_collector_list_t *collector){
    collector_node_t *current;
    collector_node_t *prev;

    if(!collector)
        return;
    
    current = collector->last;

    while(current){
        prev = current->previous;
        current->free(current->item);
        os_free(current);
        current = prev;
    }

    collector->last = NULL;
    collector->first = NULL;
}
*/