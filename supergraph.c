#include "supergraph.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "th_pool.h"
#include "helper_methods.c"

extern query_helper * rst_current_ptn;
extern query_helper * rst_head;

extern int add_work (void *(*process) (void *arg), void *arg);
extern void *passing_args_to_functions (void *arg);
extern void init_pool();

extern void clean_pool();
extern void pool_wait();
extern void init_pool(int n_threads);

bool bool_pool_on;
size_t n_threads;

/**
 * The function used to set up a thread pool
 * @param n_processors
 * @return
 */
query_helper* engine_setup(size_t n_processors) {
	bool_pool_on = false;
	if(n_processors > 1){
		init_pool(n_processors);
		bool_pool_on = true;
	}
	n_threads = n_processors;
	return NULL;
}

/**
 * The function used to find all repo for one single post
 *
 * @param posts
 * @param counts
 * @param post_id
 * @param heper
 *
 * @return the result of all the repos
 */
result* find_all_reposts(post* posts, size_t count, uint64_t post_id, query_helper* helper) {
	// the result which needs to be returned
	result *rst;
	rst = (result*)malloc(sizeof(result));

	/*
	 * Find the post with the post id givin
	 * if there's no such post, return NULL query
	*/
	int post_index = 0;
	bool contain_bool = false;
	for(post_index = 0; post_index < count; post_index ++){
		if(posts[post_index].pst_id == post_id){
			contain_bool = true;
			break;
		}
	}
	if(contain_bool == false){
		rst->n_elements = 0;
		rst->elements = NULL;
		return rst;
	}


	/*
	 * If there's no repo for this original post
	 * then return a NULL query
	 */
	if(posts[post_index].n_reposted == 0){
		rst->n_elements = 1;
		rst->elements = malloc(sizeof(post));
		rst->elements[0] = &posts[post_index];
		return rst;
	}

	/*
	* initialize the result,
	* and allocate the memory with the size of the number of its derect repo
	*/
	rst->n_elements = 0;
	rst->elements = malloc(sizeof(post)* count);

	/*
	 *A helper of a list to store repo
	 *of which the repo has not yet been included
	 *points to the repo currently visiting
	 */
	List_Node * list_cur;
	/*initialize the head of the list*/
	list_cur = (List_Node*)malloc(sizeof(List_Node));
	list_cur->currentVal = post_index;
	list_cur->front = NULL;

	/*keep tracking of the tail the list*/
	List_Node* current_tail;
	current_tail = list_cur;

	rst->n_elements = 1;
	rst->elements[0] = &posts[post_index];
	/*find all repo in this loop*/
	while(list_cur != NULL){
		/*
		 * If the post currently visiting has any repo
		 * then add all the repo to the end of the helper list
		 */
		if(posts[list_cur->currentVal].n_reposted != 0){
			for(int i = 0; i < posts[list_cur->currentVal].n_reposted; i ++){
				rst->elements[rst->n_elements] = &posts[posts[list_cur->currentVal].reposted_idxs[i]];
				rst->n_elements ++;

				//Linking new posts to the end of the list
				current_tail->next = (List_Node*)malloc(sizeof(List_Node));
				current_tail->next->front = current_tail;
				current_tail = current_tail->next;
				current_tail->currentVal = posts[list_cur->currentVal].reposted_idxs[i];

			}
			current_tail->next = NULL;
		}

		/*
		 * If there's no further posts in the list
		 *that hasn't been visited or has no repos
		 */
		if(list_cur->next == NULL){
			break;
		}else{
			list_cur = list_cur->next;
			free(list_cur->front);
		}
	}
	free(list_cur);
	rst->elements = realloc(rst->elements, sizeof(post)* (rst->n_elements));

	return rst;
}
/**
 * The function used to find the original post for the given post
 *
 * @param posts
 * @param count
 * @param post_id
 * @param helper
 *
 * @return the result the original post
 */
