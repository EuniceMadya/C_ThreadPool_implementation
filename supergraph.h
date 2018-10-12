#ifndef SUPERGRAPH_H
#define SUPERGRAPH_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct query_helper query_helper;
typedef struct result result;
typedef struct criteria criteria;
typedef struct user user;
typedef struct post post;

typedef struct list_helper List_Node;
typedef struct map_helper map;
typedef struct map_origin map_origin;
typedef struct user_post_detail user_post_detail;
typedef struct user_queue_detail user_queue_detail;
//typedef struct thread_arg THRD_ARG;

/*
 * The list that stores all the result
 * returned by the function called in threads
 *
 */
struct query_helper {
	int dummy_field;
	user_queue_detail* rst_current;
	struct query_helper* next;
	int value;
	float user_threshold;
	bool bool_value;
	bool* is_bot_list;
};

//A helper struct for storing the result etc
struct list_helper {
	int currentVal; // Normally for recording the ids
	List_Node * next;
	List_Node * front;
};

//A helper struct for finding robots with the (repo) criteria
struct user_post_detail{
	int number_of_post;
	int number_of_repo;
	bool bot;
};


struct user_queue_detail{
	int front;
	bool seen;
};

//A helper struct for find shortest link between two users
struct map_helper{
	int value;
};

struct map_origin{
	bool is_origin;
};


/*  The structs below are from the scafold and cannot be changed */
struct result {
	void** elements;
	size_t n_elements;
};

struct criteria {
	float oc_threshold;
	float acc_rep_threshold;
	float bot_net_threshold;
};

struct user {
	uint64_t user_id;
	size_t* follower_idxs;
	size_t n_followers;
	size_t* following_idxs;
	size_t n_following;
	size_t* post_idxs;
	size_t n_posts;
};

struct post {
	uint64_t pst_id;
	uint64_t timestamp;
	size_t* reposted_idxs;
	size_t n_reposted;
};



query_helper* engine_setup(size_t n_processors);

result* find_all_reposts(post* posts, size_t count, uint64_t post_id, query_helper* helper);

result* find_original(post* posts, size_t count, uint64_t post_id, query_helper* helper);
//int originPost_of_Repo(post* posts, size_t count, int cou);// helper method
//int originPost_of_Repo_helper(post* posts,int given_index, int target_index);

result* shortest_user_link(user* users, size_t count, uint64_t userA, uint64_t userB, query_helper* helper);
user_queue_detail * find_shortest_path(user* users, size_t count,int userA, int userB);
//helper method

result* find_bots(user* users, size_t user_count, post* posts, size_t post_count, criteria* crit, query_helper* helper);
//helper method
result * find_bots_thread(user* users, size_t user_count, post* posts, size_t post_count, criteria* crit, map_origin * bool_origin_post,size_t n_threads);
//helper method
bool* finding_for_nondiscrete_robots(user* users, criteria* crit, int start_point, int length, map_origin* post_details);
bool* finding_discrete_bots(user*users, user_post_detail* user_post_details, criteria*crit, int starting_point, int length);



void engine_cleanup(query_helper* helpers);

#endif
