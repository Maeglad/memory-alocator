#include "wrapper.h"
#include <stdio.h>
/* Kod funkcii my_alloc a my_free nahradte vlastnym. Nepouzivajte ziadne
 * globalne ani staticke premenne; jedina globalna pamat je dostupna pomocou
 * mread/mwrite/msize, ktorych popis najdete vo wrapper.h */

/* Ukazkovy kod zvladne naraz iba jedinu alokaciu. V 0-tom bajte pamate si
 * pamata, ci je pamat od 1 dalej volna alebo obsadena. 
 *
 * V pripade, ze je volna, volanie my_allloc skonci uspesne a vrati zaciatok
 * alokovanej RAM; my_free pri volnej mamati zlyha.
 *
 * Ak uz nejaka alokacia prebehla a v 0-tom bajte je nenulova hodnota. Nie je
 * mozne spravit dalsiu alokaciu, takze my_alloc musi zlyhat. my_free naopak
 * zbehnut moze a uvolni pamat.
 */


/**
 * Inicializacia pamate
 *
 * Zavola sa, v stave, ked sa zacina s prazdnou pamatou, ktora je inicializovana
 * na 0.
 */
void my_init(void) {
    mwrite(0, 0);
    int memSize = msize()-3; 
    uint8_t pom = memSize>>8; 
    mwrite(1,pom);
    mwrite(2,(uint8_t)(memSize & 255));  
}

/**
 * Poziadavka na alokaciu 'size' pamate. 
 *
 * Ak sa pamat podari alokovat, navratova hodnota je adresou prveho bajtu
 * alokovaneho priestoru v RAM. Pokial pamat uz nie je mozne alokovat, funkcia
 * vracia FAIL.
 *
 * moja idea: hlavcka bude ulozena takto
 *  1 bajt: 0(free) / 1(occupied)
 *  2 bajty: velkost zapisana binarne a splitnuta na byty, velkost bez hlavicky
 *
 * robim first fit
 *
 * moje predpoklady su ze program testovaca nerobi writy do nepridelenej pamete
 * pamet nepresiahne 2^16 lebo moje hlavicky su ukladane fixne na 3 byty a nechcem riskovat s malom casu
 *
 * alocator by mal teraz fungovat
 */
int my_alloc(unsigned int size) {
    int poloha = 0; 
    // ak je mnozstvo pamete rovne 0 tak si osobne myslim ze allocovat nic nemusim
    if(size <= 0)return FAIL;
    // hlavcku si musim ulozit nech sa deje cokolvek
    if(size + 3 > msize())return FAIL; 
    // ak je poloha + size < msize tak ma zmysel dalej hladat
    // pozriem sa na polohu ak je free pozbieram data o jej velkosti
    // ak nie skocim na dalsiu
    // ak najdem dost velku polohu tak ju skratim na moju velkost a vyrobim novy 
    while( (poloha + size + 3) <= msize() ){
        int obsadene;
        int block = 0;
        obsadene = mread(poloha);
        block = mread(poloha+1)<<8;
        block += mread(poloha+2);
        if(obsadene == 1){
             poloha += 3 + block;
             continue;
        }
        else if( block >= size){
            mwrite(poloha, 1);
            if((block - size) < 3){// block >= size cize nejdem do -
                return poloha + 3;
            }
            else{
                // vytvaram aj hlavicky k blokom dlzky 0 lebo ak sa ten block za tym uvolni tak ziskam 3 bajty
                mwrite(poloha + 1, size>>8);
                mwrite(poloha + 2, size & 255);
                // zapisem novu velkost do povodnej hlavicky   
                // nova hlavicka obsahujuca zvysok bloku
                block = block - size - 3;
                mwrite(poloha + 3 + size    , 0);
                mwrite(poloha + 3 + size + 1, block>>8);
                mwrite(poloha + 3 + size + 2, block & 255);
                return poloha + 3;
            }
            
        }
        else {
            poloha += 3 + block;
        }
    }

    return FAIL;
}

/**
 * Poziadavka na uvolnenie alokovanej pamate na adrese 'addr'.
 *
 * Ak bola pamat zacinajuca na adrese 'addr' alokovana, my_free ju uvolni a
 * vrati OK. Ak je adresa 'addr' chybna (nezacina na nej ziadna alokovana
 * pamat), my_free vracia FAIL.
 */

int my_free(unsigned int addr) {

	if(overAdresu(addr) == 0){
        return FAIL;
    }
    // adresa je validna musim ist od predu a pozerat na dalsiu adresu ak je free free mergenem
    int poloha = 0;
    mwrite(addr - 3, 0);
    while(poloha < msize()){
        int blockSize = 0;
        blockSize = mread(poloha + 1)<<8;
        blockSize += mread(poloha + 2);
        if( (poloha + blockSize+3) == msize()){
            return OK;
        }
        if(mread(poloha) == 0){
            //printf("cistim %d %d\n", poloha, blockSize); 
            while( ((poloha + blockSize + 3) < msize())&&(mread(poloha + blockSize + 3) == 0) ){
                int nextSize = 0;
                nextSize = mread(poloha + blockSize + 3 + 1)<<8;
                nextSize += mread(poloha + blockSize + 3 + 2);
                blockSize += nextSize + 3;
               // printf("%d", blockSize);
            }
            mwrite(poloha + 1, blockSize>>8);
            mwrite(poloha + 2, blockSize & 255);
        }
        poloha += blockSize + 3;
        //printf("%d %d\n", poloha, blockSize); 
    }
	return OK;
}
// toto by malo fungovat spravne
int overAdresu(int addr){
    if(addr > msize())return 0;
    if(addr < 3)return 0;
    if(mread(addr-3) != 1)return 0;
    int poloha = 0; 
    while(poloha < (addr-3)){
        int blockSize = mread(poloha + 1)<<8;
        blockSize += mread(poloha + 2);
        poloha += 3 + blockSize;
    }
    if(poloha != ( addr-3 ) ){return 0;}
    return 1; 
}
