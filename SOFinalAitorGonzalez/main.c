#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "include/estructuras.h"
#include "include/linkedList.h"
#include "src/gestionMemoria.c"
#define NB_THREADS 5 //numero de hilos basicos
#define DIR_PAGE_TABLE 0x20EB00
#define ULT_DIR_KERNEL 0x3FFFFC
#define PRIMERA_DIR    0x000000
#define TAMAGNO_TLB 4
#define TAM_PAL 4

linkedList processQueue;
int contador_p = 0, killed=0;
void obtener_parametros(int argc, char *argv[]);
void inic_machine();
void inic_basic_threads();
void *clk();
void *crearHilo();
void *timScheduler();
void *timLoader();
void *loader();
void *scheduler();
int  numCpus, numCores, numThreads, numTotalThreads, done_clk, frec_loader, frec_dis;
bool doneDisp, doneLoader;
int main(int argc, char *argv[])
{
    if (argc != 6){
        printf("introduce los siguientes 5 parametros numCpus, numCores, numThreads, frec_dis, frec_loader");
        exit(0);
    }

    numCpus = atoi(argv[1]);
    numCores = atoi(argv[2]);
    numThreads = atoi(argv[3]);
    frec_dis = atoi(argv[4]);
    frec_loader = atoi(argv[5]);   
    if (frec_dis == frec_loader){
        frec_loader++;
    }
    numTotalThreads = numCores*numCpus*numThreads;
    int i, j, k, l, cont=0;
    srand(time(NULL));
    inicializar_lista(&processQueue);
    pthread_mutex_init(&m_clk, NULL);
    pthread_cond_init(&c1_clk, NULL);
    pthread_cond_init(&c2_clk, NULL);
    pthread_mutex_init(&m_disp, NULL);
    pthread_cond_init(&c1_disp, NULL);
    pthread_cond_init(&c2_disp, NULL);
    pthread_mutex_init(&m_load, NULL);
    pthread_cond_init(&c1_load, NULL);
    pthread_cond_init(&c2_load, NULL);
    basic_threads = malloc(sizeof(pthread_t)*NB_THREADS);
    pthread_create(&basic_threads[0], NULL, clk, NULL);
    hilos = malloc(sizeof(struct Hilo *)*numTotalThreads);

    machine.cpus = (CPU*)malloc(sizeof(CPU) * numCpus);
    for (i = 0; i < numCpus; i++) {
        machine.cpus[i].cores = (Core*)malloc(sizeof(Core) * numCores);
        for (j = 0; j < numCores; j++) {
            machine.cpus[i].cores[j].hilos = (Hilo*)malloc(sizeof(Hilo) * numThreads);
            for (k = 0; k < numThreads; k++) {
                pthread_create(&machine.cpus[i].cores[j].hilos[k].pthread, NULL, crearHilo, &machine.cpus[i].cores[j].hilos[k]);
                machine.cpus[i].cores[j].hilos[k].i=cont/numThreads;
                machine.cpus[i].cores[j].hilos[k].tid=cont;
                machine.cpus[i].cores[j].hilos[k].pcb=NULL;
                machine.cpus[i].cores[j].hilos[k].tlb = malloc(sizeof(TLB));

                for(l=0; l<TAMAGNO_TLB;l++){
                    machine.cpus[i].cores[j].hilos[k].tlb->dir_fisi[l]= -1;
                    machine.cpus[i].cores[j].hilos[k].tlb->dir_virt[l]= -1;
                    machine.cpus[i].cores[j].hilos[k].tlb->vecesReferenciado[l]= -1;
                }
                
                hilos[cont]= &machine.cpus[i].cores[j].hilos[k];
                
                cont++;
                
            }
        }
    }

    iniciarPageTable();
    pthread_create(&basic_threads[1], NULL, timScheduler, NULL);
    pthread_create(&basic_threads[2], NULL, timLoader, NULL);
    pthread_create(&basic_threads[3], NULL, loader, NULL);
    pthread_create(&basic_threads[4], NULL, scheduler, NULL);
    pthread_join(basic_threads[0], NULL);
    pthread_join(basic_threads[1], NULL);
    pthread_join(basic_threads[2], NULL);
    pthread_join(basic_threads[3], NULL);
    pthread_join(basic_threads[4], NULL);
    printf("\nHemos finalizado");
    return 0;
}





void* clk(){

    done_clk = 0;
    while (1)
    { 
        // Primero bloqueamos todo los hilos, esto quiere decir que todos los hilos han acabado
        pthread_mutex_lock(&m_clk);

        while (done_clk < numTotalThreads + NB_THREADS -3 -killed){
            // Esperamos a que todos los hilos se ejecuten, que nos manden el flag en la condicion de que han acabado los hilos
            pthread_cond_wait(&c1_clk, &m_clk);          
        }
        done_clk = 0;
        // Primero se lo manda a todos, que ya tienen disponible
        pthread_cond_broadcast(&c2_clk);
        // El segundo desbloquea el mutex y va a volver al while
        pthread_mutex_unlock(&m_clk);
        
    }
    

}


