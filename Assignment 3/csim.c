//Kristofer Hughes
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "cachelab.h"

//globals
int hit_total= 0, miss_total= 0, eviction_total= 0; //moving declaration up here to globals for simplicity
typedef unsigned long long int findAddress;
int s_bits = 0, lines_per_set = 0, block_bits = 0; //moved existing vars out of main(), removed tag_bits var because it was unused
int verbose = 0; //moved existing var out of main()
char* filename = NULL; //moved existing var out of main()

struct simStruct {
    findAddress addressDesc;
    unsigned evalCount;
    char evaluate;
};

struct simStruct *** cache;

void iniTialize() { //new helper func to initialize cache
    cache = (struct simStruct ***) malloc(pow(2, s_bits) * sizeof(struct simStruct **));
    
    for (int setIndex = 0; setIndex < pow(2, s_bits); ++setIndex) {
        cache[setIndex] = (struct simStruct **) malloc(lines_per_set * sizeof(struct simStruct *));
        for (int j = 0; j < lines_per_set; ++j) {
            cache[setIndex][j] = 
                (struct simStruct *) malloc(sizeof(struct simStruct));
   
            (cache[setIndex][j])->evaluate = 0;
            (cache[setIndex][j])->addressDesc = 0;
            (cache[setIndex][j])->evalCount = 0; }}}

void findFile(char* fileTrace) { //new helper func to find and compare the trace file against the cache
    char newBuffer[1000];
    findAddress addr=0;
    unsigned int len=0;
    FILE* findingTrace = fopen(fileTrace, "r");
    if(!findingTrace) {
        fprintf(stderr, "%s: %s\n", fileTrace, strerror(errno));
        exit(1);
    }
    findAddress setAddressA = pow(2,block_bits) - 1;
    findAddress setAddressB = pow(2,s_bits) - 1;
    setAddressB <<= (block_bits);
    findAddress finalAddress = ~(setAddressA | setAddressB);
 
    while(fgets(newBuffer, 1000, findingTrace) != NULL) {
        if(newBuffer[1]=='S' || newBuffer[1]=='L' || newBuffer[1]=='M') {
            sscanf(newBuffer+3, "%llx,%u", &addr, &len);
            if(verbose)
                printf("%c %llx,%u\n", newBuffer[1], addr, len);

            findAddress setIndex = (setAddressB & addr) >> block_bits;
            findAddress addressDesc = (finalAddress & addr) >> (s_bits + block_bits);
            int totalCount = 1;
            switch(newBuffer[1]) {
            case 'M':
                totalCount = 2;
            case 'S':
            case 'L': {
                for (int k = 0; k < totalCount; ++k) { //find if data exists... then hit/miss/eviction based on result of data
                    int checkEvict = 0;
                    char checkHits = 0;
                    char checkSpace = 1;
                    for (int j = 0; j < lines_per_set; ++j) {
                        if ((cache[setIndex][j])->evaluate) {
                            if ((cache[setIndex][j])->addressDesc == addressDesc) {
                                ++hit_total;
    
                                for (int i = 0; i < lines_per_set; ++i)
                                    if ((cache[setIndex][i])->evaluate)
                                        ++((cache[setIndex][i])->evalCount);
                                (cache[setIndex][j])->evalCount = 0;
                                checkHits = 1;
                                break;
                            }}
                        else {
                            checkSpace = 0;
                            checkEvict = j;
                            
                        }}

                    if (!checkHits) {
                        ++miss_total;
                        if (!checkSpace) {
                            (cache[setIndex][checkEvict])->evaluate = 1;
                            (cache[setIndex][checkEvict])->addressDesc = addressDesc;
                            for (int i = 0; i < lines_per_set; ++i)
                                if ((cache[setIndex][i])->evaluate)
                                    ++((cache[setIndex][i])->evalCount);
                            
                            (cache[setIndex][checkEvict])->evalCount = 0;
                        }
                        else {
                            ++eviction_total;
                            int maximum = 0;
                            int findMax = 0;
                            for (int i = 0; i < lines_per_set; ++i) {
                                if ((cache[setIndex][i])->evalCount >
                                     findMax) {
                                    findMax = 
                                        (cache[setIndex][i])->evalCount;
                                    maximum = i;  } }
  
                            (cache[setIndex][maximum])->addressDesc = addressDesc;

                            for (int i = 0; i < lines_per_set; ++i) {
                                ++((cache[setIndex][i])->evalCount);
                            }

                            (cache[setIndex][maximum])->evalCount =
                                0;  }}}
                break;
            }
            default:
                break;   }}}
                
    fclose(findingTrace);
}

void reStart() { //new helper func to restart the cache
    for (int setIndex = 0; setIndex < pow(2, s_bits); ++setIndex) {     
  
        for (int j = 0; j < lines_per_set; ++j) {
            free(cache[setIndex][j]);}
        free(cache[setIndex]);}
    free(cache);
}


void printUsage()
{
	printf( "Usage: csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n" );
}

int main(int argc, char* argv[]) {
    char option; //only kept var, look at global vars for the relocated vars originally in main()
    while((option=getopt(argc,argv,"s:E:b:t:vh")) != -1) {
        switch(option) {
        case 's':
            s_bits = atoi(optarg);
            break;
        case 'E':
            lines_per_set = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 't':
            filename = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }
    if (s_bits == 0 || lines_per_set == 0 || block_bits == 0 || filename == NULL) {
        printf("%s: Value for cache parameter not passed on command line\n", argv[0]);
        printUsage(argv);
        exit(1);
    }
    iniTialize();
    findFile(filename);
    reStart();
    printSummary(hit_total, miss_total, eviction_total);
    return 0;
}
