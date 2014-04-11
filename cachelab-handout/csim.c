/************************

	Name: Yichao Xue
	Andrew ID: yichaox

************************/

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* define state */
#define HIT 0
#define MISS_HIT 1
#define MISS 2
#define MISS_EVICTION_HIT 3
#define MISS_EVICTION 4
#define HIT_HIT 5

/* initialize struct 
 * define a line 
 */
typedef struct {
	int valid;
	int tag;
	int lruIndex;
} Line;

/* global variable */
int hit;
int miss;
int eviction;

/* function declaration */
Line** initCache(int s, int E);
int getTag(int addr, int s, int b);
int getSet(int addr, int s, int b);
int getArg(int argc, char **argv, int *verbose, int *ps, int *pE, int *pb, 
	char *traceFileName);
int lruCounter(Line** cache, int s, int E, int index);
int parseTraceFile(Line** cache, int s, int E, int b, char *lineBuffer);

/* main */
int main(int argc, char **argv)
{
	int i;
	int s, E, b;
	int verbose = 0;
	char traceFileName[64];
	Line** cache;
	FILE* traceFile;
	int state;
	char lineBuffer[64];

	getArg(argc, argv, &verbose, &s, &E, &b, traceFileName); 
	cache = initCache(s, E);
	//open trace file
	traceFile = fopen (traceFileName, "r");
	if (!traceFile){
		printf("%s: %s\n", traceFileName, "Error occured when opening file");
		exit(-1);
	}

	while(fgets(lineBuffer, 64, traceFile) != NULL){
		if(lineBuffer[0] == ' '){
			lineBuffer[strlen(lineBuffer) - 1] = '\0';
			state = parseTraceFile(cache, s, E, b, lineBuffer);
			if (verbose == 1)
			{
				switch(state){
					case HIT:
					printf("%1.1s %s hit\n", lineBuffer + 1, lineBuffer + 3);
					break;

					case MISS:
					printf("%1.1s %s miss\n", lineBuffer + 1, lineBuffer + 3);
					break;

					case MISS_HIT:
					printf("%1.1s %s miss hit\n", lineBuffer + 1, 
						lineBuffer + 3);
					break;

					case MISS_EVICTION:
					printf("%1.1s %s miss eviction\n", lineBuffer + 1, 
						lineBuffer + 3);
					break;

					case MISS_EVICTION_HIT:
					printf("%1.1s %s miss eviction hit\n", lineBuffer + 1, 
						lineBuffer + 3);
					break;

					case HIT_HIT:
					printf("%1.1s %s hit hit\n", lineBuffer + 1, 
						lineBuffer + 3);
					break;

					default:
					printf("%s\n", "Error!");
					break;
				}
			}
		}
	}
	//close file
	fclose(traceFile);
	/* free memory */
	for (i = 0; i < E; ++i)
	{
		free(cache[i]);
	}
	free(cache);

	printSummary(hit, miss, eviction);

	return 0;
}

/* initialize cache */
Line** initCache(int s, int E){
	int i, j;
/* allocate memory to cache */
	Line** cache = malloc((1 << s) * sizeof(Line *)); //set 
	//check for memory error
	if (!cache)
	{
		printf("%s\n", "allocate memory failed");
		exit(-1);
	}

	for (i = 0; i < (1 << s); i++){
		cache[i] = malloc(E * sizeof(Line)); //line
		//check for memory error
		if (!cache)
		{
			printf("%s\n", "allocate memory failed");
			exit(-1);
		}

		for(j = 0; j < E; j++){
			cache[i][j].valid = 0;   //initial value of valid
		}
	}
	return cache;
}

/* get tag from address */
int getTag(int addr, int s, int b){
	int mask = 0x7fffffffffffffff >> (s + b - 1);
	return ((addr >> (b + s)) & mask);
}

/* get tag from address */
int getSet(int addr, int s, int b){
	int mask = 0x7fffffffffffffff >> (63 - s);
	return ((addr >> b) & mask);
}

/* get command arguments */
int getArg(int argc, char **argv, int *verbose, int *ps, int *pE, int *pb,
 char *traceFileName){
	int arg;
	int argCount;
	while ((arg = getopt(argc, argv, "vs:E:b:t:")) != -1){
		switch (arg){
			case 'v':
			*verbose = 1;
			break;

			case 's':
			++argCount;
			*ps = atoi(optarg);
			break;

			case 'E':
			++argCount;
			*pE = atoi(optarg);
			break;

			case 'b':
			++argCount;
			*pb = atoi(optarg);
			break;

			case 't':
			++argCount;
			strcpy(traceFileName, optarg);
			break;

			default:
			printf("%s\n", "Illegal command arguments, please input again");
			exit(-1);
			break;
		}
	}

	if(argCount < 4){
		printf("%s\n", "Illegal command arguments, please input again");
			exit(-1);
	}
	return 0;
}

/* LRU counter: determine which line to be replace */
int lruCounter(Line** cache, int s, int E, int index){   
	int i;
	for (i = 0; i < E; i++)
	{
		if((cache[s][i].valid == 1) && 
			(cache[s][i].lruIndex > cache[s][index].lruIndex))
		{
			--cache[s][i].lruIndex;
		}
	}
	cache[s][index].lruIndex = E - 1;
	return 0;
}

/* parse trace file */
int parseTraceFile(Line** cache, int s, int E, int b, char *lineBuffer){
	char opt;
	int addr;
	int tag;
	int set; //selected tag and set
	int i;

	sscanf(lineBuffer, " %c %x", &opt, &addr);
	//get selected tag and set
	tag = getTag(addr, s, b);
	set = getSet(addr, s, b);  

	//hit
	for (i = 0; i < E; i++)
	{
		if ((cache[set][i].valid == 1) &&( cache[set][i].tag == tag))
		{
			if ((opt == 'M'))
			{
				++hit;
				++hit;
				lruCounter(cache, set, E, i);
				return HIT_HIT;
			}else{
				++hit;
				lruCounter(cache, set, E, i);
				return HIT;
			}
			
		}
	}

	//miss
	++miss;
	for (i = 0; i < E; i++){
		//cache is empty, no eviction
		if(cache[set][i].valid == 0){
			cache[set][i].valid = 1;
			cache[set][i].tag = tag;
			lruCounter(cache, set, E, i);
			if((opt == 'M')){
				++hit;
				return MISS_HIT;
			}else{
				return MISS;
			}
		}
	}

	//miss and need eviction
	++eviction;
	for  (i = 0; i < E; i++){
		//
		if (cache[set][i].lruIndex == 0)
		{
			cache[set][i].valid = 1;
			cache[set][i].tag = tag;
			lruCounter(cache, set, E, i);
			if((opt == 'M')){
				++hit;
				return MISS_EVICTION_HIT;
			}else{
				return MISS_EVICTION;
			}
		}
	}
	return -1;
}

