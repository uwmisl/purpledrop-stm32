#include "stm32f4xx.h"

int _write(int handle, char *data, int size ) 
{
    int count ;

    handle = handle ; // unused

    for( count = 0; count < size; count++) 
    {
        ITM_SendChar( data[count] ) ;  // Your low-level output function here.
    }

    return count;
}