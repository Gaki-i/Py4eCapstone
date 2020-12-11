#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

//hashtable struct
struct HashElem{
	unsigned char *key;
	char *movie;
	struct HashElem *next;
};

//init global Hashtable
struct HashElem* ht[512];

//hashfunction
int hash (unsigned char *str)
{
  int hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c;	/* hash * 33 + c */

  return hash % 512;
}


struct HashElem *get(unsigned char *key, char *movie)
{
	printf("GET\n");
	int index = hash(key);
	struct HashElem *elem = (struct HashElem*) malloc(sizeof(struct HashElem));
	elem->key = malloc( sizeof( unsigned char ) * 512 );
	elem->movie = malloc(sizeof(char) * 512);
	if(ht[index] == NULL) return NULL;
	elem = ht[index];
	elem->movie = ht[index]->movie;
	elem->key = ht[index]->key;
	while(elem != NULL)
	{
		if(memcmp(elem->key, key, sizeof(&key)) == 0){
			printf("found a matching value\n");
			return elem;
		}
		elem = elem->next;
	}
	return NULL;
}


int set(unsigned char *key, char* movie){
	printf("SET\n");
	//to iterate
	int index = hash(key);
	struct HashElem *newelem = (struct HashElem*) malloc(sizeof(struct HashElem));
	newelem->next = NULL;
	//fehler
	newelem->key = malloc(sizeof(unsigned char) * 512);
	memcpy(newelem->key, key, sizeof(&key));
	newelem->movie = malloc(sizeof(char) * 512);
	strcpy(newelem->movie, movie);
	if(ht[index] == NULL)
	{
		printf("no element at index\n");
		ht[index] = newelem;
		return 0;
	}
	struct HashElem *curr = (struct HashElem*) malloc(sizeof(struct HashElem));
	curr->next = ht[index]->next;
	curr->key = malloc(sizeof(unsigned char) * 512);
	memcpy(curr->key, ht[index]->key, sizeof(&ht[index]->key));
	curr->movie = malloc(sizeof(char) * 512);
	strcpy(curr->movie, ht[index]->movie);
	printf("TEST\n");
	if(memcmp(ht[index]->key, key, sizeof(&key)) == 0)
	{
		printf("key already exists, replace value\n");
		strcpy(ht[index]->movie, movie);		
		return 0;
	}
	while(curr->next != NULL)
	{
		if(memcmp(curr->next->key, key, sizeof(&key)) == 0)
		{
			printf("key already exists, replace value");
			strcpy(curr->next->movie, movie);		
			return 0;		
		}
		curr = curr->next;
	}
	printf("unique key, insert value\n");
	curr->next = newelem;
	return 0;
}


struct HashElem del(struct HashElem *elem)
{
	printf("DEL\n");
	struct HashElem *curr = (struct HashElem*) malloc(sizeof(struct HashElem));
	curr->key = malloc( sizeof( unsigned char ) * 100 );
	curr->movie = malloc(sizeof(char) * 100);
	struct HashElem *prev = (struct HashElem*) malloc(sizeof(struct HashElem));
	prev->key = malloc( sizeof( unsigned char ) * 100 );
	prev->movie = malloc(sizeof(char) * 100);
	unsigned char* key = elem->key;

    //get the hash 
    int index = hash(key);
	curr = ht[index];
	prev =curr;
	if(curr == NULL) return *elem;
	if(curr->next == NULL){
		ht[index] = NULL;
		return *elem;
	}
    while(curr != NULL) 
	{
		if(memcmp(elem->key, key, sizeof(&key)) == 0) 
		{ 
			printf("delete element\n");
			prev->next = curr->next;
			free(curr);
			return *elem;
		}
		
		//go to next cell
		prev = curr;
		curr = curr->next;
		
	}      
	return *elem; 
} 

int main(int argc, char *argv[]) 
{
	//init variables
	int sockfd, portno;
	socklen_t socklen;
	unsigned char *reply = (unsigned char *) malloc( 512 );
	struct sockaddr_in serv_addr, client_addr;
	int n;
	
	if(argc != 2)
	{
		printf("Usage: ./server port\n");
		exit(1);
	}
	//socket()
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	//important infos of the server
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
	memset(serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero);
    //enable many binds in a close timerange -> tell kernel to re-use port
    int useSockAgain = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &useSockAgain, sizeof(useSockAgain));
	//bind()
	if(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof serv_addr) == -1)
	{
		printf("binding failed\n");
		exit(1);	
	}
	//listen()
	while(1)
	{	
		int secondsockfd;
		struct HashElem *elem = (struct HashElem*) malloc(sizeof(struct HashElem));
		elem->key = malloc( sizeof( unsigned char ) * 100 );
		elem->movie = malloc(sizeof(char) * 100);
		struct HashElem *tempElem = (struct HashElem*) malloc(sizeof(struct HashElem));
		tempElem->key = malloc( sizeof( unsigned char ) * 100 );
		tempElem->movie = malloc(sizeof(char) * 100);
		int id = 0;
		int keyLen = 0;
		int movieLen = 0;
		listen(sockfd,5);

		socklen = sizeof client_addr;
		//accept()
		secondsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &socklen);
		if(secondsockfd == -1)
		{
			printf("accept failed\n");
			exit(1);
		}
		//recv()
		n = recv(secondsockfd, reply, 255,0);
		if(n == -1)
		{
			printf("recv failed\n");
			exit(1);
		}
		//get Key, Movie and ID korrekt
		id = reply[1];
		keyLen = reply[2] * 256 + reply[3];
		movieLen = reply[4] * 256 + reply[5];
		int i;
		for(i = 0; i < keyLen; i++)
		{
			elem->key[i] = reply[6+i];
		}
		for(i = 0; i < movieLen; i++)
		{
			elem->movie[i] = reply[6+keyLen+i];
		}
		//get Operation
		if ((reply[0] & 0b00000100) >= 1) tempElem = get(elem->key, elem->movie);
		if ((reply[0] & 0b00000010) >= 1) set(elem->key, elem->movie);
		if ((reply[0] & 0b00000001) >= 1) del(elem);
		//set acknowledgement
		if ((reply[0] & 0b00001000) < 1) reply[0] = reply[0] +8;
		//answers get with key and value and others without key and value
		
		if ((reply[0] & 0b00000100) >= 1)
		{
			if(tempElem != NULL)
			{
				reply[4] = strlen(tempElem->movie) / 256;
				reply[5] = strlen(tempElem->movie);
				printf("moviename: %s movieLen: %d\n",tempElem->movie, reply[4] + reply[5]);
			}
			else
			{
				reply[4] = 0;
				reply[5] = 0;
			}
		}
		else
		{
			reply[4] = 0;
			reply[5] = 0;
		}
		//send() random line
		n = send(secondsockfd, reply, 512, 0);
		if(n == -1)
		{
			printf("send failed\n");
			exit(1);
		}
		close(secondsockfd);
	//close
	}
	close(sockfd);
	return 0;
}
