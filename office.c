#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"
#include "semaphore.h"

#define MAX_BUF 100
#define NUM_OFFICES 4

typedef struct students{
	int id;
	int office_no;
	int urgent;
	pthread_mutex_t lock;
}Student;

typedef struct Buffer{
	Student *student;
	pthread_mutex_t lock;
	pthread_cond_t full;
	pthread_cond_t empty;
	int in, out, count, size;
}Buffer;

typedef struct Cond{
	int	normal;
	int *urgent;	// NUM_OFFICES to be allocated
	pthread_mutex_t lock;  
	pthread_cond_t  cond;	// to be allocated

} Cond;


Buffer *normal_Q, *special_Q, **urgent_Q, **answer_Q;
Cond *cond;
Student *student;
int k;

Buffer *B_init (int dim){
	Buffer *b = (Buffer *)malloc(sizeof(Buffer));
	b->student = (Student *)malloc(dim * sizeof(Student));
	pthread_mutex_init(&b->lock, NULL);
	pthread_cond_init(&b->full, NULL);
	pthread_cond_init(&b->empty, NULL);
	b->in= b->out = b->count = 0;
	b->size = 0;
	return b;
	
}
Cond * cond_init(int n){
	Cond *c = (Cond *)malloc(sizeof(Cond));
	pthread_mutex_init(&c->lock, NULL);
	pthread_cond_init(&c->cond, NULL);
	c->urgent = (int *)malloc(n * sizeof(int));
	return c;
}

void send(Buffer *buf, Student s){
	pthread_mutex_lock(&buf->lock);
	while(buf->size>=MAX_BUF){
		printf("Full\n");
		pthread_cond_wait(&buf->empty, &buf->lock);
	}
	printf("student %d waiting in queue\n",s.id);
	buf->student[buf->in++] = s;
	buf->in%=MAX_BUF;
	buf->size++;
	pthread_cond_broadcast(&buf->full);
	pthread_mutex_unlock(&buf->lock);

}
Student receive(Buffer *buf){
	Student stud;
	pthread_mutex_lock(&buf->lock);
	while(buf->size<=0){
		//printf("Empty\n");
		pthread_cond_wait(&buf->full, &buf->lock);
	}
	stud = buf->student[buf->out++];
	printf("office receiving student %d\n", stud.id);
	buf->out%=MAX_BUF;
	buf->size--;
	buf->count++;
	pthread_cond_broadcast(&buf->empty);
	pthread_mutex_unlock(&buf->lock);
	return stud;
}
/**********************************************THREADS START ROUTING************************************/
/*******************************************************************************************************/

static void *specialOfficeThreads(void *args){
	pthread_detach(pthread_self());
	/*while(1){
		Student s = receive(special_Q);
		sleep((rand()%6)+3);
		send(special_Q, s);
		
	}*/
	
	
	pthread_exit((void *)pthread_self());
}
static void *officeThreads(void *args){
	pthread_detach(pthread_self());
	int *id = (int *)args;
	while(1){
		sleep(2);
		if(normal_Q->count == k){
			break;
		}
		/*flip a coin*/
		Student st = receive(normal_Q);
		printf("count = %d\n",normal_Q->count);
		srand(time(NULL));
		int r = (rand() % 100);
		if(r>40){
			/*serve and say bye	*/
			st.office_no = *id;
			st.urgent = 0;
			send(&answer_Q[st.id][*id], st);
			//printf("office %d received student %d\n", *id, st.id);
			receive(&urgent_Q[*id][st.id]);
			sleep(1);
			send(&answer_Q[st.id][*id], st);
			
		} else {
			/*serve and send to special office*/
			st.office_no = *id;
			st.urgent = 1;
			send(&answer_Q[st.id][*id], st);
		}
		
		sleep(5);
		
	}
	pthread_exit((void *)pthread_self());
}

static void *studentThreads(void *args){

	pthread_detach(pthread_self());
	sleep((rand()%10));
	int *tid = (int *)args;
	int id = *tid;
	//printf("Student %d\n",id);
	send(normal_Q, student[id]);
	
	Student st = receive(&answer_Q[id][student[id].office_no]);
	/*sleep(2);
	if(st.urgent == 0){
		printf("Student %d received response from office %d\n", id, student[id].office_no);
		
	} else{
		printf("Student %d needs to go to special office\n", id);
		/*Go to special office
		send(special_Q, st);
		receive(special_Q);
		//printf("student %d served by SPECIAL OFFICE\n", st.id);
		send(&urgent_Q[st.office_no][id], st);
		receive(&answer_Q[id][student[id].office_no]);
		printf("student %d completed at office %d\n", id, st.office_no);
	}*/
	
	printf("Student %d terminated after service at office\n", id);
	pthread_exit((void *)pthread_self());
}

/***************************************************************************************************/
/***************************************************************************************************/

void init(){
	student = (Student *)malloc(k * sizeof(Student));
	
	normal_Q = B_init(MAX_BUF);
	special_Q = B_init(MAX_BUF);
	/*office->student*/
	int i, j;
	answer_Q = (Buffer **)malloc(NUM_OFFICES * sizeof(Buffer *));
	for(i=0; i<k; i++){
		answer_Q[i] = (Buffer *)malloc(k * sizeof(Buffer));
			answer_Q[i]->student = (Student *)malloc(MAX_BUF * sizeof(Student));
			pthread_mutex_init(&answer_Q[i]->lock, NULL);
			pthread_cond_init(&answer_Q[i]->full, NULL);
			pthread_cond_init(&answer_Q[i]->empty, NULL);
			answer_Q[i][j].in= answer_Q[i]->out = answer_Q[i][j].count = 0;
			answer_Q[i]->size = 0;	
	}
	/*student->office*/
	urgent_Q = (Buffer **)malloc(k * sizeof(Buffer *));
	for(i=0; i < NUM_OFFICES; i++){
		urgent_Q[i] = (Buffer *)malloc(NUM_OFFICES * sizeof(Buffer));
		urgent_Q[i]->student = (Student *)malloc(MAX_BUF * sizeof(Student));
		pthread_mutex_init(&urgent_Q[i]->lock, NULL);
		pthread_cond_init(&urgent_Q[i]->full, NULL);
		pthread_cond_init(&urgent_Q[i]->empty, NULL);
		urgent_Q[i]->in= urgent_Q[i]->out = urgent_Q[i]->count = 0;
		urgent_Q[i]->size = 0;
	}
	/*office conditions*/
	//cond = cond_init(NUM_OFFICES);
}
int main(int argc, char *argv[]){
	if(argc!=2){
		printf("usage: %s k\n",argv[0]);
		exit(1);
	}
	k = atoi(argv[1]);
	pthread_t *th_st, *th_off, th_sp;
	th_st = (pthread_t *)malloc(k * sizeof(pthread_t));
	th_off = (pthread_t *)malloc(NUM_OFFICES * sizeof(pthread_t));
	
	
	int i,j, *pi;
	init();

	for(i=0; i<k; i++){
		student[i].id = i;
		pthread_create(&th_st[i],NULL,studentThreads, (void *)&student[i].id);
	}
	for(i=0;i<NUM_OFFICES;i++){
		pi = (int *)malloc(sizeof(int));
		*pi = i;
		pthread_create(&th_off[i],NULL, officeThreads, (void *)pi);
	}
	//pthread_create(th_off,NULL, specialOfficeThreads, (void *)NULL);

	pthread_exit(0);
	return 0;
}