result* find_original(post* posts, size_t count, uint64_t post_id, query_helper* helper) {

	/*
	 * the result which needs to be returned
	 * and allocate a space for it
	 */
	result *rst;
	rst = (result*)malloc(sizeof(result));

	bool contain_bool = false;
	int post_index = 0;

	/*
	 * Find whether the post exists or not
	 * return NULL query if there's no such post
	 */
	for(post_index = 0; post_index < count; post_index ++){
		if(posts[post_index].pst_id == post_id){
			contain_bool = true;
			break;
		}
	}
	if(contain_bool == false){
		rst->n_elements = 0;
		rst->elements = NULL;
		return rst;
	}
	user_queue_detail * post_details;
	post_details = malloc(sizeof(post)* count);
	int* post_list = malloc(sizeof(int)*count);

	for(int ip = 0; ip <count; ip ++ ){
		post_details[ip].seen = true;
	}
	for(int ip = 0; ip < count; ip ++){
		for(int iii = 0; iii < posts[ip].n_reposted; iii ++){
			post_details[posts[ip].reposted_idxs[iii]].seen = false;
		}
	}
	for(int ipp = 0; ipp < count; ipp ++){
		post_details[ipp].front = -1;
		if(post_details[ipp].seen == true && ipp == post_index){
			rst->elements = malloc(sizeof(post));
			rst->elements[0] = &posts[ipp];
			rst->n_elements = 1;
			free(post_details);
			free(post_list);
			return rst;
		}
	}
	int total_repo = 0;
	for(int ipp = 0; ipp < count; ipp ++){
		if(post_details[ipp].seen == false){
			continue;
		}
		int ij = 0;
		int counter = 0; //how many repo adding up
		int current_post_index = ipp; //index current visiting post
		int index_in_post_list = 0; //index of current visiting post in the post_list
		post_list[counter] = ipp; //header should always be the original post
		total_repo = posts[ipp].n_reposted;
		while(counter < total_repo){
			for(ij = 0; ij < posts[current_post_index].n_reposted; ij ++){
				counter ++;
				post_list[counter] = posts[current_post_index].reposted_idxs[ij];
				total_repo += posts[posts[current_post_index].reposted_idxs[ij]].n_reposted;
				post_details[posts[current_post_index].reposted_idxs[ij]].front = ipp;
			}
			index_in_post_list ++;
			current_post_index = post_list[index_in_post_list];
		}
	}


	rst->elements = malloc(sizeof(post));
	rst->elements[0] = &posts[post_details[post_index].front];
	rst->n_elements = 1;
	free(post_details);
	free(post_list);

	return rst;
}

/**
 * The function used to find the original post for the given post
 *
 * @param users
 * @param count
 * @param userA
 * @param userB
 * @param helper
 *
 * @return the shortest path if there's a path between A and B
 */
result* shortest_user_link(user* users, size_t count,uint64_t userA, uint64_t userB, query_helper* helper){


//	printf("\n\nnew order here need to be done\n");
	result * rst;
	rst = (result*)malloc(sizeof(result));

	/**
	 * Find out whether the users array contains both users
	 */
	if(userA == userB){
		rst->elements = NULL;
		rst->n_elements = 0;
		return rst;
	}

	bool A_contain = false;
	bool B_contain = false;
	int couA;
	int couB;

	for(couA = 0; couA < count; couA ++){
		if(users[couA].user_id == userA){
			A_contain = true;
			break;
		}
	}
	for(couB = 0; couB < count; couB ++){
		if(users[couB].user_id == userB){
			B_contain = true;

			break;
		}
	}
	if(A_contain == false || B_contain == false){
		rst->elements = NULL;
		rst->n_elements = 0;
		return rst;
	}

	/**
	 * Call for helper function to find path both from userA and B
	 * and compare the value
	 */
	user_queue_detail* rstA_to_B ;
	user_queue_detail * rstB_to_A ;

	if(bool_pool_on == true && count > 10000){
		shortest_path_arg * pathA_TO_B = malloc(sizeof(shortest_path_arg));

		pathA_TO_B->request_function = 'f';
		pathA_TO_B->users = users;

		pathA_TO_B->count_users = count;
		pathA_TO_B->userA = couA;
		pathA_TO_B->userB = couB;

		shortest_path_arg * pathB_TO_A = malloc(sizeof(shortest_path_arg));
		pathB_TO_A->request_function = 'f';
		pathB_TO_A->users = users;
		pathB_TO_A->count_users = count;
		pathB_TO_A->userA = couB;
		pathB_TO_A->userB = couA;
//		begin = clock();
//		time_elapsed = (double)(begin) / CLOCKS_PER_SEC;
		add_work(passing_args_to_functions, pathA_TO_B);

		add_work(passing_args_to_functions, pathB_TO_A);

	    pool_wait();
		free(pathB_TO_A);
		free(pathA_TO_B);
		query_helper *temp_node = rst_head;

		while(temp_node->next != rst_current_ptn ){
			temp_node = temp_node->next;
		}


		rstA_to_B = (temp_node->rst_current);

		rstB_to_A = (rst_current_ptn->rst_current);

	}else{

		rstA_to_B = find_shortest_path(users, count, couA, couB);
		rstB_to_A = find_shortest_path(users, count, couB, couA);

	}


	if(rstA_to_B[couB].seen == false && rstB_to_A[couA].seen == false){
		free(rstA_to_B);
		free(rstB_to_A);
		rst->elements = NULL;
		rst->n_elements = 0;
		//printf("no path!!\n");
		return rst;
	}
	//printf("at least one path exitst\n");

	int final_min_distance = 0;
	int AB_distance = rstA_to_B[couA].front;
	int BA_distance = rstB_to_A[couB].front;



	int i_value = 0;
	if( BA_distance>= AB_distance){
		final_min_distance = AB_distance + 1;
	//	printf("the final distance is %d\n", final_min_distance);
	//	printf("the starting point is %d\n", couA);
		rst->elements = malloc(sizeof(user)*(final_min_distance));
		rst->n_elements = final_min_distance;
		int i = couB;
		i_value = final_min_distance - 1;
		while(i_value != -1){
			rst->elements[i_value] = &users[i];
			i = rstA_to_B[i].front;
			i_value --;
		}
	}else if (AB_distance > BA_distance){
		final_min_distance = BA_distance + 1;
		rst->elements = malloc(sizeof(user)*(final_min_distance));
		rst->n_elements = final_min_distance;

		int i = couA;
		i_value = final_min_distance - 1;
		while(i_value > -1){
			rst->elements[i_value] = &users[i];
			i = rstB_to_A[i].front;
			i_value --;
		}
	}
	free(rstB_to_A);
	free(rstA_to_B);
	return rst;
}


