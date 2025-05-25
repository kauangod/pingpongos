#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"


// ****************************************************************************
// Adicione TUDO O QUE FOR NECESSARIO para realizar o seu trabalho
// Coloque as suas modificações aqui,
// p.ex. includes, defines variáveis,
// estruturas e funções
//
// ****************************************************************************
#include <signal.h>
#include <sys/time.h>
#define ALPHA -1
#define QUANTUM 20
//#define DEBUG

unsigned int _systemTime = 0;
unsigned int dispatcher_activation_count = 0; //  Alternativas pois não é possível acessar as variáveis
unsigned int dispatcher_last_activation_time = 0;// do dispatcher caso a fila não esteja vazia.
unsigned int dispatcher_create_time = 0;
unsigned int dispatcher_processor_time = 0;
unsigned int dispatcher_execution_time = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;

//verifica se é a primeira vez que há a troca,
// para não imprimir as informações do dispatcher antes do fim.
int its_first_time = 0;

// tratador do sinal
void handler (int signum)
{
    _systemTime++; // ms
    if(preemption == 1){
        if(taskExec->id != 1){
            taskExec->ticks--;
            if(taskExec->ticks == 0){
                task_yield();
            }
        }
    }
}

task_t* scheduler (){
    if(readyQueue == NULL){
        printf("Fila de tarefas prontas vazia\n");
        return NULL;
    }
    else if(readyQueue == readyQueue->next){
        return readyQueue;
    }

    task_t *first = readyQueue;
    int maxPrio = 21;
    task_t *taskMaxPrio = NULL;
    task_t *task = first;

    do{
        if(task->prio_d < maxPrio){
            maxPrio = task->prio_d;
            taskMaxPrio = task;
        }
        task = task->next;
    }while(task!=first);

    task = first;

    do{
        if(task != taskMaxPrio){
            task->prio_d = task->prio_d + ALPHA;
        }
        task = task->next;
    }while(task!=first);

    taskMaxPrio->prio_d = taskMaxPrio->prio_s;
    return taskMaxPrio;
}

void task_setprio (task_t *task, int prio){
    if(prio < -20 || prio > 20){
        printf("Prioridade fora do intervalo permitido\n");
    }
    if(task == NULL){
        taskExec->prio_s = prio;
        taskExec->prio_d = prio;
    }
    else{
        task->prio_s = prio;
        task->prio_d = prio;
    }
}

int task_getprio (task_t *task){
    if(task == NULL) {
        return taskExec->prio_s;
    }
    return task->prio_s;
}

unsigned int systime (){
    return _systemTime;
}

void before_ppos_init () {
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_ppos_init () {
    task_setprio(taskMain, 0);
    // Código adaptado do arquivo timer.c
    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = handler ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ){
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create(task_t *task){
    task_setprio(task,0);
    if(task->id == 1){
        its_first_time = 1;
        dispatcher_create_time = _systemTime;
    }
    else{
        task->create_time = _systemTime;
        task->activation_count = 0;
        task->processor_time = 0;
        task->execution_time = 0;
        task->last_activation_time = 0;
    }


#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif

}

void before_task_exit () {
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    taskExec->execution_time = _systemTime - taskExec->create_time;
    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
    taskExec->id, taskExec->execution_time, taskExec->processor_time, taskExec->activation_count);
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    if(task->id != 1){
        task->ticks = QUANTUM;
    }
    if(taskExec->id == 1){
        dispatcher_processor_time += (_systemTime - dispatcher_last_activation_time);
        if (!its_first_time && task->id == 0){
            dispatcher_execution_time = _systemTime - dispatcher_create_time;
            printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
            taskExec->id, dispatcher_execution_time, dispatcher_processor_time, dispatcher_activation_count);
        }   // Infos task dispatcher levando em consideração que ele não utiliza o exit()
           // e perde o processador para a main.
        its_first_time = 0;
    }
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    if (task->id != 1){
        task->last_activation_time = _systemTime;
        task->activation_count++;
    }
    else{
        dispatcher_last_activation_time = _systemTime; // Se é o dispatcher, atualiza o tempo de última ativação
        dispatcher_activation_count++; // e a contagem de ativações com as variáveis globais.
    }
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_yield () {
    taskExec->processor_time += (_systemTime - taskExec->last_activation_time);
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


void before_task_suspend( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
