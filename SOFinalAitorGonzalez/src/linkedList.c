#include <stdio.h>

typedef struct Nodo
{
    PCB* pcb;
    struct Nodo* sig;

}Nodo;

typedef struct linkedList
{
    int tamagno;
    int estado;
    struct Nodo *primero;
    struct Nodo *ultimo;
}linkedList;

void inicializar_lista(linkedList * list){
    list->tamagno = 0;   
    list->estado = 0;
    list->primero = NULL;
    list->ultimo = NULL;
}

PCB * obtenerNodo(linkedList * list){
    Nodo *node;
    node = list->primero;
    PCB *pcb;
    list->primero = node->sig;
    pcb = node->pcb;
    list->tamagno --;
    free(node);

    return pcb;
}


void anadirNodo(linkedList * list, PCB * pcb){
    int pos = -1;
    int tam = 0;
    struct Nodo* nodo = (struct Nodo*)malloc(sizeof(Nodo));
    if (nodo == NULL){
        perror("malloc"); 
        exit(1);
    }
    nodo->pcb = pcb;
    nodo->sig = NULL;
    tam = list->tamagno;
    if(tam == 0){ 
        list->primero = nodo;
        list->ultimo = nodo;
    }
    else if(pos>=tam || pos <0){
        
        list->ultimo->sig = nodo;
        list ->ultimo = nodo;
    }
    else{
        Nodo *node;
        node = list->primero;
        for (int i=0; i<pos-1; i++){
            node = node->sig;
        }
        nodo->sig = node->sig;
        node->sig = nodo;
    }
    list->tamagno ++;

}



