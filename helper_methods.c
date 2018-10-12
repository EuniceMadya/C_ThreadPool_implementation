#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "supergraph.h"
#include "th_pool.h"
#include "th_pool.c"



/**
 * The helper function used to find which post the given post
 * is reposting from
 *
 * @param users
 * @param count
 * @param userA
 * @param userB
 *
 * @return the shortest path from the given userA to userB
 * or itself if it's already the original post
 */
user_queue_detail * find_shortest_path(user* users, size_t count,int userA_index, int userB_index){

	user_queue_detail* user_queue_details;
	user_queue_details = malloc(sizeof(user_queue_detail)* (count+1));

	int* user_list = malloc(sizeof(int)*(count+1));

	for(int i = 0; i < count+1; i ++){
		user_queue_details[i].seen = false;
		user_queue_details[i].front = -1;
		user_list[i] = -1;
	}

	int current_visiting_user = userA_index;
	int current_visiting_index_userlist = 0;

	user_queue_details[userA_index].seen = true;
	user_list[0] = userA_index;
	int temp_index_in_loop = userA_index;
	int counter = 0;
	int distance = 0;
	while(current_visiting_user != -1){
		for(int i = 0; i < users[current_visiting_user].n_following; i++){

			temp_index_in_loop = users[current_visiting_user].following_idxs[i];
			if(	user_queue_details[temp_index_in_loop].seen == true){
				continue;
			}
			user_queue_details[temp_index_in_loop].seen = true;
			user_queue_details[temp_index_in_loop].front = current_visiting_user;
			counter ++;
			user_list[counter] = temp_index_in_loop;
			//printf("temp index is %d\n",temp_index_in_loop);
			if(temp_index_in_loop == userB_index){
				user_queue_details[userB_index].seen = true;
				user_queue_details[userB_index].front = current_visiting_user;

				break;
			}


		}
		if(temp_index_in_loop == userB_index){
			user_queue_details[temp_index_in_loop].seen = true;
			break;
		}
		current_visiting_index_userlist++ ;
		current_visiting_user =user_list[current_visiting_index_userlist];
	}
	if(user_queue_details[userB_index].seen == false){
		free(user_list);
		user_queue_details[userA_index].front =distance + 1;
		return user_queue_details;
	}
	distance = 0;
	int i_val = userB_index;
	while(1){
		i_val = user_queue_details[i_val].front;
		distance ++;
		if(i_val == userA_index){
			break;
		}
	}
	user_queue_details[userA_index].front = distance;

	free(user_list);
	return user_queue_details;

}





