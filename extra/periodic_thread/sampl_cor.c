#include <sys/time.h>     
#include <signal.h>       
#include <unistd.h>      
#include <stdint.h>       
#include <stdio.h>        
#include <stdlib.h>
#include <math.h>


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

printf("Setting timer\n");
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
  int N =(int)t/dt; //number of samples
  long period= (long) (dt*1000000); // period in ms
  float* result=(float *)malloc(sizeof(float)*N ); //array of results 
  
  printf("Total time t=%f and time interval dt=%f\n",t,dt );
  printf("Number of samples %d\n",N );


  struct timeval periodic, now;  // time values of the current time and the periodic i*dt


 static sigset_t sigset;  //For SIGALRM signal used
 
 int status =set_timer(period,&sigset);
 if(status<0){
  printf("Error setting timer\n");
  return -1;}

 int i=0;
 int dummy;
 int set=0;
 float dif ;
 int number=0;
 float max=0;
 float min=dt*1000+1000;
 float mean=0;
 float cyclic[64];
for(i=0;i<64; i++){
  cyclic[i]=0;
}
i=0;
while(i<N){

  /*Wait for signal*/
  sigwait(&sigset,&dummy );
  
  /* Do work*/
  gettimeofday(&now, NULL);
  if(i==0 ){ gettimeofday(&periodic, NULL);
  }else{
  add(&periodic,period);}
  dif = (now.tv_sec - periodic.tv_sec) * 1000.0;      // sec to ms
  dif += (now.tv_usec - periodic.tv_usec) / 1000.0;   // us to ms
  result[i]=dif;
  mean=(mean*64-cyclic[i%64]+dif)/64;
  cyclic[i%64]=dif;

    printf("%f\n",dif );


    if(dif<min){
      min=dif;
    }
  if(dif>max){
      max=dif;
    }


i++;
    if(fabs(dif)>0.1){
      number++;
    }
    if(i%500==0){
      number=0;
    }
      
if((fabs(mean)>0.1 && number>10 ) || fabs(dif) >10){
  set ++;
  /*Restart Timer*/ 
  struct itimerval t;
  struct timeval temp; 
  gettimeofday(&now, NULL);
  temp.tv_sec=periodic.tv_sec;
  temp.tv_usec=periodic.tv_usec;
  add(&temp, period);


   float difnext = (temp.tv_sec - now.tv_sec) * 1000.0;      // sec to ms
  difnext += (temp.tv_usec - now.tv_usec) / 1000.0;   // us to ms
  long offs=(difnext*1000);
  
  t.it_value.tv_sec = offs / 1000000;
  t.it_value.tv_usec = offs % 1000000;

  t.it_interval.tv_sec = period / 1000000;
  t.it_interval.tv_usec = period % 1000000;

  /* Creates a periodic timer */
   status =setitimer(ITIMER_REAL, &t, NULL);
    if(status<0){
  printf("Error setting timer\n");
  return -1;}
   mean=0;
for(int j=0;j<64; j++){
  cyclic[j]=0;
}
number=0;


    }
 }
 printf("min %f \n",min);
 printf("max %f \n",max);

printf("Number of timer resets %d",set);

  return 0;
}
