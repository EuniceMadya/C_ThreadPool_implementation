/*
 * th_pool.c
 *
 *  Created on: 31 May 2018
 *      Author: madya
 */

/*
 * The following thread pool framework ideas
 * are derived from the information provided by the following link:
 * https://blog.csdn.net/hubi0952/article/details/8045094
 *
 */

#include "th_pool.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include "supergraph.h"

POOL * pool = NULL;
query_helper * rst_head;
query_helper * rst_current_ptn;

extern user_queue_detail * find_shortest_path(user* users, size_t count,int userA, int userB);
//extern bool original_identify(post* posts, size_t count, uint64_t post_id, query_helper* helper);

/*
 * This function is for initialize the thread pool
 * with given number of threads that can be used
 * @param n_threads
 *
 */
void init_pool(int n_threads){
	pool = (POOL*)malloc(sizeof(POOL));
	pool->shutdown = false;
	pool->thread_num = n_threads;
	pool->thread_id = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);
	pool->work_number = 0; // No work yet
	pool->work_queue_header = NULL; //both list's head and tail is NULL now
	pool->work_queue_tail = NULL;

	pthread_mutex_init(&pool->queue_lock, NULL);
	pthread_mutex_init(&pool->main_lock, NULL);

	pthread_cond_init(&pool->queue_ready, NULL);
	pthread_cond_init(&pool->main_ready, NULL);
	rst_head =(query_helper*) malloc(sizeof(query_helper));
	rst_current_ptn = rst_head;
	rst_head->next = NULL;

	for(int i = 0; i < pool->thread_num; i ++){
        pthread_create (&(pool->thread_id[i]), NULL, thread_control,NULL);
	}


}

/*
 * This function is for exiting all the threads
 * and free the pool
 * @param n_threads
 *
 */
void clean_pool(){
	/*
	 * If the pool has already been shut down
	 * then do nothing,
	 * otherwise shut down the pool
	 */
	if(pool->shutdown == true){
		return;
	}
	pool->shutdown = true;

	/*
	 * Letting all the thread be ready to exit
	 */
    pthread_cond_broadcast(&(pool->queue_ready));
    for(int i = 0; i < pool->thread_num; i ++){
    		pthread_join(pool->thread_id[i], NULL);
    }
    query_helper * temp_node;

    temp_node = rst_head;

    while(temp_node->next != NULL){
		temp_node = temp_node->next;
    		free(rst_head);
    		rst_head = temp_node;
    }
    pthread_mutex_destroy(&(pool->main_lock));
    pthread_mutex_destroy(&(pool->queue_lock));

    free(temp_node);
    free(pool->thread_id);
    free(pool);
}

/*
 * This function is for adding the work
 * to the end of the work query the pool has
 * @param process
 * @param arg
 *
 * return 0
 */
int add_work (void *(*process) (void *arg), void *arg){

	/*
	 * Adding work to the query
	 */
    pthread_mutex_lock (&(pool->queue_lock));

	if(pool->work_number == 0){
		pool->work_queue_header = (Work_list*)malloc(sizeof(Work_list));
		pool->work_queue_tail = pool->work_queue_header;
	}else{
		pool->work_queue_tail->next_work = (Work_list*)malloc(sizeof(Work_list));
		pool->work_queue_tail = pool->work_queue_tail->next_work;
	}
	pool->work_queue_tail->next_work = NULL;
	pool->work_number ++;

	pool->work_queue_tail->arg = arg;
	pool->work_queue_tail->new_work = process;

    pthread_mutex_unlock (&(pool->queue_lock));
    pthread_cond_signal (&(pool->queue_ready));

	return 0;
}

/*
 * This function will be called
 * for executing the work in the query
 *
 * @param arg
 *
 * return NULL
 */
void *thread_control (void *arg){
	while(1){

		pthread_mutex_lock (&(pool->queue_lock));
		/*The thread will wait till there's actually work needs to be executed*/
		while (pool->work_number == 0 && (pool->shutdown == false)){
		    pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
		}
		/*
		 * The thread will be exiting through this,
		 * if pool is shut down
		 */
		if(pool->shutdown == true){
			pthread_mutex_unlock(&(pool->queue_lock));
			pthread_exit(NULL);
		}

		/*
		 * Dequeue what work is at the front of the queue and execute it
		 */
		if(pool->work_queue_header != NULL && pool->work_number > 0){
			Work_list * current_work = pool->work_queue_header;
			pool->work_queue_header = pool->work_queue_header->next_work;

			 (*(current_work->new_work)) (current_work->arg);
		    pthread_mutex_unlock (&(pool->queue_lock));

			free(current_work);

			if(pool->work_number == 0){
				pthread_cond_signal(&(pool->main_ready));
			}

		}

	}
	return NULL;
}
/*
 * The function will be called using function pointer
 * That will specify and execute what helper method needed
 * to be executed with multi threads
 */
void *passing_args_to_functions (void *arg){

	shortest_path_arg* argv = (shortest_path_arg*)arg;
	/*All the results will be put in the result list struct as a list*/

		rst_current_ptn->next = malloc(sizeof(query_helper));
		rst_current_ptn = rst_current_ptn->next;
		rst_current_ptn->next = NULL;

	if(argv->request_function == 'f'){
		//rst_current_ptn->rst_current = malloc(sizeof(List_Node));
		rst_current_ptn->rst_current = find_shortest_path(argv->users, argv->count_users,  argv->userA,  argv->userB);
		pool->work_number--;

	}
	if(argv->request_function == 'N'){
		rst_current_ptn->is_bot_list = finding_for_nondiscrete_robots(argv->users, argv->crit, argv->userA, argv->userB, argv->bool_origin_post);
		pool->work_number--;
	}
	if(argv->request_function == 'D'){
		rst_current_ptn->is_bot_list = finding_discrete_bots(argv->users, argv->user_post_details, argv->crit, argv->userA, argv->userB);

		pool->work_number--;
	}

	return NULL;
}

/*
 * A function that will be called in supergraph(main thread)
 * To let the main thread waiting for
 * the thread pool finishing executing the function
 */
void pool_wait(){
	if(pool->shutdown == true){
		pthread_mutex_unlock(&(pool->main_lock));
		pthread_exit(NULL);
	}
	while(pool->work_number != 0){
	    pthread_cond_wait (&(pool->main_ready), &(pool->main_lock));
	}
	pthread_mutex_unlock (&(pool->main_lock));
}
