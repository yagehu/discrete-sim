#include <stdlib.h>
#include <stdio.h>

#include "gel.h"

typedef struct gel_node {
	event_t *event;
	struct gel_node *next;
	struct gel_node *prev;
} gel_node_t;

struct gel {
	gel_node_t *front;
	gel_node_t *rear;
	unsigned int length;
};

gel_t *gel_create(void)
{
	gel_t *gel = (gel_t *)malloc(sizeof(gel_t));
	gel->front = NULL;
	gel->rear = NULL;
	gel->length = 0;
	return gel;
}

int gel_insert(gel_t *gel, event_t *new_event)
{
	/* Check for NULL */
	if (!gel)
		return -1;

	gel_node_t *new = malloc(sizeof(gel_node_t));
	new->event = new_event;

	/* If GEL is empty */
	if (gel->length == 0) {
		new->next = NULL;
		new->prev = NULL;
		gel->front = new;
		gel->rear = new;
		gel->length = 1;
		return 0;
	} else { /* else GEL is not empty */
		gel_node_t *current = gel->front;

		while (current) {
			if (new->event->time > current->event->time) {
				if (current->next) {
					current = current->next;
				} else {
					current->next = new;
					new->next = NULL;
					new->prev = current;
					gel->rear = new;
					gel->rear->next = NULL;
					gel->length++;
					return 0;
				}
			} else if (new->event->time == current->event->time) {
				printf("same\n");
				exit(0);
			} else {
				if (current->prev) {
					new->prev = current->prev;
					new->next = current;
					current->prev->next = new;
					current->prev = new;
					gel->length++;
					return 0;
				} else {
					current->prev = new;
					new->prev = NULL;
					new->next = current;
					gel->front = new;
					gel->length++;
					return 0;
				}
			}
		}
	}

	return -1;
}

/* Get first event */
int gel_get(gel_t *gel, event_t *event)
{
	/* Check for NULL and empty GEL */
	if (!gel || gel->length == 0)
		return -1;

	event->time = gel->front->event->time;
	event->backoff = gel->front->event->backoff;
	event->difs = gel->front->event->difs;
	event->src_host = gel->front->event->src_host;
	event->dest_host = gel->front->event->dest_host;
	event->type = gel->front->event->type;

	/* If only one event in GEL */
	if (gel->length == 1) {
		gel->front = NULL;
		gel->rear = NULL;
		gel->length = 0;
		return 0;
	}

	gel->front = gel->front->next;
	free(gel->front->prev);
	gel->front->prev = NULL;
	gel->length--;
	return 0;
}

int gel_destroy(gel_t *gel)
{
	/* Check for NULL argument */
	if (!gel)
		return -1;

	gel_node_t *current = gel->front;

	while (current) {
		gel_node_t *temp = current;
		current = current->next;
		free(temp->event);
		free(temp);
	}

	free(gel);

	return 0;
}

event_t *gel_create_event(
	double time,
	int backoff,
	double difs,
	int src_host,
	int dest_host,
	event_type_t type)
{
	event_t *new = malloc(sizeof(event_t));
	new->time = time;
	new->backoff = backoff;
	new->difs = difs;
	new->src_host = src_host;
	new->dest_host = dest_host;
	new->type = type;

	return new;
}
