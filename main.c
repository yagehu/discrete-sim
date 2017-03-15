#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gel.h"
#include "queue.h"

/* Arrival rate of events */
#define ARRIVAL_RATE		0.9
/* Data frame length rate */
#define DATA_RATE		0.003
/* Maximum data frame length in bytes */
#define MAX_DATA_LENGTH		1544.0
/* Number of hosts */
#define HOST_COUNT		10
/* Maximum backoff value */
#define MAX_BACKOFF		100
/* Acknowledgement frame size in bytes */
#define ACK_SIZE		64
/* Wireless channel capacity in bps */
#define WLAN_CAP		11000000
/* Short Inter-frame Space in ms */
#define SIFS			0.05
/* Distributed Inter-frame Space in ms */
#define DIFS			0.1
/* Time between each channel sense in ms */
#define DT			0.01
/* Number of events to simulate */
#define MAX_EVENT		1000000

typedef struct network_t {
	queue_t **hosts;
	bool is_busy;
} network_t;

void process_arrival_event(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay);
void process_departure(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay,
	int *total_bytes_sent);
void process_sense(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay);
double neg_exp_gen(double rate);
int generate_data_frame_length(void);
int generate_dest_host(int src_host);

void print(gel_t *gel, int count, double current_time)
{
	for (int i = 0; i < count; i++) {
		event_t *temp = malloc(sizeof(event_t));
		gel_get(gel, temp);
		printf(
			"%lf %lf %d %d %d\n",
			current_time,
			temp->time,
			temp->src_host,
			temp->dest_host,
			temp->type
		);
	}
}

int main(void)
{
	int total_bytes_sent = 0;
	double total_network_delay = 0.0;
	double current_time = 0.0;
	/* Initialize GEL */
	gel_t *gel = gel_create();
	/* Initialize network_t struct containing hosts */
	network_t *network = malloc(sizeof(network_t));
	network->hosts = malloc(sizeof(queue_t *) * HOST_COUNT);
	network->is_busy = false;

	/* Initialize FIFO buffer for all hosts */
	for (int i = 0; i < HOST_COUNT; i++)
		network->hosts[i] = queue_create();

	/* Initialize random number generator */
	srand48(time(NULL));

	/* Create 1st arrival event for each host and insert into the GEL */
	for (int i = 0; i < HOST_COUNT; i++) {
		/* Create new event */
		event_t *new =
			gel_create_event(
				neg_exp_gen(ARRIVAL_RATE),
				-1,
				-1,
				i,
				-1,
				ARRIVAL
			);

		/* Insert event into GEL */
		gel_insert(gel, new);
	}

	for (int i = 0; i < MAX_EVENT; i++) {
		event_t *current = malloc(sizeof(event_t));
		gel_get(gel, current);

		switch (current->type) {
		case ARRIVAL:
			process_arrival_event(
				gel,
				network,
				*current,
				&current_time,
				&total_network_delay
			);
			break;
		case SENSE:
			process_sense(
				gel,
				network,
				*current,
				&current_time,
				&total_network_delay
			);
			break;
		case DEPARTURE:
			process_departure(
				gel,
				network,
				*current,
				&current_time,
				&total_network_delay,
				&total_bytes_sent
			);
		}
	}

	/* Print results */
	printf("Discrete-event simulation of WLAN:\n");
	printf("/************************************************/\n");
	printf("/*                  Parameters                  */\n");
	printf("/************************************************/\n");
	printf("Number of hosts: %d\n", HOST_COUNT);
	printf("Number of events simulated: %d\n", MAX_EVENT);
	printf("Packet arrival rate: %lf\n", ARRIVAL_RATE);
	printf("Data frame length rate: %lf\n", DATA_RATE);
	printf("Backoff value: %d\n", MAX_BACKOFF);
	printf("Short Inter-frame Space (SIFS): %lf ms\n", SIFS);
	printf("Distributed Inter-frame Space (DIFS): %lf ms\n", DIFS);
	printf("/************************************************/\n");
	printf("/*                   Results                    */\n");
	printf("/************************************************/\n");
	printf("Simulation time: %lf ms\n", current_time);
	printf("Total network delay: %lf ms\n", total_network_delay);
	printf("Total bytes sent: %d\n", total_bytes_sent);

	/* Clean up */
	gel_destroy(gel);

	for (int i = 0; i < HOST_COUNT; i++)
		queue_destroy(network->hosts[i]);

	free(network->hosts);
	free(network);

	return 0;
}

