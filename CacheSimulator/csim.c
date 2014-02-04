/*
* Jeremiah Crowley 
* jcc608
*/

#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h> 

int setMask=0,blockMask=0,setBits,tagBits;
int hits=0,misses=0,evictions=0,timeCounter=0;
unsigned int tagMask;

//Create structure for each cache line
struct cacheLine
{
    int timeChanged; // time when cacheLine changed
    int valid; // cacheLine valid value
    int tag; // cacheLine tag value
};

//Create structure for counter of cache hits and misses 
struct counter
{
    int timeCounter; //Keep track of time for LRU algorithm 
    int hits; 
    int misses; 
    int evictions; 
};

struct cacheLine** initializeCache(int sets, int E) 
{
    struct cacheLine** cache = (struct cacheLine**)malloc(sets * E * sizeof(struct cacheLine*));
    for (int i = 0; i < sets; i++) //loop through number of sets
    {
        cache[i] = (struct cacheLine*)malloc(E * sizeof(struct cacheLine));
        
        //initialize cacheLine values
        cache[i] -> valid = 0;
        cache[i] -> tag = 0;
        cache[i] -> timeChanged = 0;
    }
    return cache;
}

void bitMask(int S, int B) 
{
  	int maskBit=1,s=S,b=B,maskBit1=1;

        while(s!=0)
        {
                setMask=setMask+maskBit;
                maskBit=maskBit<<1;
                s=s-1;
        }
        while(b!=0)
        {
                blockMask=blockMask+maskBit1;
                maskBit1=maskBit1<<1;
                b=b-1;
        }
        tagMask=~(setMask+blockMask);
        setMask=setMask<<B;
}

void cacheSim(int address, int s, int b, int E, struct cacheLine** cache)
{
    int linesCount = 0; //Counter to see if cache is full

    tagBits=address & tagMask; //Get tagBits by bitwise AND with tagMask
    setBits=address & setMask; //Get setBits by btiwise AND with setMask                   
    setBits=setBits>>b; //Shift setBits right by b
    tagBits=tagBits>>(b+s); //Shift tagBits right by b+s
   
    for (int i = 0; i < E; i++) //Loop through lines per set
    {
        timeCounter += 1; //Add one to timeCounter to keep track of time
        
	int cacheTag = (cache[setBits][i]).tag; //Retrieve tag value
        int cacheValid = (cache[setBits][i]).valid; //Retrieve vaild value

        if (cacheValid == 1)
        {
        	linesCount++; //Add to counter, since line is full
        }
            
        if (cacheValid == 1 && cacheTag == tagBits) //Cache Hit
        {
                //Change time to current 
                cache[setBits][i].timeChanged = timeCounter;
                //Hit counter++
                hits += 1;
                return;
        }
    }
	
    //If not retured, it's a cache miss
    
    if(linesCount == E) //If cache is full then we need to evict 
    {
	misses += 1; //Misses counter++
        evictions += 1; //Evictions counter++

        int timeMin = cache[setBits][0].timeChanged; //Initialize timeMin to the time of the first cacheLine 
        int indexMin = 0;
        
        for (int i = 0; i < E; i++) //Loop through the set to fine LRU 
        {
            if(cache[setBits][i].timeChanged < timeMin)
            {
                //If time is greater than timeMin, update timeMin to timeChanged, and update indexMin to i 
                timeMin = cache[setBits][i].timeChanged;
                indexMin = i;
            }
        }
        
        //update tag and time to current values 
        cache[setBits][indexMin].tag = tagBits;
        cache[setBits][indexMin].timeChanged = timeCounter;
    }
    else
    {
	misses += 1; //Misses coutner++
        for (int i = 0; i < E; i++) //Loop to find an empty cacheLine
        {
            if(cache[setBits][i].valid == 0)
            {
                cache[setBits][i].valid = 1;
                cache[setBits][i].timeChanged = timeCounter;
                cache[setBits][i].tag = tagBits;
                return;
            }
        }
    }
}

int main(int argc, char** argv)
{
    //initialize variables 
    int s = 0;
    int E = 0;
    int b = 0;
    //input file
    FILE * inputFile;

    int opt; 

    while((opt = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                inputFile = fopen(optarg, "r");
                break;
            default:
                break;
        }
    }
    
    char command;
    int address;
    int size;

    int sets = pow(2,s);//Number of sets
    struct cacheLine** cache = initializeCache(sets,E); //Initialize cacheLine
    bitMask(s,b); 
    
    //Parse input file 
    while(fscanf(inputFile, " %c %x,%d", &command, &address, &size) > 0)
    {
        switch(command)
        {
            case 'L': //Load
            case 'S': //Store
                cacheSim(address, s, b, E, cache);
                break;
            case 'M': //Modify
                cacheSim(address, s, b, E, cache);
                cacheSim(address, s, b, E, cache);
                break;
            default:
                break;
        }
    }
    fclose(inputFile);
    printSummary(hits,misses,evictions);

    return 0;
}
