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
    int memSize = msize()-2; 
    uint8_t pom = memSize>>8; 
    mwrite(0,pom);
    mwrite(1,(uint8_t)(memSize & 255));  
}

/**
 * Poziadavka na alokaciu 'size' pamate. 
 *
 * Ak sa pamat podari alokovat, navratova hodnota je adresou prveho bajtu
 * alokovaneho priestoru v RAM. Pokial pamat uz nie je mozne alokovat, funkcia
 * vracia FAIL.
 *
 * moja idea: hlavcka bude ulozena takto
 *  1 bajt[8] : 0(free) / 1(occupied)
 *  1 bajt[0-7], 2 bajt: velkost zapisana binarne a splitnuta na byty, velkost bez hlavicky
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
    if(size + 2 > msize())return FAIL; 
    // ak je poloha + size < msize tak ma zmysel dalej hladat
    // pozriem sa na polohu ak je free pozbieram data o jej velkosti
    // ak nie skocim na dalsiu
    // ak najdem dost velku polohu tak ju skratim na moju velkost a vyrobim novy 
    while( (poloha + size + 2) <= msize() ){
        int obsadene;
        int block = 0;
        obsadene = mread(poloha) & 128;
        block = (mread(poloha)<<8) & 127;
        block += mread(poloha+1);
        if(obsadene){
             poloha += 2 + block;
             continue;
        }
        else if( block >= size){
            if((block - size) < 2){// block >= size cize nejdem do -
                int tmp = mread(poloha);
                mwrite(poloha, tmp + 128);
                return poloha + 2;
            }
            else{
                // vytvaram aj hlavicky k blokom dlzky 0 lebo ak sa ten block za tym uvolni tak ziskam 2 bajty
                mwrite( poloha, 128 + (size>>8) );
                mwrite( poloha + 1, size & 255 );
                // zapisem novu velkost do povodnej hlavicky   
                // nova hlavicka obsahujuca zvysok bloku
                block = block - size - 2;
                mwrite(poloha + 2 + size , ( block>>8) );
                mwrite(poloha + 2 + size + 1, block & 255);
                return poloha + 2;
            }
            
        }
        else {
            poloha += 2 + block;
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
    int upperSize = mread(addr-2);
    mwrite(addr-2, upperSize & 127);
   // mwrite(addr - 2, 0);// toto musim hodit inam
    while(poloha < msize()){
        int blockSize = 0;
        blockSize = (mread(poloha)<<8) & 127;
        blockSize += mread(poloha + 1);
        if( (poloha + blockSize+2) == msize()){
            return OK;
        }

        if( ( mread(poloha)&128 ) == 0){
            //printf("cistim %d %d\n", poloha, blockSize); 
            while( ((poloha + blockSize + 2) < msize())&&( ( mread(poloha + blockSize + 2)&128 ) == 0) ){
                int nextSize = 0;
                nextSize = (mread(poloha + blockSize + 2)<<8)& 127;
                nextSize += mread(poloha + blockSize + 2 + 1);
                blockSize += nextSize + 2;
               // printf("%d", blockSize);
            }
            mwrite(poloha, blockSize>>8);
            mwrite(poloha + 1 , blockSize & 255);
        }
        poloha += blockSize + 2;
        //printf("%d %d\n", poloha, blockSize); 
    }
	return OK;
}
// toto by malo fungovat spravne
int overAdresu(int addr){
    if(addr > msize())return 0;
    if(addr < 2)return 0;
    if((mread(addr-2) & 128) != 128)return 0;
    int poloha = 0; 
    while(poloha < (addr-2)){
        int blockSize =( mread(poloha)<<8) & 127;
        blockSize += mread(poloha + 1);
        poloha += 2 + blockSize;
    }
    if(poloha != ( addr-2 ) ){return 0;}
    return 1; 
}
