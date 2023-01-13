#include <pthread.h>
//#include "../include/estructuras.h"
#define NB_THREADS 5 //numero de hilos basicos
#define DIR_PAGE_TABLE 0x20EB00
#define ULT_DIR_KERNEL 0x3FFFFC
#define PRIMERA_DIR    0x000000
#define TAMAGNO_TLB 4
#define TAM_PAL 4

pthread_t * basic_threads;

pthread_mutex_t m_clk, m_disp, m_load;
pthread_cond_t c1_clk, c2_clk, c1_disp, c2_disp, c1_load, c2_load;





// Se almacena los punteros de los ELF
typedef struct mm
{
    unsigned char* pgb;
    unsigned char* code;
    unsigned char* data;
    
}mm;

typedef struct PCB
{   
    // Ya que solo vamos a tener pid positivos le añadimos el unsigned para tener valores positivos
    unsigned int pid;
    mm * mm;


}PCB;

// En la estructura TLB se almacenara el valor de las paginas virtuales, el valor de la direccion fisica y las veces que ha sido referenciado
typedef struct  TLB
{
    unsigned int dir_virt[TAMAGNO_TLB]; 
    unsigned int dir_fisi[TAMAGNO_TLB]; 
    int vecesReferenciado[TAMAGNO_TLB]; 
}TLB;


typedef struct Hilo
{
    int i;
    int tid;
    pthread_t pthread;
    PCB * pcb;
    unsigned char *pc;
    unsigned int ri;
    unsigned char *ptbr;
    int r_general[16];
    TLB *tlb;
}Hilo;

typedef struct Core
{
    int i;
    Hilo *hilos;
}Core;

typedef struct CPU
{
    int i;
    Core *cores;
}CPU;


typedef struct Machine
{
    CPU *cpus;
    int elf; 
    int ult_elf;
}Machine;

pthread_t * threadMaquina;
pthread_mutex_t m_clk, m_disp, m_load;
pthread_cond_t c1_clk, c2_clk, c1_disp, c2_disp, c1_load, c2_load;


Machine machine;

Hilo **hilos;

unsigned char physical[0xFFFFFF];