result * find_bots_thread(user* users, size_t user_count, post* posts, size_t post_count, criteria* crit, map_origin * bool_origin_post, size_t n_threads) {
	result * rst;
	rst = malloc(sizeof(result));
	rst->n_elements = 0;
	rst->elements = malloc(sizeof(user)*user_count);

	shortest_path_arg * args_for_threads = malloc(sizeof(shortest_path_arg)* n_threads);
	int block_size = user_count / n_threads;
	query_helper *temp_node = rst_current_ptn;
	for(int i = 0; i < n_threads; i ++){
		args_for_threads[i].request_function = 'N';
		args_for_threads[i].crit = crit;
		args_for_threads[i].users = users;
		args_for_threads[i].userA = i*block_size;
		args_for_threads[i].bool_origin_post = bool_origin_post;
		if(i == n_threads - 1){
			args_for_threads[i].userB = user_count - i * block_size;

			add_work(passing_args_to_functions, &args_for_threads[i]);
			continue;
		}
		args_for_threads[i].userB =block_size;
		add_work(passing_args_to_functions, &args_for_threads[i]);
	}
    pool_wait();
    int counter = 0;
	user_post_detail* user_post_details =  malloc(sizeof(user_post_detail)*user_count); /* The users with the number of their repos */
	bool* reslt_from_thread;
	int index = 0;
    while(index < n_threads){
        temp_node = temp_node->next;

    	reslt_from_thread = (bool*)temp_node->is_bot_list;
    	for(int i = counter; i < counter + args_for_threads[index].userB; i ++){
   			user_post_details[i].bot = reslt_from_thread[i-counter];
    		if(user_post_details[i].bot == true){
    			rst->n_elements ++;
    			rst->elements[rst->n_elements -1] = &users[i];
   			}
   		}
    	counter +=  args_for_threads[index].userB;
    	index ++;
    free(temp_node->is_bot_list);
    }
    //free(temp_node->is_bot_list);
	if(rst->n_elements == 0){
    	free(rst->elements);
    	rst->elements = NULL;
   		free(user_post_details);
		free(args_for_threads);

   		rst->n_elements= 0;
    	return rst;
    }
    for(int i = 0; i < n_threads; i ++){
    		args_for_threads[i].user_post_details = user_post_details;
    		args_for_threads[i].request_function = 'D';
    }

	bool more_bots = true;

	while(more_bots == true){
		more_bots = false;
		for(int it = 0; it < n_threads; it ++){
			add_work(passing_args_to_functions, &args_for_threads[it]);
		}
	    pool_wait();


    		index = 0;
    		counter = 0;
    		while(index < n_threads){
    			temp_node = temp_node->next;
    			reslt_from_thread = (bool*)temp_node->is_bot_list;
    			for(int i = 0; i <  args_for_threads[index].userB; i ++){
    				if(user_post_details[i + counter].bot == false){
    		    			user_post_details[i + counter].bot = reslt_from_thread[i];
    		    			if(user_post_details[i + counter].bot == true){
    		    				more_bots = true;
    		    				rst->n_elements ++;
    		    				rst->elements[rst->n_elements -1] = &users[i + counter];
    		    			}
    				}
    		    	}
    		    	counter +=  args_for_threads[index].userB;
    		    	index ++;
    		    free(temp_node->is_bot_list);
    		 }
	}





    free(user_post_details);
    free(args_for_threads);
	rst->elements = realloc(rst->elements, sizeof(user)*rst->n_elements);

	return rst;
}

bool* finding_discrete_bots(user*users, user_post_detail* user_post_details, criteria*crit, int starting_point, int length){
	bool* result = malloc(sizeof(bool)*length);
	float th_bot = 0;
	size_t following_bots_num = 0;

	for(int iu = 0; iu < length; iu ++){
		result[iu] = false;
		following_bots_num = 0;
		if(user_post_details[iu + starting_point].bot == false){
			for(int ic = 0; ic < users[iu + starting_point].n_followers; ic ++){
				if(user_post_details[users[iu + starting_point].follower_idxs[ic]].bot == true){
					 following_bots_num ++;
				}
			}
			th_bot = ((float)(users[iu + starting_point].n_followers)) * crit->bot_net_threshold;
			if(th_bot < (float)following_bots_num){
				result[iu]= true;
			}
		}else{
			result[iu] = true;
		}
	}


	return result;
}

bool* finding_for_nondiscrete_robots(user* users, criteria* crit, int start_point, int length,map_origin* post_details){
	bool* result = malloc(sizeof(bool)*length);
	user_post_detail* user_post_details =  malloc(sizeof(user_post_detail)*length); /* The users with the number of their repos */
	float repo_th;
	float flw_th;
	bool met_repo_crit = false; /* The original post for each post in each user*/
	bool met_flw_crit = false;

	for(int iu = 0; iu < length ; iu ++){

		met_repo_crit = false;
		met_flw_crit = false;
		result[iu] = false;
		user_post_details[iu].bot = false;
		user_post_details[iu].number_of_post = users[iu + start_point].n_posts;
		user_post_details[iu].number_of_repo = 0;
		/*find the repo number for each users*/
		for(int ip = 0; ip < user_post_details[iu].number_of_post; ip ++){
			if(post_details[users[iu+start_point].post_idxs[ip]].is_origin== false){
				user_post_details[iu].number_of_repo += 1;
			}
		}

		repo_th = crit->oc_threshold * (float)(users[iu + start_point].n_posts);
		flw_th = crit->acc_rep_threshold * (((float)(users[iu + start_point].n_followers) + (float)(users[iu + start_point].n_following)));

		if(repo_th < (float)(user_post_details[iu].number_of_repo)){
			met_repo_crit = true;
		}
		if(flw_th > (float)(users[iu + start_point].n_followers)){
			met_flw_crit = true;
		}

		if((met_repo_crit == true ||  met_flw_crit == true)){
			result[iu] = true;
		}
	}
	free(user_post_details);
	return result;
}
