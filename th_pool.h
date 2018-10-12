/*
 * th_pool.h
 *
 *  Created on: 31 May 2018
 *      Author: madya
 */

#ifndef TH_POOL_H_
#define TH_POOL_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

/*
 * The following thread pool framework ideas
 * are derived from the information provided by the following link:
 * https://blog.csdn.net/hubi0952/article/details/8045094
 *
 */

typedef struct result_list result_List;

#include "supergraph.h"

/*
 * A queue of work that needs the thread to be done
 */
typedef struct worker
{
    void *(*new_work) (void *arg);
    void *arg;
    struct worker *next_work;

} Work_list;

/*
 * The struct for the thread_pool
 */
typedef struct thread_pool
{
    pthread_mutex_t queue_lock;
    pthread_mutex_t main_lock;

    pthread_cond_t queue_ready;
    pthread_cond_t main_ready;

    Work_list *work_queue_header;
    Work_list *work_queue_tail;

    bool shutdown;

    pthread_t *thread_id;
    int thread_num;
    int work_number;

} POOL;

/*
 * Struct of the arguments passing through threads for finding shortest path
 * for the function needs to be called
 */
typedef struct find_shortest_path_arg{
	char request_function;
	user* users;
	user_post_detail* user_post_details;
	post* posts;
	user* given_user;

	size_t count;
	size_t count_users;
	criteria* crit;
	map_origin * bool_origin_post;

	int userA;
	int userB;
	int number_of_robots;

	int post;
	void* resturn_result;
}shortest_path_arg;


void init_pool(int n_threads);
int add_work (void *(*process) (void *arg), void *arg);
void *thread_control (void *arg);
void *passing_args_to_functions (void *arg);
void pool_wait();
#endif /* TH_POOL_H_ */
