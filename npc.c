//
//  npc - novena puzzle competition.
//  Brute force md5 collision finder, single-threaded.  Parallelize by invoking multiple instances on command line.
//
//  http://novena-puzzle.crowdsupply.com/
//  https://www.crowdsupply.com/kosagi/novena-open-laptop
//
//  Created by Roland G. McIntosh on 4/10/14.
//  Copyright (c) 2014 Roland G. McIntosh. All rights reserved.
//
//  This program produced about 10 or 20 collisions for difficulties 98,99, and 100
//  when 8 instances ran from 2AM - 7AM on the Macbook.  Unfortunately, despite difficulty 
//  starting at "90" when the competition started, it jumpted rapidly
//  as actual l33t hax0rs applied their FPGA / CUDA / OpenCL / Amazon EC2 skillz to the problem,
//  leaving this straightforward "simple" approach in the dust.
//
//  So I have one single point.  Hoping to get lucky enough to get 2 points, to at least make the top 10 list.
//
//  Closest I've gotten is 101 matching bits (other than the 128 matching bits from guessing correctly that "novena" was the source of the target digets)
//  str: aaa000fun4996956590, md5: cbc98e88bb2f16e8ecb2f7c0e7bde070, bits in common: 101
//  posting: {"username":"rgm","contents":"aaa000fun4996956590"}
//  {"status": "failure", "reason": "hash cbc98e88bb2f16e8ecb2f7c0e7bde070 only matches 101 bits"}
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <curl/curl.h>

// to shut Xcode up about MD5 being deprecated.  Xcode, I'm sure you're awesome, but you're very Apple-y and I don't want to use CoreServices.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// function prototypes
void printDigest( char*, unsigned char* );
int countMatchingBits( unsigned char* );
void setDigestString( char*, unsigned char*);
void postResult( char* );
int setRandomBaseString( char**, char* );

// globals :-/
const char magicString[] = "novena";              // stored in digest[]
unsigned char digest[MD5_DIGEST_LENGTH];          // = md5("novena") = 29c986a49abf80e9edf2ffe8efb7e040
unsigned char candidateDigest[MD5_DIGEST_LENGTH]; // buffer for storing digests, used by inner loop
const int PROGRESS_ITERATIONS = 10000000;         // show progress after this many ticks

int main(int argc, const char * argv[])
{
    int numCPUs = sysconf( _SC_NPROCESSORS_ONLN );
    int difficulty = 104;   // how many bits must match before accepted by server
    char candidate[128];    // buffer - holds string to be hashed
    char nonce[21];         // nonce appended to a base string. 2^64 only has 20 digits
    char strDigest[33];     // ASCII representation of digest = 32 chars + null
    char defaultBase[] = "novena#";
    char *baseString;

    unsigned long max = ULONG_MAX; // the nonce appended to the base string, on which md5() is run

    // To allow for command-level parallelization, get an input string as a hash base
    // If no param, use hard-coded default
    if ( argc == 2 ) {
        baseString = malloc(strlen(argv[1]) + 1);
        if (baseString != NULL) {
            strcpy(baseString, argv[1]);
        } else {
            printf("Couldn't allocate space for that.");
            return -2;
        }
    } else {
        if ( !setRandomBaseString(&baseString, defaultBase) ) {
            printf("error.\n");
            return -2;
        }
    }

    if ( numCPUs > 0 ) {
        printf("Found %d CPUs. It'd be nice if I had threads, eh?\n", numCPUs);
        // learn pthreads. decide about better output.  link with ncurses?
        // ugh.  maybe just a few console escape sequences
    }

    
    // Find digest of target
    MD5((unsigned char*)&magicString, strlen(magicString), (unsigned char*)&digest);

    printf("Hashing starting from base string %s, difficulty %d\n", baseString, difficulty);
    
    for ( unsigned long i = 0; i <= max; ++i ) {
        sprintf(candidate, "%s%lu", baseString, i); // fuck you strcpy, strcat, strncat, strlen, memset, malloc, and all your friends.  We'll try again with those some day...
        
        MD5((unsigned char*)&candidate, strlen(candidate), (unsigned char*)&candidateDigest);

        int matchingBits = countMatchingBits(candidateDigest);
        
        if ( i % PROGRESS_ITERATIONS == 0) {
            printf(".");
            fflush(stdout); // this took some searching to figure out about buffered output
        }
        
        // Found something sort of close, submit it just in case and to feel proud of a useless accomplishment
        if ( matchingBits >= difficulty - 5) {
          setDigestString(strDigest, candidateDigest);
          printf("\nstr: %s, md5: %s, bits in common: %d\n", candidate, strDigest, matchingBits);
          postResult( candidate );
        }
    }
    
    printf("Done\n"); // TODO: trap signals, make loop run forever until interrupted, print hash rate.
    
    free(baseString);
    return 0;
}

