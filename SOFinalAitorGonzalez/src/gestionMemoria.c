#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#define NB_THREADS 5 //numero de hilos basicos
#define DIR_PAGE_TABLE 0x20EB00
#define ULT_DIR_KERNEL 0x3FFFFC
#define PRIMERA_DIR    0x000000
#define TAMAGNO_TLB 4
#define TAM_PAL 4

// se utilizan operaciones de máscara de bits para extraer los 3 bytes menos significativos de la 
// dirección física en lugar de operaciones de desplazamiento de bits. Esto hace que el código sea más fácil de leer y entender.
// Se hace uso de & operator con una máscara de 0xFF (o 255 en decimal)
// para obtener los últimos 8 bits del número que corresponde al byte menos significativo de un número de 32 bits.
// Finalmente, se agrega la instrucción de incrementar la dirección física al final del bucle.

void iniciarPageTable() {
    int pte = 0x00400000;

    for (int i = 0; i <= (49151 * 4); i += 4) {
        int a = pte & 0xFF; 
        int b = (pte >> 8) & 0xFF; 
        int c = (pte >> 16) & 0xFF; 

        // Almacenar los bytes en la tabla de páginas
        physical[0x20EB00 + i] = 0x00;
        physical[0x20EB00 + i + 1] = a;
        physical[0x20EB00 + i + 2] = b;
        physical[0x20EB00 + i + 3] = c;
        
        pte += 256;
    }
}
/*
Este código está leyendo un archivo ELF desde un directorio llamado "./elfs/". 
Comienza abriendo el directorio utilizando la función opendir(). Si el directorio se abre correctamente, 
declara una variable llamada "directorio" que se utilizará para recorrer los archivos en el directorio. 
Luego, crea una variable llamada "num_elf" que es una lista de caracteres de tamaño 3, y una variable llamada "n_elf" que es un entero.
A continuación, utiliza un bucle while para leer todos los archivos en el directorio utilizando la función readdir(). 
Dentro del bucle, extrae los tres dígitos del nombre del archivo y lo almacena en la variable "num_elf". 
Luego, convierte la variable "num_elf" a un entero y la almacena en la variable "n_elf".
A continuación, compara el valor de "n_elf" con el valor actual de "machine.ult_elf" y si el valor de "n_elf" es mayor que "machine.ult_elf" 
o machine.elf es igual a cero, asigna el valor de "n_elf" a "machine.ult_elf". Este proceso se realiza para todos los archivos en el directorio.
Finalmente, después de que el bucle haya terminado, el código cierra el directorio utilizando la función closedir().
*/

void leerELF(){
    DIR * directorio = opendir("./elfs/");
    if(directorio!=NULL){
        struct dirent *entry;
        char num_elf[3]= "   ";
        int n_elf;
        // Vamos a recorrer el directorio completo
        while ((entry = readdir(directorio))!=NULL)
        {   
            num_elf[2] = entry->d_name[6];
            num_elf[1] = entry->d_name[5];
            num_elf[0] = entry->d_name[4];

            n_elf = strtol(num_elf,NULL,10);

            if(n_elf>machine.ult_elf || machine.elf==0){

                machine.ult_elf = n_elf;
            }  

        }

        closedir(directorio);
        
    }
}