void *crearHilo(Hilo *hilo){

    pthread_mutex_lock(&m_clk);
    int resultado = 0;
    sleep(1);
    printf("\n hilo creado con el pid: %d", hilo->tid);

    while (1)
    {       
        done_clk++;

       if(hilo->pcb!=NULL){
           if (ejecFases(hilo)<0){
                limpiarHilo(hilo);
                contador_p ++;
                if(contador_p == machine.ult_elf+1){
                    killed++;
                    pthread_cond_signal(&c1_clk);
                    pthread_mutex_unlock(&m_clk);
                    pthread_exit(NULL);
                }

           }
    
       }
        pthread_cond_signal(&c1_clk);
        
        pthread_cond_wait(&c2_clk, &m_clk);


    }
    
}

void * timScheduler(){
    // Este timer se activa gracias al reloj, por tanto necesamos un mutex para permitir esa comunicacion
    // Por un lado tenemos la comunicacion clock timer dispacher
    // por otro tenemos el timer dispacher con el dispacher
    int contador = 0, periodo = 0;
    pthread_mutex_lock(&m_clk);
    periodo = frec_dis;
    while(1){
        done_clk++;
        contador++;
        // printf("\nContador= %d", contador);
        // Cada vez que se despierta el hilo la variable contador suma 1, si el contador es == al periodo aleatorio 
        // entonces se le llama al process generator para que genere un nuevo proceso
        if (contador == periodo){
            printf("\nHa saltado el temporizador del dispacher");
            pthread_mutex_lock(&m_disp);
            while (!doneDisp){
            
                pthread_cond_wait(&c1_disp, &m_disp);
            
            }
            doneDisp=false;

            pthread_cond_broadcast(&c2_disp);
            pthread_mutex_unlock(&m_disp);

            periodo = frec_dis;
            contador = 0;
        }

        pthread_cond_signal(&c1_clk);
        pthread_cond_wait(&c2_clk, &m_clk);
        
    }
}

void *scheduler(){
    int i;
    pthread_mutex_lock(&m_disp);
        while(1){
            doneDisp=true;
            printf("\nscheduler= %d", doneDisp);
            // Vamos a comprobar si ya hemos acabado para finalizar los hilos
            if (machine.ult_elf+1 == contador_p){
                int i;
                for(i=0;i<numTotalThreads;i++){
                    pthread_join(hilos[i]->pthread, NULL);
                }
                for(i=0;i<numTotalThreads;i++){
                    if (i!=4){pthread_join(basic_threads[i], NULL);}    
                }
                pthread_cond_signal(&c1_disp);
                pthread_mutex_unlock(&m_disp);
                pthread_exit(NULL);
            }

            // Vamos a comprobar que el loader no este usando el processQueueu
            if(processQueue.estado==0){
                // A continuacion vamos a bloquear
                processQueue.estado = 1;
                for(i = 0; i<numTotalThreads;i++){
                    //si sigue existiendo algun pcb encolado
                    if(processQueue.tamagno>0){ 

                        if(hilos[i]->pcb == NULL){
                            printf("\n Pc:code %d\n", *processQueue.primero->pcb->mm->code);
                            
                            cargarHilo(hilos[i], obtenerNodo(&processQueue));
                            printf("\nPCB PID: %d",hilos[i]->pcb->pid);
                        }
                    
                    }else{
                        break;
                    }

                }

                processQueue.estado=0;
            }

            
            pthread_cond_signal(&c1_disp);
            pthread_cond_wait(&c2_disp, &m_disp);
        
        }

}

void * timLoader(){
    int contador = 0, periodo = 0;
    pthread_mutex_lock(&m_clk);
    periodo = frec_loader;
    while(1){
        done_clk++;
        contador++;
        if (contador == periodo){
            printf("\nHa saltado el temporizador del loader");
            pthread_mutex_lock(&m_load);
            while (!doneLoader){
            
                pthread_cond_wait(&c1_load, &m_load);
            
            }
            doneLoader=false;

            pthread_cond_broadcast(&c2_load);
            pthread_mutex_unlock(&m_load);

            periodo = frec_loader;
            contador = 0;


        }


        pthread_cond_signal(&c1_clk);
        pthread_cond_wait(&c2_clk, &m_clk);
        
    }
}

void *loader(){
    int pids = 0;
    machine.elf = -1;
    leerELF();
    pthread_mutex_lock(&m_load);
        while(1){
            doneLoader=true;
            printf("\nLoader= %d", doneLoader);
            if (machine.ult_elf+1 == contador_p){
                int k;
                for(k=0;k<numTotalThreads;k++){
                    pthread_join(hilos[k]->pthread, NULL);
                }
                for(k=0;k<numTotalThreads;k++){
                    if (k!=4){
                        pthread_join(basic_threads[k], NULL);
                    }
                }
                pthread_cond_signal(&c1_load);
                pthread_mutex_unlock(&m_load);
                pthread_exit(NULL);
            }
            if(processQueue.estado == 0){
                processQueue.estado = 1;
                PCB * pcb = (PCB* )malloc(sizeof(PCB));
                pcb->pid=pids;
                machine.elf++;
                cargarELF(pids, processQueue, pcb);
                pids++;
                anadirNodo(&processQueue, pcb);
                printf("\n %s", "tamaño de la process queue");
                printf("\n %d", processQueue.tamagno);
                processQueue.estado = 0;
            }
            pthread_cond_signal(&c1_load);
            pthread_cond_wait(&c2_load, &m_load);
        }

}


