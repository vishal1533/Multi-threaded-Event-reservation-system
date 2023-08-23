/* 
    Group 19
    Members: Samiul Haque 224101044
             Abinash Kumar Ray 224101062
             Vishal Chinchkhede 224101057
             Himanshu Chhabra 224101023
    Assignment 4
    Question 3, code in C
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_EVENTS 100
#define AUDITORIUM_CAPACITY 500
#define BOOKING_CAPACITY_LOWER_BOUND 5
#define BOOKING_CAPACITY_UPPER_BOUND 10
#define MAX_ACTIVE_QUERIES 10
#define NUM_THREADS 20
#define TARGET_SECONDS 40
pthread_mutex_t mutex_table = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_active_queries = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct Query
{
    int event;
    int type;
    int thread_id;
};

struct Event
{
    int available_seats;
    pthread_rwlock_t rwlock;
};

struct threadbook
{
    int thread_id;
    int event_no[NUM_EVENTS];
    int no_seat_booked[NUM_EVENTS];
};

struct Query shared_table[MAX_ACTIVE_QUERIES];
struct Event events[NUM_EVENTS];
struct threadbook threaddetail[NUM_THREADS];

// Struct for an event
typedef struct
{
    int available_seats;
    pthread_rwlock_t rwlock;
} event_t;
int active_queries_count = 0;

int emptyindexfinder()
{
    int index = -1;
    for (int i = 0; i < MAX_ACTIVE_QUERIES; i++)
    {
        if (shared_table[i].event == -1)
            index = i;
    }
    return index;
}

void *worker_thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    // Get the current time and target end time
    usleep(1000);
    time_t start_time = time(NULL);
    time_t end_time = start_time + TARGET_SECONDS;
    while (time(NULL) < end_time)
    {
        // sleep for a random short interval
        usleep(1000000);

        // generate a random query
        int query_type = rand() % 3;
        int event_num = rand() % (NUM_EVENTS);
        int booking_capacity = BOOKING_CAPACITY_LOWER_BOUND + (rand() % (BOOKING_CAPACITY_UPPER_BOUND - BOOKING_CAPACITY_LOWER_BOUND + 1));
        int cancel_ticket = rand() % AUDITORIUM_CAPACITY;

        // create query object
        struct Query query = {.event = event_num, .type = query_type, .thread_id = thread_id};

        // acquire the mutex lock on shared table
        pthread_mutex_lock(&mutex_table);
        int idx = emptyindexfinder();
        // wait until max active queries are not exceeded
        while (idx == (-1))
        {
            // pthread_cond_wait(&cond_active_queries, &mutex_table);
            usleep(10);
            idx = emptyindexfinder();
            if (idx == (-1))
                printf("\n*****************************************   THREAD %d blocked  #########################\n", thread_id + 1);
        }

        // if event is not in the table, add query to the table and increment active queries count
        shared_table[idx] = query;

        // release the mutex lock on shared table
        pthread_mutex_unlock(&mutex_table);
        printf("\n#############   Thread %d started ######################\n", thread_id + 1);
        // execute query
        switch (query_type)
        {
        case 0: // inquire available seats
            pthread_rwlock_rdlock(&events[event_num].rwlock);
            printf("Thread %d: Inquiring available seats for Event %d - %d seats available\n", thread_id + 1, event_num, events[event_num].available_seats);
            pthread_rwlock_unlock(&events[event_num].rwlock);
            break;
        case 1: // book tickets
            pthread_rwlock_wrlock(&events[event_num].rwlock);
            if (events[event_num].available_seats >= booking_capacity)
            {
                events[event_num].available_seats -= booking_capacity;
                threaddetail[thread_id].event_no[event_num] = 1;
                threaddetail[thread_id].no_seat_booked[event_num] = booking_capacity;
                threaddetail[thread_id].thread_id = thread_id;
                printf("Thread %d: Booked %d seats for Event %d - %d seats remaining\n", thread_id + 1, booking_capacity, event_num, events[event_num].available_seats);
            }
            else
            {
                printf("Thread %d: Unable to book %d seats for Event %d - %d seats remaining\n", thread_id + 1, booking_capacity, event_num, events[event_num].available_seats);
            }
            pthread_rwlock_unlock(&events[event_num].rwlock);
            break;
        case 2: // cancel a booked ticket
            pthread_rwlock_wrlock(&events[event_num].rwlock);
            if (threaddetail[thread_id].event_no[event_num] == 1 && threaddetail[thread_id].no_seat_booked[event_num] >= cancel_ticket)
            {
                events[event_num].available_seats += cancel_ticket;
                threaddetail[thread_id].no_seat_booked[event_num] -= cancel_ticket;
                if (threaddetail[thread_id].no_seat_booked[event_num] == 0)
                    threaddetail[thread_id].event_no[event_num] = 0;

                printf("Thread %d: Cancelled %d seats for Event %d - %d seats remaining\n", thread_id + 1, cancel_ticket, event_num, events[event_num].available_seats);
            }
            else
            {
                printf("Thread %d: Unable to cancel %d seats for Event %d - %d seats remaining\n", thread_id + 1, cancel_ticket, event_num, threaddetail[thread_id].no_seat_booked[event_num]);
            }
            pthread_rwlock_unlock(&events[event_num].rwlock);
            break;
        case 3: // get number of available seats for an event
            pthread_rwlock_rdlock(&events[event_num].rwlock);
            printf("Thread %d: Event %d has %d available seats\n", thread_id + 1, event_num, events[event_num].available_seats);
            pthread_rwlock_unlock(&events[event_num].rwlock);
            break;
        default:
            printf("Thread %d: Invalid operation code %d\n", thread_id + 1, query_type);
            break;
        }
        shared_table[idx].event = -1;
    }
    printf("Thread exited : %d\n", thread_id + 1);
    pthread_exit(NULL);
}

int main()
{
    int thread_args[NUM_THREADS];
    for (int i = 0; i < MAX_ACTIVE_QUERIES; i++)
    {
        shared_table[i].event = -1;
    }
    srand(time(NULL)); // Initialize events
    for (int i = 0; i < NUM_EVENTS; i++)
    {
        events[i].available_seats = AUDITORIUM_CAPACITY;
        pthread_rwlock_init(&events[i].rwlock, NULL);
    }

    // Create threads to perform operations on events
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        for (int a = 0; a < NUM_EVENTS; a++)
        {
            threaddetail[i].event_no[a] = 0;
            threaddetail[i].no_seat_booked[a] = 0;
        }
        threaddetail[i].thread_id = i;
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, worker_thread_func, (void *)&thread_args[i]);
    }

    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf("*******************************     RUNNING COMPLETE          ##############################################################\n");
    // Clean up
    for (int i = 0; i < NUM_EVENTS; i++)
    {
        printf("        Available seat of event %d : %d\n", i + 1, events[i].available_seats);
        pthread_rwlock_destroy(&events[i].rwlock);
    }
    return 0;
}