/*
Este código está cargando un archivo ELF (Formato Ejecutable y Enlazable) desde una ruta específica ("./elfs/progXXX.elf")
donde XXX es un número de tres dígitos. Abre el archivo, obtiene su tamaño en bytes y lee las primeras dos líneas del archivo,
que contienen las direcciones de memoria de las secciones ".text" y ".data" del archivo ELF. Luego, calcula el número de instrucciones en el archivo
y el número de páginas necesarias para almacenarlas en la memoria. El código luego busca en la Tabla de Páginas por páginas vacías y
guarda las direcciones de esas páginas.
*/
void cargarELF(int pids, linkedList processQueue,  PCB * pcb){
    char path[19] = "./elfs/progXXX.elf\0";
    path[13] = '0'+machine.elf%10;
    path[12] = '0'+machine.elf/10;
    path[11] = '0'+machine.elf/100;
    int fd;
    if((fd = open(path, O_RDONLY) )<0){
        exit(-1);
    }
    struct stat sta;
    int tamagno = 0;
    stat(path, &sta);
    tamagno = sta.st_size;
    printf("\nEste el el tamaño del fichero en bytes,  %d",tamagno);
    tamagno = tamagno -26; 
    int longlinea = 13;
    //vamos a leer la primera linea del fichero
    char buff[longlinea];
    read(fd, buff, longlinea);
    buff[12] = '\0'; 
    char dirTxt[7];
    int i;
    for(i=0;i<6;i++){
        dirTxt[i]=buff[6+i];
    }
    dirTxt[6]='\0';
    //vamos a leer la segunda linea del fichero
    read(fd, buff, longlinea);
    buff[12] = '\0'; 
    char dirData[7];
    for(i=0;i<6;i++){
        dirData[i]=buff[6+i];
    }
    dirData[6]='\0';


    int numInstruc = tamagno / 9;
    tamagno = numInstruc * 4;
    int numPag = numInstruc/64 + 1;
    int pagObjectivo, contador=0;
    printf("\nNumero de pagina %d\n", numPag);
    unsigned int *dir_pte = malloc(sizeof(unsigned int)*numPag);
    for(i=0; i < 49152 *4; i+=4){
        if(physical[(0x20EB00 + i)] !=1){
            dir_pte[contador] = 0x20EB00 + i;
            contador++;
            if (contador==numPag)break;
        }
    }
    if(contador!=numPag){
        machine.elf--;
        return; 
    }
    for(i = 0; i<numPag; i++){
        physical[dir_pte[i]] = 1;
    }

    unsigned int a, b, c;
    a = physical[dir_pte[0]+1]; 
    b = physical[dir_pte[0]+2]; 
    c = physical[dir_pte[0]+3]; 

    int direccion_codigo = 0;
    direccion_codigo += a;
    direccion_codigo = (direccion_codigo << 8) + b;
    direccion_codigo = (direccion_codigo << 8) + c;

    pcb->mm = malloc(sizeof(mm));
    pcb->mm->code = &(physical[direccion_codigo]); 
    int data_d = (int)strtol(dirData, NULL, 7);
    direccion_codigo = direccion_codigo + data_d;
    pcb->mm->data = &(physical[direccion_codigo]);  
    pcb->mm->pgb = &(physical[(dir_pte[0])]); 
    direccion_codigo = direccion_codigo - data_d;
    char * buffer = malloc(sizeof(char)*9); 
    char hexa[3];
    hexa[2]='\0';
    unsigned char pal;
    
    while ((read(fd, buffer, 8)) > 0) {
        buffer[8]='\0';

        for(i=0; i<4;i++){
            hexa[0]= buffer[i*2];
            hexa[1]= buffer[i*2+1];
            pal = (unsigned char)strtol(hexa, NULL, 16);
            physical[direccion_codigo++] = pal;

        }
        read(fd, buffer, 1);
    }
    
    close(fd);
    free(dir_pte);
    free(buffer); 

}

//carga los valores del pcb al hilo
void cargarHilo(Hilo *hilo, PCB *pcb){
    printf("\nhilo : %d, cargado", hilo->tid);
    hilo->pcb = pcb;
    //se asigna la direccion inicial
    hilo->pc = pcb->mm->code; 
    hilo->ri = 0;
    hilo->ptbr = pcb->mm->pgb;
    //reseteamos registros y tlb
    for(int i=0; i<16;i++){
        hilo->r_general[i]=0;
    }
    for(int i=0; i<TAMAGNO_TLB;i++){
        hilo->tlb->dir_fisi[i]= -1;
        hilo->tlb->dir_virt[i]= -1;
    }
}

/*
Resetea los valores
*/
void limpiarHilo(Hilo *hilo){
    printf("\nhilo : %d, limpiado", hilo->tid);
    hilo->pc = NULL; 
    hilo->ri = -1;
    hilo->pcb = NULL;
    hilo->ptbr = NULL;
    for(int i=0; i<16;i++){
        hilo->r_general[i]=0;
    }
    for(int i=0; i<TAMAGNO_TLB;i++){
        hilo->tlb->dir_fisi[i]= -1;
        hilo->tlb->dir_virt[i]= -1;
        hilo->tlb->vecesReferenciado[i]=-1;
    }

}


/*
Este código implementa una tabla de páginas y una tabla de traducción (TLB) para un sistema de memoria virtual.
 Recupera la instrucción del contador de programa (PC) y recupera el código de operación de la instrucción. 
 El código de operación se utiliza para determinar el tipo de instrucción, como una LOAD, STORE, ADD o EXIT. 
 Si la instrucción es una LOAD o STORE, el código recupera la dirección virtual de la instrucción y la utiliza para buscar
  la dirección física correspondiente en la TLB. Si no se encuentra la dirección virtual en la TLB, el código busca en la tabla de páginas y 
  actualiza la TLB con la dirección física.*/


