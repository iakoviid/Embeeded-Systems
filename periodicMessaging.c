#include <stdio.h> 
#include <stdlib.h>
#include <sys/time.h>     
#include <string.h> 
#include <time.h> 
#include <pthread.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <signal.h> 
#define PORT 2288
#define BUFFER_SIZE 2000
#define AM 8789
#define NSEC_PER_SEC 1000000000ULL
#define MAX_PENDING_REQS 100
/*Global Variables message buffer and device-list*/
int head=0;
int n;
struct buffer_elem * buffer;
unsigned int* devlist;
int end=0;
int first_time =0;
int* socketbuf;
int sockhead=0;
unsigned long timeoffset=0;
FILE *fp2;

struct buffer_elem{
	int* devs;
	char message[300];
	//unsigned long time;
};
struct Message{
	unsigned int sender;
	unsigned int reciever;
	unsigned long int time;
	char text[256];};
pthread_mutex_t lock; 


void createMessage(char* out,struct Message  message);
void decomposeMessage(char* mes,struct Message * message);
int Isinbuffer(struct buffer_elem *buffer,char * message);
void writeElem(struct buffer_elem *buffer,char * message);
void generateMessage(char* mes,int* devlist);
void* server_routine(void* arg);
void *pthread_routine(void *arg);
void getIP(int* devlist,char** ip);
void socketbuf_insert(int *socketbuf,int new_socket);
int socketbuf_remove(int *socketbuf);
static inline void timespec_add_us(struct timespec *t, uint64_t d)
{
    d *= 1000;
    d += t->tv_nsec;
    while (d >= NSEC_PER_SEC) {
        d -= NSEC_PER_SEC;
	t->tv_sec += 1;
    }
    t->tv_nsec = d;
}
static void handler(int signum)
{
  pthread_exit(NULL);
}

//int set_timer(long period);
//static void sighand(int s);


int main(int argc, char const *argv[]) 
{ 

	if(argc<4){
		printf("Usage sampl t (total_time in secs) dt (time_interval in secs) AM-list\n");
		return 1;
	}
	FILE *fp;
  	fp = fopen("generatetime.txt", "w+");
  	//FILE *fp1;
  	//fp1 = fopen("generatesendtime.txt", "w+");
  	
  	fp2= fopen("recievetime.txt", "w+");
  	struct timeval now,prev;
  	gettimeofday(&now, NULL);
  	timeoffset = (now.tv_sec) * 1000;      // sec to ms
	timeoffset += (now.tv_usec)/ 1000;   // us to ms
	


	float t,dt;
	t = atof(argv[1]); // Total time 
	dt = atof(argv[2]); // period
	n=argc-3;
	devlist=(unsigned int *)malloc(n*sizeof(unsigned int));
	for(int i=0;i<n;i++){
		devlist[i]=atoi(argv[i+3]);
	}
	srand(time(0)); 

	buffer  = (struct buffer_elem*)malloc(BUFFER_SIZE * sizeof(struct buffer_elem));
	for(int i=0;i<BUFFER_SIZE;i++){
		buffer[i].devs=malloc(n*sizeof(int));
		for(int j=0;j<n;j++){
			buffer[i].devs[j]=1;
		}
	}

	if (pthread_mutex_init(&lock, NULL) != 0) 
    { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
     

 	signal(SIGUSR1, handler);

    pthread_t server;
	//pthread_detach(client);
	int N =(int)t/dt; //number of samples
  	long period= (long) (dt*1000); // period in ms
 
	pthread_create(&server,NULL,server_routine,NULL);


	int s=0;
  	gettimeofday(&prev, NULL);
  	gettimeofday(&now, NULL);

  	/*Get ips from the device list*/
	char** ip=(char** )malloc(n*sizeof(char*));
	getIP( devlist,ip);

	int sock = 0; 
	struct sockaddr_in serv_addr;
	//static struct timespec r;
	//clock_gettime(CLOCK_REALTIME, &r);
	//timespec_add_us(&r, 1000);
	/*Until time runs out*/
	while(s<N){
		//clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &r, NULL);
		//timespec_add_us(&r, period*1000);

	for(int i=0;i<n;i++){

	gettimeofday(&now, NULL);
	float dif = (now.tv_sec - prev.tv_sec) * 1000.0;      // sec to ms
  	dif += (now.tv_usec - prev.tv_usec) / 1000.0;   // us to ms 
	if(dif>period){
		fprintf(fp, "%f\n",dif);
		gettimeofday(&prev, NULL);
		char mes[300];
		generateMessage(mes,devlist);
		pthread_mutex_lock(&lock); 
		writeElem(buffer,mes);
		pthread_mutex_unlock(&lock); 
		s++;
		printf("New message=%s\n",mes);	
	}
	/*Connect to the i'th Device*/
	int a=0;
	int b=0;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		a=1;
		b=1;
		//return -1; 
	} 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	if(inet_pton(AF_INET, ip[i], &serv_addr.sin_addr)<=0 && a==0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		//return -1; 
		a=1;
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 && a==0) 
	{ 
		//printf("\nConnection Failed \n"); 
		//return -1; 
		a=1;
	}
	if(a==0){
	/*Send messages from the list that have not been sent to the i'th device*/
	char* out;
	pthread_mutex_lock(&lock);
	int d=BUFFER_SIZE;
	if(first_time==0){d=head;}
	for(int j=0;j<d;j++){
		if(buffer[j].devs[i]==0){
		out=buffer[j].message;
		//printf("out=%s\n",out );
		char s1[12];
		sprintf(s1, "%ld",strlen(out));
		send(sock,s1,3,0);
		send(sock , out , strlen(out) , 0 );
		//printf("Client Sends\n");
		buffer[j].devs[i]=1;
		//struct timeval tmsp;
		//gettimeofday(&tmsp, NULL);
		//unsigned long temp=(tmsp.tv_sec ) * 10000.0; 
		//temp += (tmsp.tv_usec ) / 100.0;
		//temp=temp-buffer[j].time;
		//printf("temp=%ld\n",temp);
		//fprintf(fp1, "%lu\n",temp);


	} 
	}

	pthread_mutex_unlock(&lock);
	/*Send End of Messaging*/
	send(sock,"EOM",strlen("EOM"),0); 
	}
	if(b==0){
	close(sock);}
	}
	}
	end=1;
	pthread_kill(server, SIGUSR1);

  	pthread_join(server, NULL);
   

    printf("---------------------------------------------\n");
    for(int i=0;i<BUFFER_SIZE;i++){
    	printf("%s",buffer[i].message );
    	for(int j=0;j<n;j++){
    		printf("%d",buffer[i].devs[j] );
    	}
    	printf("\n");
    }
    for(int i=0;i<BUFFER_SIZE;i++){
    //	free(buffer[i].message);
    	free(buffer[i].devs);
    }

    free(buffer);
    fclose(fp);
    free(devlist);
    //fclose(fp1);

    pthread_mutex_destroy(&lock); 
  	return 0;
}

