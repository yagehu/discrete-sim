#include <stdlib.h>

#include "queue.h"

typedef struct node {
	void *data;
	struct node *next;
} node_t;

struct queue {
	node_t *front;
	node_t *rear;
	int length;
};

queue_t *queue_create(void)
{
	queue_t *q = (queue_t *)malloc(sizeof(queue_t));
	q->front = NULL;
	q->rear = NULL;
	q->length = 0;

	return q;
}

int queue_enqueue(queue_t *queue, void *data)
{
	/* Check for NULL arguments */
	if (!queue || !data)
		return -1;

	node_t *new = (node_t *)malloc(sizeof(node_t));
	new->data = data;
	new->next = NULL;

	/* If queue is empty */
	if (queue->rear == NULL) {
		queue->front = new;
		queue->rear = new;
		queue->length++;
		return 0;
	}

	queue->rear->next = new;
	queue->rear = new;
	queue->length++;
	return 0;
}

int queue_dequeue(queue_t *queue, void **data)
{
	/* Check for NULL arguments */
	if (!queue || !data)
		return -1;

	/* If queue is empty */
	if (queue->front == NULL)
		return -1;

	/* If only one element in queue */
	if (queue->front == queue->rear) {
		*data = queue->front->data;
		queue->front = NULL;
		queue->rear = NULL;
		queue->length--;
		return 0;
	}

	*data = queue->front->data;
	node_t *dequeued = queue->front;
	queue->front = queue->front->next;
	free(dequeued);
	queue->length--;
	return 0;
}

int queue_destroy(queue_t *queue)
{
	/* Check for NULL argument */
	if (!queue)
		return -1;

	if (queue->front) {
		node_t *current = queue->front;

		while (current) {
			node_t *temp = current;
			current = current->next;
			free(temp);
		}
	}

	free(queue);

	return 0;
}

int queue_length(queue_t *queue)
{
	/* Check for NULL argument */
	if (!queue)
		return -1;

	return queue->length;
}