#include <pthread.h>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <vector>
#include <future>
#include "quill.h"



typedef struct {
    std::function<void()> deque[5000000];
    int high = 0;
} deque;

pthread_key_t *key;
pthread_t *thread_pool;
deque * deque_array;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int breaking = 0;
std::vector<int> manager;

int num_threads=1;

void thread_exit(void* data){
    free(data);
}

bool wake_up_cond(){

    for(int i = 0; i < num_threads; i++){
        if(deque_array[i].high > 0){
            return true;
        }
    }

    return false;

}

void add_first(int id){
    if(deque_array[id].high +1 >= 5000000){
        return;
    }

    for(int i =  deque_array[id].high - 1; i >= 0; i--){
        deque_array[id].deque[i+1] = deque_array[id].deque[i];
    }

    deque_array[id].high++;
}

void delete_first(int id){

    if(deque_array[id].high == 0){
        return;
    }

    for(int i =0 ; i < deque_array[id].high -1; i++){
        deque_array[id].deque[i] = deque_array[id].deque[i+1];
    }

    deque_array[id].high--;

}

void* thread_func(void* data){

    int *id = reinterpret_cast<int*>(data);
    int *thread_id = (int *) malloc(sizeof(int));
    *thread_id = *id;
    pthread_setspecific(key[*id],thread_id);
    
    bool stop = false;

    pthread_mutex_lock(&mutex);
    while(breaking < 1){
        pthread_cond_wait(&cond,&mutex);
    }

    pthread_mutex_unlock(&mutex);

    printf("thread %d initalized\n",*id);

    while(!stop){


        std::function<void()> task;
        bool task_check = false;

        pthread_mutex_lock(&mutex);

        if(deque_array[*id].high > 0){
            task = deque_array[*id].deque[0];
            delete_first(*id);
            manager.pop_back();
            task_check = true;
        }

        pthread_mutex_unlock(&mutex);

        if(task_check){
            task();
            pthread_cond_signal(&cond);
        }

        else{

            for(int i = 0; i<num_threads; i++){
                
                if(*id != i){

                    pthread_mutex_lock(&mutex);

                    if(deque_array[i].high > 0){
                        task = deque_array[i].deque[deque_array[i].high -1];
                        deque_array[i].high--;
                        manager.pop_back();
                        task_check = true;
                    }
                    pthread_mutex_unlock(&mutex);
                    

                    if(task_check){
                        task();
                        pthread_cond_signal(&cond);
                        break;
                    }
                }
            }

        }

        if(!task_check){

            pthread_mutex_lock(&mutex);
            if(manager.size() > 1){
                while(deque_array[*id].high < 1){

                    pthread_cond_wait(&cond,&mutex);
                    
                }
                      
            }

            else{
                while(wake_up_cond()){
                    pthread_cond_wait(&cond,&mutex);
                }
                stop = true;
                pthread_cond_broadcast(&cond);
            }  
            pthread_mutex_unlock(&mutex);

            }
            
        }


    int tout = *(int*) pthread_getspecific(key[*id]);

    printf("thread %d exiting\n",tout);
    pthread_cleanup_push(thread_exit,thread_id);
    pthread_cleanup_pop(1);
    pthread_cond_broadcast(&cond);
    return NULL;

    }

void async(std::function<void()> &&lambda){

    std::function<void()>* task = new std::function<void()>(lambda);
    
    srand(time(NULL));
    int thread_id = rand()%num_threads;

    pthread_mutex_lock(&mutex);
    //printf("asy succ\n");

    if(deque_array[thread_id].high > 4999999){
        printf("Deque buffer overflow for thread %d. Exiting...\n",thread_id);
        
        for(int i = 0; i< num_threads; i++){
            if( i!= thread_id){
                pthread_exit(&thread_pool[i]);
            }
        }

        pthread_exit(&thread_pool[thread_id]);
        perror("zamn\n");
    }

    
    add_first(thread_id);
    deque_array[thread_id].deque[0] = *task;
    //deque_array[thread_id].high++;

    // deque_array[thread_id].deque[deque_array[thread_id].high] = *task;
    // deque_array[thread_id].high++;
    
    manager.push_back(1);

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);

}

void start_finish(){
    for(int i = 0; i < num_threads; i++){
        int *a = (int *) malloc(sizeof(int));
        *a = i;
        void* data = reinterpret_cast<void*>(a);
        pthread_key_create(&key[i],NULL);
        pthread_create(&thread_pool[i],NULL,&thread_func,data);
    }
    printf("start finish success\n");

}


void end_finish(){

    breaking = 1;
    pthread_cond_broadcast(&cond);

    for(int i = 0; i < num_threads; i++){
        pthread_join(thread_pool[i],NULL);
    }

    printf("end finish success\n");
}

void init_runtime(){
    char* value = getenv("QUILL_WORKERS");
    if(value != NULL){
        num_threads = atoi(value);
        printf("number of threads created %d\n",num_threads);
    }
    thread_pool = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
    deque_array = (deque *) malloc(sizeof(deque)*num_threads);
    key = (pthread_key_t *) malloc(sizeof(pthread_key_t)*num_threads);
    for(int i = 0; i < num_threads; i++){

        deque_array[i].high = 0;
    }

    printf("init runtime success\n");
}

void finalize_runtime(){
    free(thread_pool);
    free(deque_array);
    free(key);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    printf("finalize runtime success\n");
}