void socketbuf_insert(int *socketbuf,int new_socket){

	if(socketbuf[sockhead]>0){
		close(socketbuf[sockhead]);
	}
	socketbuf[sockhead]=new_socket;
	
	sockhead++;
	sockhead=sockhead%MAX_PENDING_REQS;



}
int socketbuf_remove(int *socketbuf){
	int result=0;
	for(int i =0;i<MAX_PENDING_REQS;i++){
		if(socketbuf[i]>0){
			result=socketbuf[i];
			socketbuf[i]=0;
			break;

		}
	}

	return result;
}


void* server_routine(void* arg){
	/*-------------------------------------Server Code--------------------------------------*/
	socketbuf=(int *)calloc(MAX_PENDING_REQS, sizeof(int));
	int server_fd, new_socket; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Forcefully attaching socket to the port 2288 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
	

	// Forcefully attaching socket to the port 2288 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
	pthread_t pid;
	pthread_create(&pid,NULL,pthread_routine,NULL);
	while (1) {

	
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		perror("accept"); 
		exit(EXIT_FAILURE); 
	}
	socketbuf_insert(socketbuf,new_socket);
	

 
	}

}
void getIP(int* devlist,char** ip){
	for(int i=0;i<n;i++){
	char* temp="10.0.";
	int a=devlist[i]/100;
	char s1[12];
	sprintf(s1, "%d",a);
	int b=devlist[i]%100;
	char s2[12];
	sprintf(s2, "%d",b);
	char* out= (char*) malloc(strlen(s1)+1 + strlen(s2) +strlen(temp));
	strcpy(out, temp);
	strcat(out, s1);
	strcat(out, ".");
	strcat(out, s2);
	ip[i]=out;


	}	
}

