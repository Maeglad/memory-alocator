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
    printf("%d",memSize);
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
 */
int my_alloc(unsigned int size) {
    int poloha = 0; 
    // ak je mnozstvo pamete rovne 0 tak si osobne myslim ze allocovat nic nemusim
    if(size == 0)return FAIL;
    // hlavcku si musim ulozit nech sa deje cokolvek
    if(size + 3 > msize())return FAIL;
    // ak je poloha + size < msize tak ma zmysel dalej hladat
    // pozriem sa na polohu ak je free pozbieram data o jej velkosti
    // ak nie skocim na dalsiu
    // ak najdem dost velku polohu tak ju skratim na moju velkost a vyrobim novy 
    while( (poloha + size + 3) <= msize() ){
        uint8_t obsadene;
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
            
            if(size - block < 3){
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

	/* Adresa nie je platnym smernikom, ktory mohol vratit my_alloc */
	if (addr != 1)
		return FAIL;

	/* Nie je alokovana ziadna pamat, nemozeme ju teda uvolnit */
	if (mread(0) != 1)
		return FAIL;

	/* Vsetko je OK, mozeme uvolnit pamat */
	mwrite(0, 0);
	return OK;
}