int ejecFases(Hilo *hilo){

    int i;

    hilo->ri = 0;
    for(i = 0;i<TAM_PAL-1;i++){

        hilo->ri += *(hilo->pc + i);
        
        hilo->ri = hilo->ri <<8;

    }

    hilo->ri += *(hilo->pc + 3);
    hilo->pc +=4; 
    char a = hilo->ri >> 28; 
    char ADD[3] = "ADD";
    char EXIT[3] = "EXT";
    char LOAD[3] = " LD";
    char STORE[3] = " ST";
    unsigned char b, c, d ,e;
    unsigned int dir_virtual = -1;


    switch (a) {
        case 0:
        case 1:
            b = hilo->ri >> 28;
            dir_virtual = hilo->ri & 0x0FFFFFFF;
            break;
        case 2:
            b = hilo->ri >> 28;
            c = (hilo->ri >> 24) & 0xF;
            d = (hilo->ri >> 20) & 0xF;
            break;
        case 0xF:
            break;
        default:
            break;
    }

    unsigned int pagina, pagVirtual;
    unsigned int direcFisica = 0, direcFin=0;
    if (dir_virtual!=-1){
        pagina = (dir_virtual << 24) >>24 ; 
        pagVirtual  = dir_virtual >> 8 ;
        int encontrado =0;
        int posicion = -1;
        for(i=0; i<TAMAGNO_TLB;i++){
            if(hilo->tlb->dir_virt[i]==pagVirtual){
                encontrado = 1;
                hilo->tlb->vecesReferenciado[i] ++; 
                posicion = i;
                printf("\nla direccion fisica de la tbl es %x\n", hilo->tlb->dir_fisi[i]);
                break;
            }
        }
        

        unsigned char d0, d1, d2, d3;
        if (encontrado){ 
            printf("\nla direccion fisica es %x\n", direcFisica);
            direcFisica = ((hilo->tlb->dir_fisi[posicion] << 8) >> 8) + pagina; 
            for(i = 0; i<TAM_PAL-1;i++){
                direcFin += physical[direcFisica + i];
                direcFin = direcFin << 8;
            }
            direcFin += physical[direcFisica + 3];
            
        }else{
        int minimo = 1000, pos=-1;
        for(i=0;i<TAMAGNO_TLB;i++){
            if(hilo->tlb->vecesReferenciado[i]<minimo){
                minimo = hilo->tlb->vecesReferenciado[i];
                pos = i;
            }
        }

        hilo->tlb->dir_virt[pos]=pagVirtual;
        hilo->tlb->vecesReferenciado[pos]=-1;

        unsigned char * pte = hilo->ptbr + (pagVirtual * 4);

        unsigned int te = 0;
        for(i=0;i<TAM_PAL;i++){

            te+=*(pte +i);
   
            te = te <<8;


        }

        te = te >> 8;
 
        hilo->tlb->dir_fisi[pos]= te;

        direcFisica = (hilo->tlb->dir_fisi[pos] << 8) >> 8 + pagina;
        direcFisica = hilo->tlb->dir_fisi[pos] + pagina;

        for(i = 0; i<TAM_PAL-1;i++){
                direcFin += physical[direcFisica + i];
                direcFin = direcFin << 8;
            }
        direcFin += physical[direcFisica + 3];
        
        }
    }

    switch(a){
        // Caso Load
        case 0:
            c = direcFin;
            hilo->r_general[b] = c;
            break;
        // Caso Store
        case 1:
            b = hilo->r_general[c];
            int bb, cc, dd, ee;
            bb = cc = dd = ee = b;
            bb = bb >> 24;
            physical[direcFisica + 0] = bb;
            cc = (cc << 8) >>24;
            physical[direcFisica + 1] = cc;
            dd = (dd << 16) >> 24;
            physical[direcFisica + 2] = dd;
            ee = (ee << 24)>> 24;
            physical[direcFisica + 3] = ee;
            break;
        // Caso suma
        case 2:
            hilo->r_general[b] = hilo->r_general[c]+hilo->r_general[d];
            break;
        // Caso exit
        case 0xF:
            return(-1);
            break;
        default:
            break;

    }

    return(0);
}
