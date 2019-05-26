#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

typedef struct Car{
	int cid;
	int serv_time;
	double pay_amount;
	int q_num;
}Car;

Car *car;

static void * thread_car(void * arg){
	pthread_detach(pthread_self());
	int *i = (int *) arg;
	int id = *i;
	
}

int main(int argc, char *argv[]){

	pthread_t *th_car;
	int id = 0;
	
	while(1){	
		car = (Car *)malloc(sizeof(Car));	
		th_car = (pthread_t *)malloc(8 * sizeof(pthread_t));
		sleep((rand() % 5) + 1);	
			int i;	
		/*create narrivals cars*/
		for(i=id+1; i <= 8+id; i++){
			printf("creating car thread %d\n",i);
			int *ci = (int *)malloc(sizeof(int));	
			*ci = i; 
			pthread_create(&th_car[i], NULL, thread_car, (void *)ci);
		}
		id += 8;
		int tot_arrivals = 8 + id;
		Car *new_car = (Car *)realloc(car, tot_arrivals * sizeof (Car));
		car = new_car;
	}






}
