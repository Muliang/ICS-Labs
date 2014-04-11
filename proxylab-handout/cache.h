/*
 * header file for cache.c
 */

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_entry{
	char *request;
	char *content;
	size_t size;
	struct cache_entry *prev;
	struct cache_entry *next;
} cache_entry;

void init_cache();
void insert_cache_entry(char *request, char *content, size_t size);
void move2front(cache_entry *entry);
void evict();
cache_entry *is_cached(char *request);
void empty_cache();