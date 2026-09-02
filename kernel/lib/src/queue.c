#include <queue.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <stddef.h>

void queue_init(queue_t* q) {
    q->head = q->tail = NULL;
#ifdef __wasm__
    q->free_items = NULL;
#endif
    q->num = 0;
}

static queue_item_t* queue_item_acquire(queue_t* q) {
#ifdef __wasm__
    queue_item_t* it = q->free_items;
    if(it != NULL)
        q->free_items = it->next;
    else
        it = (queue_item_t*)kmalloc(sizeof(queue_item_t));
#else
    queue_item_t* it = (queue_item_t*)kmalloc(sizeof(queue_item_t));
    (void)q;
#endif
    if(it != NULL)
        memset(it, 0, sizeof(queue_item_t));
    return it;
}

static void queue_item_release(queue_t* q, queue_item_t* it) {
    if(q == NULL || it == NULL)
        return;
#ifdef __wasm__
    it->data = NULL;
    it->prev = NULL;
    it->next = q->free_items;
    q->free_items = it;
#else
    kfree(it);
#endif
}

void queue_push(queue_t* q, void* data) {
    queue_item_t* it = queue_item_acquire(q);
    if(it == NULL)
        return;
    it->data = data;

    if(q->tail == NULL) {
        q->head = q->tail = it;
    }
    else {
        q->tail->next = it;
        it->prev = q->tail;
        q->tail = it;
    }
    q->num++;
}

void queue_push_head(queue_t* q, void* data) {
    queue_item_t* it = queue_item_acquire(q);
    if(it == NULL)
        return;
    it->data = data;

    if(q->head == NULL) {
        q->head = q->tail = it;
    }
    else {
        q->head->prev = it;
        it->next = q->head;
        q->head = it;
    }
    q->num++;
}

void* queue_pop(queue_t* q) {
    if(q->head == NULL)
        return NULL;

    queue_item_t* it = q->head;
    q->head = q->head->next;
    if(q->head) 
        q->head->prev = NULL;
    else
        q->tail = NULL;

    void* ret = it->data;
    queue_item_release(q, it);
    if(q->num > 0)
        q->num--;
    return ret;
}

queue_item_t* queue_in(queue_t* q, void* data) {
    queue_item_t* it = q->head;
    while(it != NULL) {
        if(it->data == data)
            return it;
        it = it->next;
    }
    return NULL;
}

void queue_remove(queue_t* q, queue_item_t* it) {
    if(it == NULL)
        return;
    
    if(it->next != NULL)
        it->next->prev = it->prev;
    if(it->prev != NULL)
        it->prev->next = it->next;
    
    if(it == q->head)
        q->head = it->next;
    if(it == q->tail)
        q->tail = it->prev;
    queue_item_release(q, it);
    if(q->num > 0)
        q->num--;
}

void queue_clear(queue_t* q, free_func_t fr) {
    queue_item_t* it = q->head;
    while(it != NULL) {
        queue_item_t* next = it->next;
        if(fr != NULL && it->data != NULL)
            fr(it->data);
        kfree(it);
        it = next;
    }
#ifdef __wasm__
    it = q->free_items;
    while(it != NULL) {
        queue_item_t* next = it->next;
        kfree(it);
        it = next;
    }
#endif
    q->head = q->tail = NULL;
#ifdef __wasm__
    q->free_items = NULL;
#endif
    q->num = 0;
}

uint32_t queue_num(queue_t* q) {
    if(q->head == NULL)
        return 0;
    return  q->num;
}

bool queue_is_empty(queue_t* q) {
    return (q->head == NULL || q->num == 0);
}