/**
 * The function is for finding all the robot users
 *
 * @param users
 * @param user_count
 * @param posts
 * @param post_count
 * @param crit
 * @param helper
 *
 * @return the robot list if there is any
 */
/**
 * The function is for finding all the robot users
 *
 * @param users
 * @param user_count
 * @param posts
 * @param post_count
 * @param crit
 * @param helper
 *
 * @return the robot list if there is any
 */
result* find_bots(user* users, size_t user_count, post* posts, size_t post_count, criteria* crit, query_helper* helper) {
	/*initialize the result */
	result * rst;
	map_origin * bool_origin_post = malloc(sizeof(map_origin)*post_count);
	for(int ip = 0; ip <post_count; ip ++ ){
		bool_origin_post[ip].is_origin = true;
	}
	for(int ip = 0; ip < post_count; ip ++){

		for(int iii = 0; iii < posts[ip].n_reposted; iii ++){
			bool_origin_post[posts[ip].reposted_idxs[iii]].is_origin = false;
		}
	}
	if(bool_pool_on ==  true && user_count > 10000){
		rst = find_bots_thread(users, user_count, posts, post_count, crit,bool_origin_post, n_threads);
		free(bool_origin_post);
		return rst;
	}

	rst = malloc(sizeof(result));
	rst->n_elements = 0;
	rst->elements = malloc(sizeof(user)*user_count);
	user_post_detail* user_post_details; /* The users with the number of their repos */
	bool met_repo_crit = false;
	bool met_flw_crit = false;
	float repo_th = 0;
	float flw_th = 0;
	float th_bot = 0;



	/*find obivious robots*/
	user_post_details = (user_post_detail*)malloc(sizeof(user_post_detail) * user_count);
	for(int iu = 0; iu < user_count; iu ++){ // iu is the index of the current user
		user_post_details[iu].bot = false;
		user_post_details[iu].number_of_post = users[iu].n_posts;
		user_post_details[iu].number_of_repo = 0;
		/*find the repo number for each users*/
		for(int ip = 0; ip < user_post_details[iu].number_of_post; ip ++){
			if(bool_origin_post[users[iu].post_idxs[ip]].is_origin== false){
				user_post_details[iu].number_of_repo += 1;
			}
		}

		repo_th = crit->oc_threshold * (float)(user_post_details[iu].number_of_post);

		flw_th = crit->acc_rep_threshold * (((float)(users[iu].n_followers) + (float)(users[iu].n_following)));

		if(repo_th < (float)(user_post_details[iu].number_of_repo)){
			met_repo_crit = true;
		}
		if(flw_th > (float)(users[iu].n_followers)){
			met_flw_crit = true;
		}

		if(!(met_repo_crit == false &&  met_flw_crit == false)){
			rst->n_elements ++;
			user_post_details[iu].bot = true;
			rst->elements[rst->n_elements -1] = &users[iu];
		}
		met_repo_crit = false;
		met_flw_crit = false;
	}
	if(rst->n_elements == 0){
		free(rst->elements);
		rst->elements = NULL;
		free(user_post_details);
		free(bool_origin_post);
		rst->n_elements= 0;
		return rst;
	}
	map * user_bot_flwng = malloc(sizeof(user)*user_count);

	bool more_bots = true;

	while(more_bots == true){
		more_bots = false;
		for(int iu = 0; iu < user_count; iu ++){
			user_bot_flwng[iu].value = 0;
			if(user_post_details[iu].bot == false){
				for(int ic = 0; ic < users[iu].n_followers; ic ++){
					if(user_post_details[users[iu].follower_idxs[ic]].bot == true){
						user_bot_flwng[iu].value++;
					}
				}
			}
		}


		for(int iu = 0 ; iu < user_count; iu ++){
			if(user_post_details[iu].bot == false){
				th_bot = ((float)(users[iu].n_followers)) * crit->bot_net_threshold;
				if(th_bot < (float)(user_bot_flwng[iu].value)){

					user_post_details[iu].bot= true;
					more_bots = true;
					rst->n_elements ++;
					rst->elements[rst->n_elements -1] = &users[iu];
				}
			}
		}
	}
	rst->elements = realloc(rst->elements, sizeof(user)*rst->n_elements);
	free(user_bot_flwng);
	free(bool_origin_post);
	free(user_post_details);
	return rst;
}


void engine_cleanup(query_helper* helpers) {
	if(bool_pool_on == true){
		clean_pool();
	}

}

