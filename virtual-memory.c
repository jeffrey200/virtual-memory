#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "virtual-memory.h"

//Virtual Memory
//RAM Size: 64MB
//Block Size: 4MB
//Used Memory:
//  isinmemory:     256 * 1bit => 32 bytes
//  lastused:       256 * 4bytes
//  mappedaddress:  256 * 1byte
//  padding:        256 * 3byte
//  Total:          2080bytes

#define MB  1024 * 1024
#define RAM_SIZE    64 * MB


volatile uint32_t timer = 0;

char *buffer;

char *isinmemory;
struct page *pages;

struct page
{
    uint32_t lastused;
    uint8_t mappedaddress;
};



int main(){

    pthread_t thread1, thread2;

    printf("starting threads...\n");
    pthread_create(&thread1, NULL, incTimer, NULL);
    pthread_create(&thread2, NULL, setup, NULL);

    pthread_join(thread2, NULL);
    printf("done\n");

    return 0;

}



void *setup(void *vargp){

    buffer = calloc(RAM_SIZE, 1);

    isinmemory = (char *) calloc(32, 1);
    pages = (struct page *) calloc(256, sizeof(struct page));

    for(int i=0; i<256; i++){
        if(i<16){
            setBit(isinmemory, i);
            pages[i].mappedaddress = i;
        }else{
            pages[i].mappedaddress = 0xff;
        }
    }

    printf("start test program\n");

    writeToMemory(1024, 0x88);
    setLastUsed(0, 0);
    writeToMemory(18 * 4 * MB + 1024, 0xfe);


    printf("%x\n", readFromMemory(1024));
    printf("%x\n", readFromMemory(18 * 4 * MB + 1024));

    return NULL;
}



//Main Functions

void handlePageFault(uint8_t pagenr){
    uint8_t unusedpage = getUnusedPage();
    uint8_t realunused = getMappedAddressInMemory(unusedpage);
    if(!isPageEmpty(unusedpage)){
        writePageToDisk(unusedpage);
    }else{
        setIsPageInMemory(unusedpage, 0);
        setMappedAddressInMemory(unusedpage, 255);
    }
    loadPageFromDisk(pagenr, realunused);
}

char readFromMemory(uint32_t address){
    uint8_t page = addressToPageNr(address);
    if(isPageInMemory(page)){
        uint32_t offset = address - (4 * MB * page);
        setLastUsed(page, timer);
        char *buffer = getAddressPage(page);
        char byte = buffer[offset];
        free(buffer);
        return byte;
    }else{
        handlePageFault(page);
        return readFromMemory(address);
    }
}

void writeToMemory(uint32_t address, char byte){
    uint8_t page = addressToPageNr(address);
    if(isPageInMemory(page)){
        uint32_t offset = address - (4 * MB * page);
        uint32_t newaddr = getMappedAddressInMemory(page) * 4 * MB + offset;
        setLastUsed(page, timer);
        buffer[newaddr] = byte;
    }else{
        handlePageFault(page);
        writeToMemory(address, byte);
    }
}



//Page swapping function

void writePageToDisk(uint8_t pagenr){
    char str_pagenr[3];
    sprintf(str_pagenr, "%d", pagenr);
    setIsPageInMemory(pagenr, 0);
    char *buffer = getAddressPage(pagenr);
    writeFileToDisk(str_pagenr, 4 * MB, buffer);
    setMappedAddressInMemory(pagenr, 255);
    free(buffer);
}

void loadPageFromDisk(uint8_t pagenr, uint8_t startaddr){
    char str_pagenr[3];
    sprintf(str_pagenr, "%d", pagenr);
    char *buf = readFileFromDisk(str_pagenr, 4 * MB);
    writeBytesToBuffer(buf, 4 * MB, startaddr * 4 * MB);
    setIsPageInMemory(pagenr, 1);
    setMappedAddressInMemory(pagenr, startaddr);
    free(buf);
}



//File functions

void writeFileToDisk(char *name, uint32_t length, char *array){
    FILE *nfile = fopen(name, "wb");
    fwrite(array, length, 1, nfile);
    fclose(nfile);
}

