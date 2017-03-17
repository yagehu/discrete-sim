#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct queue queue_t;

queue_t *queue_create(void);
int queue_enqueue(queue_t *queue, void *data, double arrival_time);
int queue_dequeue(queue_t *queue, void **data, double *arrival_time);
int queue_get(queue_t *queue, void **data, double *arrival_time);
int queue_destroy(queue_t *queue);
int queue_length(queue_t *queue);

#endif