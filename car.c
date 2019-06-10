#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<semaphore.h>
#include<stdbool.h>
#include<errno.h>

#define C_QUEUE_MAX 100
#define N_CAR_MAX 200

enum FUEL_TYPE{PETROL, DIESEL};

typedef struct CarQueue{
	int front, rear, size;
	int capacity;
	int *cars;
	pthread_mutex_t q_lock;
	sem_t *empty, *full;
}CarQueue;

typedef struct Car{
	int cid;
	enum FUEL_TYPE f_type;
	int serv_time;
	double pay_amount;
	int q_num;
	sem_t *q_sem;
}Car;
Car *car;
CarQueue *c_queue1, *c_queue2;

int pipe_fd[2];
sem_t *p_sem;

int narrivals, interval, service;
int tot_arrivals;

void put(CarQueue *c_queue, int car_id){
	sem_wait(c_queue->empty);
	c_queue->rear = (c_queue->rear + 1 )% c_queue->capacity;
	c_queue->cars[c_queue->rear] = car_id;
	c_queue->size += 1;
	sem_post(c_queue->full);
}

int get(CarQueue *queue1, CarQueue *queue2){
	int car_id;
	if(sem_trywait(queue1->full) == 0){
		queue1->front = (queue1->front + 1)% queue1->capacity;
		car_id = queue1->cars[queue1->front];
		queue1->size -= 1;
		sem_post(queue1->empty);	
	} else{
		queue2->front = (queue2->front + 1)% queue2->capacity;
		car_id = queue2->cars[queue2->front];
		queue2->size -= 1;
		sem_post(queue2->empty);	
	}
	return car_id;	
}

void print_queues(){
	int i;
	printf("QUEUE1 \n");
	for(i=0; i<c_queue1->size;i++){
		printf("%d ",c_queue1->cars[i]);
	}
	/*printf("QUEUE 2\n");
	for(i=0; i<c_queue2->size;i++){
		printf("%d ",c_queue2->cars[i]);
	}*/
	return;
}

/********************THREADS START ROUTINE***********************/
/***************************************************************/
static void * thread_checkout(void * arg){
	pthread_detach(pthread_self());
	while(1){
		Car c;
		int res = read(pipe_fd[0], &c, sizeof(c));
		if(res != 1){ perror("pipe read(): "); }
		/*calculate amount to pay*/
		if(c.f_type == 0){
			/*petrol price*/
			c.pay_amount = c.serv_time * 1.6f;
		}
		if(c.f_type == 1){
			c.pay_amount = c.serv_time * 1.4f;
		}
		printf("======================CHECKOUT THREAD===========================\n");
		printf("car id: %d queue_num: %d amount_payed: %f \n", c.cid, c.q_num, c.pay_amount);
		printf("================================================================\n");
		sem_post(car[c.cid].q_sem);
	}
	pthread_exit((void *)pthread_self());
}

static void * thread_pump(void * arg){
	pthread_detach(pthread_self());
	int *id = (int *)arg;
	int pid = *id;
	//sleep(20);
	//print_queues();
	sleep(30);
	while(1){
		int c_id1, c_id2, serv_time; 
		/*pump 1 - pump4*/
		srand(time(NULL));
		if(pid <= 4){
			pthread_mutex_lock(&c_queue1->q_lock);
				c_id1 = get(c_queue1, c_queue2);				
				serv_time = ((rand() % service) + 1);
				//fprintf(stdout,"pump %d serving car %d from queue 1\n", pid, c_id1);
				sleep(serv_time);	
				car[c_id1].serv_time = serv_time;
				car[c_id1].q_num = 1;
				int res = write(pipe_fd[1], &car[c_id1], sizeof(car[c_id1]));
			pthread_mutex_unlock(&c_queue1->q_lock);
		}
		
		
		/*pump 5 - pump8*/
		if(pid > 4){
			pthread_mutex_lock(&c_queue2->q_lock);
				c_id2 = get(c_queue2, c_queue1);			
				serv_time = ((rand() % service) + 1);
				sleep(serv_time);
				
				car[c_id2].serv_time = serv_time;
				car[c_id2].q_num = 2;
				int res = write(pipe_fd[1], &car[c_id2], sizeof(car[c_id2]));			
			pthread_mutex_unlock(&c_queue2->q_lock);
		}
		
	}
	pthread_exit((void *)pthread_self());
}