// modifies first param with something random based off second param
// returns 1 when successful, for use in conditional
int setRandomBaseString( char **buf, char *base ) {
    char randStr[16];

    srand(time(NULL));
    rand(); // throw the first one away, its first digits seem to always be '579'.  curious.
    int r = rand() % 100000;
    sprintf(randStr, "%d", r);

    *buf = malloc(strlen(base) + strlen(randStr) + 1);
    if (*buf != NULL) {
        // sprintf(*buf, "%s%d", base, r);
        strcpy(*buf, base);
        strcat(*buf, randStr);
    } else {
        printf("malloc fail for random base string\n");
        return 0;
    }

    return 1;
}

// Sends a successful match to the puzzle contentest server via HTTP POST
void postResult( char * foundString ) {
  CURL *curl;
  CURLcode res;
 
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://novena-puzzle.crowdsupply.com/submit");

    // make the JSON string
    char jsonPost[180];
    char json1[] = "{\"username\":\"rgm\",\"contents\":\"";
    char json2[] = "\"}";

    strcpy(jsonPost, json1);
    strcat(jsonPost, foundString);
    strcat(jsonPost, json2);

    printf("posting: %s\n", jsonPost);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPost);
 
    // make the POST request
    res = curl_easy_perform(curl);
    printf("\n");

    // error check
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    // required cleanup
    curl_easy_cleanup(curl);
  }
}

// counts the number of bits that a candidate digest has in common
// with the contest target digest, which is a global variable.
int countMatchingBits( unsigned char* candidate ) {
    int bitCount = 0;
    
    unsigned char mask = 1;
    unsigned char bit = 0;
    unsigned char dbit = 0;
   
    // remember to lookup std::string::bitcount
    // naive impl.  prolly better way to deal with the 128 bits in the hash
    // by splitting it into two 64-bit or four 32-bit numbers and using &, ~&, ^, somehow
    // See http://graphics.stanford.edu/~seander/bithacks.html
    // TODO: profile / optimize this
    for (int j = 0; j < 16; j++ ) {
        for (int i = 0; i < 8; i++) {
            bit = (candidate[j] & (mask << i)) != 0;
            dbit = (digest[j] & (mask << i)) != 0;
            if ( bit == dbit ) {
                bitCount++;
            }
            
        }
    }

    return bitCount;
}

// Converts 16 bytes of MD5 digest into human-friendly hexadecimal string (32 bytes of ASCII + null)
// @param stringResult pointer to string to modify.  NO BOUNDS CHECKING.  Must be >= 33 bytes.
// @param digest       pointer to 16 bytes of digest
void setDigestString( char* stringResult, unsigned char* digest ){
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&stringResult[i*2], "%02x", (unsigned int)digest[i]);
    }
}

// Prints out the passed-in string along with the ASCII hex of the passed-in 16-byte digest
// used for debugging early on
void printDigest( char* str, unsigned char* digest) {
    char mdString[33];
    
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    
    printf("md5(%s) = %s\n", str, mdString);
}


