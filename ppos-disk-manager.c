//Código reconstruído via decompilação do objeto ppos-disk-manager.o
//autor: Pedro H. C. Fallgatter

#include "ppos.h"
#include "ppos-disk-manager.h"
#include "ppos-core-globals.h"
#include "disk-driver.h"
#include <signal.h>

void bodyDiskManager(void *arg);
void diskSignalHandler(int signum);

disk_t disco;
struct sigaction diskAction;
task_t taskDiskMgr;

int disk_mgr_init(int *numBlocks, int *blockSize){
  int qtdBlocos; 
  int tamBloco;

  if ((int)disk_cmd(0, 0, 0) < 0)
    return -1;
  qtdBlocos = disk_cmd(4, 0, 0);
  tamBloco = disk_cmd(5, 0, 0);
  if (qtdBlocos < 0 || tamBloco < 0)
    return -1;
  *numBlocks = qtdBlocos;
  *blockSize = tamBloco;
  disco.numBlocks = qtdBlocos;
  disco.blockSize = tamBloco;
  disco.diskQueue = 0;
  disco.requestQueue = 0;
  disco.livre = 1;
  disco.sinal = 0;
  sem_create(&disco.semaforo, 1);
  sem_create(&disco.semaforo_queue, 1);
  task_create(&taskDiskMgr, bodyDiskManager, 0);
  --countTasks;
  diskAction.sa_handler = diskSignalHandler;
  sigemptyset(&diskAction.sa_mask);
  diskAction.sa_flags = 0;
  if (sigaction(SIGUSR1, &diskAction, 0) < 0)
  {
    perror("Erro em sigaction: ");
    exit(1);
  }
  //signal(SIGSEGV, clean_exit_on_sig);
  return 0;
}

int disk_block_read(int block, void *buffer){
  diskrequest_t *request;

  if ((int)sem_down(&disco.semaforo) < 0)
    return -1;
  request = (diskrequest_t *)malloc(0x28u);
  request->task = taskExec;
  request->operation = 1;
  request->block = block;
  request->buffer = buffer;
  request->next = 0;
  request->prev = 0;
  sem_down(&disco.semaforo_queue);
  queue_append((queue_t**)&disco.requestQueue, (queue_t*)request);
  sem_up(&disco.semaforo_queue);
  if (taskDiskMgr.state == 83)
    task_resume(&taskDiskMgr);
  if ((unsigned int)sem_up(&disco.semaforo))
    return -1;
  task_suspend(taskExec, &disco.diskQueue);
  task_yield();
  return 0;
}

int disk_block_write(int block, void *buffer){
  diskrequest_t *request;

  if ((int)sem_down(&disco.semaforo) < 0)
    return -1;
  request = (diskrequest_t *)malloc(0x28u);
  request->task = taskExec;
  request->operation = 2;
  request->block = block;
  request->buffer = buffer;
  request->next = 0;
  request->prev = 0;
  sem_down(&disco.semaforo_queue);
  queue_append((queue_t **)&disco.requestQueue, (queue_t *)request);
  sem_up(&disco.semaforo_queue);
  if ( taskDiskMgr.state == 83 )
    task_resume(&taskDiskMgr);
  if ( (unsigned int)sem_up(&disco.semaforo) )
    return -1;
  task_suspend(taskExec, &disco.diskQueue);
  task_yield();
  return 0;
}

void bodyDiskManager(void *arg){
  diskrequest_t *request;

  while ( 1 )
  {
    sem_down(&disco.semaforo);
    if ( disco.sinal )
    {
      disco.sinal = 0;
      task_resume(disco.diskQueue);
      disco.livre = 1;
    }
    if (disco.livre)
    {
      if (disco.requestQueue)
      {
        request = (diskrequest_t *)disk_scheduler(disco.requestQueue);
        if ( request )
        {
          sem_down(&disco.semaforo_queue);
          queue_remove((queue_t**)&disco.requestQueue, (queue_t*)request);
          sem_up(&disco.semaforo_queue);
          if ( request->operation == 1 )
          {
            disk_cmd(1, (unsigned int)request->block, request->buffer);
            disco.livre = 0;
          }
          else if ( request->operation == 2 )
          {
            disk_cmd(2, (unsigned int)request->block, request->buffer);
            disco.livre = 0;
          }
          free(request);
        }
      }
    }
    sem_up(&disco.semaforo);
    task_yield();
  }
}

void diskSignalHandler(int signum){
  disco.sinal = 1;
}