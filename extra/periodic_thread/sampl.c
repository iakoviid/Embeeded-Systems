#include <sys/time.h>     
#include <signal.h>       
#include <unistd.h>      
#include <stdint.h>       
#include <stdio.h>        
#include <stdlib.h>



// calculates addition of positive arguments
void add(struct timeval * a, long b)
{

    
    a->tv_usec = a->tv_usec + b;
    if (a->tv_usec >= 1000000)
    {
        a->tv_sec++;
        a->tv_usec -= 1000000;
    }
}
int set_timer(long period,sigset_t *sigset){
	
  struct itimerval t;

  int offs=1;
  t.it_value.tv_sec = offs / 1000000;
  t.it_value.tv_usec = offs % 1000000;

  t.it_interval.tv_sec = period / 1000000;
  t.it_interval.tv_usec = period % 1000000;

  /* Clear the signal set and include the SIGALRM signal */
  sigemptyset(sigset);
  sigaddset(sigset, SIGALRM);
  sigprocmask(SIG_BLOCK, sigset, NULL);

  /* Creates a periodic timer */
  return setitimer(ITIMER_REAL, &t, NULL);

}



int main(int argc, char **argv)
{
	if(argc<3){
		printf("Usage sampl t (total_time) dt (time_interval) \n");
		return 1;
	}
	float t,dt;
	t = atof(argv[1]); // Total time 
	dt = atof(argv[2]); // period
  int N =(int)t/dt;	//number of samples
 	long period= (long) (dt*1000000); // period in ms
  float* result=(float *)malloc(sizeof(float)*N ); //array of results 
	
	printf("Total time t=%f and time interval dt=%f\n",t,dt );
	printf("Number of samples %d\n",N );


  struct timeval prev, now;  // time values of the current and the previous period


 static sigset_t sigset;	//For SIGALRM signal used
 
 int status =set_timer(period,&sigset);
 if(status<0){
 	printf("Error setting timer\n");
 	return -1;}

 int i=0;
 int dummy;
 
 float dif ;
 
 float max=0;
 float min=dt*1000+1000;
 float mean=0;

 while(i<N){

 	/*Wait for signal*/
 	sigwait(&sigset,&dummy );
 	
 	/* Do work*/
 	gettimeofday(&now, NULL);
  if(i==0 ){ gettimeofday(&prev, NULL);
}else{
 	add(&prev,period);}
 	dif = (now.tv_sec - prev.tv_sec) * 1000.0;      // sec to ms
  dif += (now.tv_usec - prev.tv_usec) / 1000.0;   // us to ms
  result[i]=dif;
  printf("%f\n",result[i] );
  	
  	if(dif<min){
  		min=dif;
  	}
	if(dif>max){
  		max=dif;
  	}
  	mean =(mean*i+dif)/(i+1);


i++;
 }
 printf("min %f \n",min);
 printf("max %f \n",max);
 printf("mean %f \n",mean);



  return 0;
}