//Memory needs to be free'd
char *readFileFromDisk(char *name, uint32_t length){
    char *array;
    FILE *nfile = fopen(name, "rb");
    if(nfile == NULL){
        array = (char *) calloc(length, 1);
    }else{
        array = (char *) malloc(length);
        fread(array, length, 1, nfile);
        fclose(nfile);
    }
    return array;
}



//Page functions

uint8_t addressToPageNr(uint32_t address){
    for(int i=0;i<256;i++){
        uint32_t min = i * 4 * MB;
        uint32_t max = (i+1) * 4 * MB;
        if(address >= min && address < max)
            return i;
    }
    printf("Address out of range\n");
    return 0;
}

//Memory needs to be free'd
char *getAddressPage(uint8_t pagenr){
    uint8_t startaddr = getMappedAddressInMemory(pagenr);
    uint32_t min = startaddr * 4 * MB;
    uint32_t max = min + 4 * MB;
    return sliceArray(buffer, 4 * MB, min, max);
}

uint8_t isPageEmpty(uint8_t pagenr){
    char *page = getAddressPage(pagenr);
    uint8_t isempty = 1;
    for(int i=0; i<(4 * MB); i++){
        if(page[i] != 0){
            isempty = 0;
            break;
        }
    }
    free(page);
    return isempty;
}

uint8_t getUnusedPage(){
    uint32_t longesttime = 0;
    uint8_t longest = 0;
    for(int i=0; i<256; i++){
        if(isPageInMemory(i)){
            uint32_t timediff = timer - getLastUsed(i);
            if(timediff > longesttime){
                longesttime = timediff;
                longest = i;
            }
        }
    }
    if(longest == 0 && !isPageInMemory(0))
        printf("This should not have happened\n"); //TODO
    return longest;
}



//Basic utility functions

uint8_t isPageInMemory(uint8_t pagenr){
    return getBit(isinmemory, pagenr);
}

void setIsPageInMemory(uint8_t pagenr, uint8_t isin){
    if(isin)
        setBit(isinmemory, pagenr);
    else
        clearBit(isinmemory, pagenr);
}

uint8_t isAddressInMemory(uint32_t address){
    return isPageInMemory(addressToPageNr(address));
}

uint32_t getLastUsed(uint8_t pagenr){
    return pages[pagenr].lastused; 
}

void setLastUsed(uint8_t pagenr, uint32_t time){
    pages[pagenr].lastused = time;
}

uint8_t getMappedAddressInMemory(uint8_t pagenr){
    return pages[pagenr].mappedaddress;
}

void setMappedAddressInMemory(uint8_t pagenr, uint8_t address){
    pages[pagenr].mappedaddress = address;
}

void writeBytesToBuffer(char *array, uint32_t length, uint32_t address){
    for(int i=0; i<length; i++){
        buffer[address + i] = array[i];
    }
}



//Timer

void *incTimer(void *vargp){
    while(1){
        usleep(1000);
        timer++;
    }
    return NULL;
}



//Basic help functions

//Memory needs to be free'd
char *sliceArray(char *array, uint32_t arraysize, uint32_t pos1, uint32_t pos2){ 
    char *returnarray = (char *) malloc((pos2 - pos1) * sizeof(char));
    for(int i=pos1; i<pos2; i++)
        returnarray[i - pos1] = array[i];
    return returnarray;
}

int getIndexFromArray(char *array, int nr){
    for(int i=0; i<32; i++)
        if(nr < ((i + 1) * 8))
            return i;
    printf("Number out of range\n");
    return 0;
}

void setBit(char *array, int nr){
    int index = getIndexFromArray(array, nr);
    nr -= (index * 8);
    array[index] |= (1 << nr);
}

void clearBit(char *array, int nr){
    int index = getIndexFromArray(array, nr);
    nr -= (index * 8);
    array[index] &= ~(1 << nr);
}

int getBit(char *array, int nr){
    int index = getIndexFromArray(array, nr);
    nr -= (index * 8);
    return (array[index] & (1 << nr)) == (1 << nr);
}
