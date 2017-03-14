#ifndef GEL_H
#define GEL_H

typedef struct gel gel_t;

typedef enum event_type {
	ARRIVAL,
	DEPARTURE,
	SENSE_DIFS,
	TRANSMIT
} event_type_t;

typedef struct event {
	double time;
	int backoff;
	double difs;		/* Distributed Inter-frame Space */
	int src_host;
	int dest_host;
	event_type_t type;
} event_t;

gel_t *gel_create(void);
int gel_insert(gel_t *gel, event_t *event);
int gel_get(gel_t *gel, event_t *event);
int gel_destroy(gel_t *gel);
event_t *gel_create_event(
	double time,
	int backoff,
	double difs,
	int src_host,
	int dest_host,
	event_type_t type
);

#endif