static void * thread_car(void * arg){
	pthread_detach(pthread_self());
	int id;
	Car *c = (Car *) arg;
	id= c->cid;	

	/*select fuel type randomly*/
	srand(time(NULL));
	int r = ((rand() % 10));
	if(r >= 5){	car[id].f_type = PETROL;}
	else{ car[id].f_type = DIESEL; }
	
	/*randomly chose queue*/
	srand(time(NULL));
	int q = ((rand() % 2) + 1);
	if(q == 1){
		put(c_queue1, id);
		//printf("car %d waiting on queue 1\n",id);
		sem_wait(car[id].q_sem);		
	} 
	if(q == 2){
		put(c_queue2, id);
		//printf("car %d waiting on queue 2\n",id);
		sem_wait(car[id].q_sem);
		
	}
	
	printf("car %d exiting station\n", id);
	pthread_exit((void *)pthread_self());	
	
}

void init(){
	c_queue1 = (CarQueue *)malloc(C_QUEUE_MAX * sizeof(CarQueue));
	c_queue2 = (CarQueue *)malloc(C_QUEUE_MAX * sizeof(CarQueue));
	car = (Car *)malloc(N_CAR_MAX * sizeof(Car));
	int i;
	for(i=0; i< N_CAR_MAX; i++){
		car[i].q_sem = (sem_t *)malloc(sizeof(sem_t));
		sem_init(car[i].q_sem, 0, 0);
	}
	/*init car queue1*/
	c_queue1->front = c_queue1->size = 0;
	c_queue1->rear = C_QUEUE_MAX - 1;
	c_queue1->capacity = C_QUEUE_MAX;
	c_queue1->cars = (int *)malloc(C_QUEUE_MAX * sizeof(int));
	
	c_queue1->empty = (sem_t *)malloc(sizeof(sem_t));
	sem_init(c_queue1->empty, 0, C_QUEUE_MAX);
	
	c_queue1->full = (sem_t *)malloc(sizeof(sem_t));
	sem_init(c_queue1->full, 0, 0);
	
	pthread_mutex_init(&c_queue1->q_lock, NULL);
	
	/*init car queue2*/
	c_queue2->front = c_queue2->size = 0;
	c_queue2->rear = C_QUEUE_MAX - 1;
	c_queue2->capacity = C_QUEUE_MAX;
	c_queue2->cars = (int *)malloc(C_QUEUE_MAX * sizeof(int));
	
	c_queue2->empty = (sem_t *)malloc(sizeof(sem_t));
	sem_init(c_queue2->empty, 0, C_QUEUE_MAX);
	
	c_queue2->full = (sem_t *)malloc(sizeof(sem_t));
	sem_init(c_queue2->full, 0, 0);
	
	pthread_mutex_init(&c_queue2->q_lock, NULL);
	
	
	/*init pipe*/
	int res = pipe(pipe_fd);
	if(res < 0){
		perror("pipe");
		exit(1);
	}	
}


int main(int argc, char *argv[]){
	if(argc != 4){
		printf("usage: %s narrivals interval service\n", argv[0]);
		exit(1);
	}	
	narrivals = atoi(argv[1]);
	interval = atoi(argv[2]);
	service = atoi(argv[3]);
	
	init();
	
	pthread_t th_ch, *th_pump, *th_car;
	th_pump = (pthread_t *)malloc(8 * sizeof(pthread_t));
	int i, *pi;
	/*create pump thread*/
	for(i=0; i< 8; i++){
		pi = (int *)malloc(sizeof(int));
		*pi = i;
		pthread_create(&th_pump[i], NULL, thread_pump, (void *)pi);
	}
	/*create checkout thread*/
	pthread_create(&th_ch, NULL, thread_checkout, (void *)NULL);
	
	/*create car threads with interval*/
	th_car = (pthread_t *)malloc(N_CAR_MAX * sizeof(pthread_t));
	int id = 0, j=0;
	while(j<=N_CAR_MAX){
		sleep((rand() % interval) + 1);	
		
		for(i=id+1; i <= narrivals+id; i++){				 
			car[i].cid = i;
			//printf("creating car thread %d\n",*ci);
			pthread_create(&th_car[i], NULL, thread_car, (void *)&car[i]);
		}
		id += narrivals;
		j++;
	}
	
	pthread_exit((void *)pthread_self());
	
	
	
}
