#include <stdint.h>

//Rotuine functions
void *setup(void *vargp);

//Main Functions
void handlePageFault(uint8_t pagenr);
char readFromMemory(uint32_t address);
void writeToMemory(uint32_t address, char byte);

//Page swapping function
void writePageToDisk(uint8_t pagenr);
void loadPageFromDisk(uint8_t pagenr, uint8_t startaddr);

//File functions
void writeFileToDisk(char *name, uint32_t length, char *array);
char *readFileFromDisk(char *name, uint32_t length);

//Page functions
uint8_t addressToPageNr(uint32_t address);
char *getAddressPage(uint8_t pagenr);
uint8_t isPageEmpty(uint8_t pagenr);
uint8_t getUnusedPage();

//Basic utility functions
uint8_t isPageInMemory(uint8_t pagenr);
void setIsPageInMemory(uint8_t pagenr, uint8_t isin);
uint8_t isAddressInMemory(uint32_t address);
uint32_t getLastUsed(uint8_t pagenr);
void setLastUsed(uint8_t pagenr, uint32_t time);
uint8_t getMappedAddressInMemory(uint8_t pagenr);
void setMappedAddressInMemory(uint8_t pagenr, uint8_t address);
void writeBytesToBuffer(char *array, uint32_t length, uint32_t address);

//Timer
void *incTimer(void *vargp);

//Basic help functions
char *sliceArray(char *array, uint32_t arraysize, uint32_t pos1, uint32_t pos2);
int getIndexFromArray(char *array, int nr);
void setBit(char *array, int nr);
void clearBit(char *array, int nr);
int getBit(char *array, int nr);