void process_arrival_event(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay)
{
	/* Compute arrival time of next arrival event */
	double next_arrival_time 
		= event.time + neg_exp_gen(ARRIVAL_RATE);
	/* Compute service time using randomly generated data frame length */
	double *service_time = malloc(sizeof(double));
	int data_frame_length = generate_data_frame_length();
	*service_time =
		((double)data_frame_length * 8) / ((double)WLAN_CAP)
		+ ((double)ACK_SIZE * 8) / ((double)WLAN_CAP)
		+ SIFS;

	/* Update current time */
	*current_time = event.time;
	/* Create next ARRIVAL event */
	event_t *new =
		gel_create_event(
			next_arrival_time,
			-1,
			-1,
			event.src_host,
			-1,
			ARRIVAL
		);
	/* Insert next arrival event */
	gel_insert(gel, new);

	/* If buffer in source host is empty */
	if (queue_length(network->hosts[event.src_host]) == 0) {
		/* Pick a random host to send to */
		int dest_host = generate_dest_host(event.src_host);
		/* Update total network delay */
		*total_network_delay += DT;
		/* Add packet transmission time to source host queue */
		queue_enqueue(
			network->hosts[event.src_host],
			(void *)service_time
		);

		/* If channel is busy, start backoff counter */
		if (network->is_busy) {
			/* Create new event for sensing backoff */
			new =
				gel_create_event(
					*current_time + DT,
					round(drand48() * MAX_BACKOFF),
					0,
					event.src_host,
					dest_host,
					SENSE
				);
			gel_insert(gel, new);
		} else { /* Else channel is idle, wait for DIFS */
			/* Create new event for sensing channel */
			new =
				gel_create_event(
					*current_time + DT,
					1,
					0,
					event.src_host,
					dest_host,
					SENSE
				);
			gel_insert(gel, new);
		}
	} else { /* Else buffer in source host is not empty */
		/*
		 * Buffer length is infinite so no dropped packets.
		 * Add packet transmission time to source host queue
		 */
		queue_enqueue(
			network->hosts[event.src_host],
			(void *)service_time
		);
	}
}

void process_departure(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay,
	int *total_bytes_sent)
{
	/* Update current time */
	*current_time = event.time;
	/* Set channel to idle */
	network->is_busy = false;

	double *service_time;
	queue_dequeue(network->hosts[event.src_host], (void **)&service_time);
printf("%lf\n", (*service_time - SIFS) * WLAN_CAP / 8 - ACK_SIZE);
	*total_bytes_sent += (*service_time - SIFS) * WLAN_CAP / 8 - ACK_SIZE;
}

void process_sense(
	gel_t *gel,
	network_t *network,
	event_t event,
	double *current_time,
	double *total_network_delay)
{
	/* Update current time */
	*current_time = event.time;

	/* If channel is sensed idle */
	if (!network->is_busy) {
		int backoff;
		double difs = event.difs + DT;

		/* If channel idle for 1 DIFS */
		if (difs >= DIFS) {
			/* Decrement backoff counter */
			backoff = event.backoff - 1;
		} else { /* Else not DIFS yet, keep sensing */
			event_t *new =
				gel_create_event(
					*current_time + DT,
					event.backoff,
					difs,
					event.src_host,
					event.dest_host,
					event.type
				);
			gel_insert(gel, new);
			*total_network_delay += DT;
			return;
		}

		/* If backoff counter reaches 0, transmit */
		if (backoff == 0) {
			/* Get transmission time of the pkt */
			double *service_time;
			queue_get(
				network->hosts[event.src_host],
				(void **)&service_time
			);
			/* Set channel to busy */
			network->is_busy = true;
			/* Create new DEPARTURE event */
			event_t *new =
				gel_create_event(
					*current_time + *service_time,
					-1,
					-1,
					event.src_host,
					event.dest_host,
					DEPARTURE
				);
			gel_insert(gel, new);
			/* Add transmission time to total delay */
			*total_network_delay += *service_time;
		} else { /* Else backoff counter is greater than 0 */
			/* Create sense event with new backoff counter value */
			event_t *new =
				gel_create_event(
					*current_time + DT,
					backoff,
					0,
					event.src_host,
					event.dest_host,
					SENSE
				);
			gel_insert(gel, new);
			/* Update total delay */
			*total_network_delay += DT;
		}
	} else { /* If channel is sensed busy */
		/* Freeze backoff counter and create new sense event */
		event_t *new =
			gel_create_event(
				*current_time + DT,
				event.backoff,
				0,
				event.src_host,
				event.dest_host,
				SENSE
			);
		gel_insert(gel, new);
		/* Update total delay */
		*total_network_delay += DT;
	}
}

double neg_exp_gen(double rate)
{
	double u = drand48();
	return ((-1 / rate) * log(1 - u));
}

int generate_data_frame_length(void)
{
	/* Generate temp used for determining data frame length */
	double temp = neg_exp_gen(DATA_RATE);

	/* If generated value greater than MAX_DATA_LENGTH - 1,
	 * return MAX_DATA_LENGTH.
	 */
	if (temp > (MAX_DATA_LENGTH - 1))
		return MAX_DATA_LENGTH;
	else /* Else add 1 to temp and round to nearest int */
		return round(temp + 1);
}

int generate_dest_host(int src_host)
{
	int dest_host;

	if (HOST_COUNT == 2) {
		dest_host = (src_host == 0) ? 1 : 0;
		return dest_host;
	} else
		dest_host = rand() % (HOST_COUNT - 1) + 1;

	do {
		if (dest_host != src_host)
			return dest_host;

		dest_host = rand() % (HOST_COUNT - 1) + 1;
	} while (1);
}