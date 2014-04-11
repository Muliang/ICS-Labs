/*
 * 18-213 proxy lab
 * Name: Yichao Xue
 * ID: yichaox
 *
 * cache.c: cache related functions include:
 * 1: initialize cache
 * 2: insert a new cache entry
 * 3: move a cache entry to the front of the list
 * 4: evict Least Recently Used cache entry
 * 5: search a cache entry
 * 6: empty cache
 */

#include "csapp.h"
#include "cache.h"

cache_entry *head;

/* initialize cache */
void init_cache(){
	/* allocate memory */
	if((head = Malloc(sizeof(cache_entry))) == NULL ){
		unix_error("Initialization failed");
	}

	head->request = NULL;
	head->content = NULL;
	head->prev = head;
	head->next = head;
	head->size = 0;
}

/* insert a new cache entry to list */
void insert_cache_entry(char *request, char *content, size_t size){
	cache_entry *new_entry;

	//allocate memory 
	if((new_entry = Malloc(sizeof(cache_entry))) == NULL ){
		unix_error("allocate memory failed");	
	}

	if((new_entry->request = Malloc(strlen(request) + 1)) == NULL ){
		unix_error("allocate memory failed");	
	}

	if((new_entry->content = Malloc(size)) == NULL ){
		unix_error("allocate memory failed");	
	}

	memcpy(new_entry->content, content, size);
	strcpy(new_entry->request, request);
	new_entry->size = size;
	new_entry->prev = NULL;
	new_entry->next = NULL;

	while(MAX_CACHE_SIZE < (head->size + size)){
		evict();
	}
	//insert new cache entry to front of the list
	(head->next)->prev = new_entry;
	new_entry->prev = head;
	new_entry->next = head->next;
	head->next = new_entry;
	//update size in head
	head->size += size;
}

/* move a recently accessed cache to front of the list */
void move2front(cache_entry *entry){
	if(entry == NULL){
		unix_error("point is NULL");
	}

	(head->next)->prev = entry;
	entry->prev = head;
	entry->next = head->next;
	head->next = entry;

	(entry->next)->prev = entry->prev;
	(entry->prev)->next = entry->next;

}

/* evict Least Recently Used cache entry
 *(always at the end of the list) 
 */
void evict(){
	cache_entry *tail = head->prev;

	if(tail == head){
		return;
	}

	(tail->next)->prev = tail->prev;
	(tail->prev)->next = tail->next;

	head->size = head->size - tail->size;

	Free(tail->content);
	Free(tail->request);
	Free(tail);

	return;
}

/* find a cached entry by request */
cache_entry *is_cached(char *request){
	cache_entry *cursor;

	for(cursor = head->next; cursor != head; cursor = cursor->next){
		if(!strcmp(request, cursor->request)){
			return cursor;
		}
	}
	return NULL;
}

/* empty the cache*/
void empty_cache(){
	cache_entry *cursor = head->next;
	cache_entry *temp;

	while(cursor != head){
		temp = cursor->next;

		Free(cursor->content);
		Free(cursor->request);
		Free(cursor);

		cursor = temp;
	}

	Free(head);
}