void createMessage(char* mes,struct Message  message){
	char s1[12];
	sprintf(s1, "%d",message.sender);
	char s2[12];
	sprintf(s2, "%d", message.reciever);
	char s3[20];
	sprintf(s3, "%lu", message.time);
	char* s4=message.text;
	strcpy(mes, s1);
	strcat(mes, "_");
	strcat(mes, s2);
	strcat(mes, "_");
	strcat(mes, s3);
	strcat(mes, "_");
	strcat(mes, s4);
	sprintf(s2, "%d", rand()%1000);
	strcat(mes, s2);


}
int Isinbuffer(struct buffer_elem *buffer,char * message){
	int result=0;
	for(int i=0;i<BUFFER_SIZE;i++){
		if (strcmp(buffer[i].message, message) == 0){
			result=1;
			break;
	}}
	return result;
}
void writeElem(struct buffer_elem *buffer,char * message){
	strcpy(buffer[head].message,message);
	struct timeval now;
	gettimeofday(&now, NULL);

	//buffer[head].time = (now.tv_sec ) * 10000.0;      // sec to ms
  	//buffer[head].time += (now.tv_usec ) / 100.0;   // us to ms 

	for(int i=0;i<n;i++){
		buffer[head].devs[i]=0;
	}
	//memset(arr, 0, n);
	head++;
	if(head>=BUFFER_SIZE){
		first_time=1;}
	head=head%BUFFER_SIZE;

}
void generateMessage(char* mes,int* devlist){
	struct Message message;
	message.sender=AM;
	message.reciever=devlist[rand()%n];
	struct timeval now;
	gettimeofday(&now, NULL);
	long unsigned time;
	//double temp;
	time = (now.tv_sec) * 1000;      // sec to ms
	time += (now.tv_usec)/ 1000;   // us to ms
	//time= temp;
	message.time=time-timeoffset;
	strcpy(message.text,"3-SAT is in PSPACE");
	createMessage(mes,message);


}
void decomposeMessage(char* mes,struct Message * message){
  char *tempbf;
  char *fields[4];
  int j = 0;
  tempbf = strtok(mes, "_");
  while (tempbf != NULL) {
    fields[j] = tempbf;
    j++;
    tempbf = strtok(NULL, "_");
  }
  message->sender=atoi(fields[0]);
  message->reciever=atoi(fields[1]);
  char* ptr;
  message->time=strtoul(fields[2], &ptr, 10);
  
  
  strcpy(message->text,fields[3]);
}


void *pthread_routine(void *arg) {
	int new_socket;
	while(end==0){
	new_socket=socketbuf_remove(socketbuf);
	if(new_socket>0){
  	int valread=0;
	char mes[300] = {0}; 
	char length[3]= {0};
	while(valread>=0){
	read(new_socket,length,3);
	
	if(length[0]=='E'&& length[1]=='O'&& length[2]=='M'){
		break;
	}
	int valread = read( new_socket , mes, atoi(length));
	mes[atoi(length)]='\0'; 
	char out[300];
	strcpy(out,mes);
	struct Message message;
	decomposeMessage(out,&message);
	unsigned long temp;
	struct timeval now;
	gettimeofday(&now, NULL);
	temp = (now.tv_sec) * 1000;      // sec to 0.1ms
	temp += (now.tv_usec)/ 1000 ;   // us to 0.1ms
	//printf("time=%ld temp=%ld\n",message.time,temp );
	temp=temp-timeoffset;

	temp=temp -message.time;
	printf("Recieved message %s\n",mes);

	//printf("temp=%ld\n",temp );
	//printf("%ld \n",temp);
	fprintf(fp2, "%lu\n",temp);
	if(message.reciever!=AM){
	pthread_mutex_lock(&lock); 
	//printf("Message= %s valread=%d\n",mes,valread );
	if(valread>=0 && Isinbuffer(buffer,mes)==0){
		//printf("%s\n",mes );
		//printf("Server Recieves head=%d\n",head );
		//for(int i=0;i<100000;i++){
		//printf("=========================================================================================");}
		writeElem(buffer,mes);
	}
	pthread_mutex_unlock(&lock); 
	}
	}
	close(new_socket);
    }}
	fclose(fp2);
	pthread_exit(0);



    return NULL;
}

/*int set_timer(long period){

  printf("Setting timer\n");
  struct itimerval t;

  int offs=1;
  t.it_value.tv_sec = offs / 1000000;
  t.it_value.tv_usec = offs % 1000000;

  t.it_interval.tv_sec = period / 1000000;
  t.it_interval.tv_usec = period % 1000000;

  
  signal(SIGALRM, sighand);
  
  return setitimer(ITIMER_REAL, &t, NULL);

}
static void sighand(int s)
{
	char* mes;
	generateMessage(&mes,devlist);
	pthread_mutex_lock(&lock); 
	writeElem(buffer,mes);
	pthread_mutex_unlock(&lock); 

}
*/

//char* message;
//generateMessage(&message,devlist);
//printf("%s\n",message );