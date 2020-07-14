// Converted using ConvPNG
#include <stdint.h>
#include "HDERROR.h"

#include <fileioc.h>
uint8_t *HDERROR[1] = {
 (uint8_t*)0,
};

bool HDERROR_init(void) {
    unsigned int data, i;
    ti_var_t appvar;

    ti_CloseAll();

    appvar = ti_Open("HDERROR", "r");
    data = (unsigned int)ti_GetDataPtr(appvar) - (unsigned int)HDERROR[0];
    for (i = 0; i < HDERROR_num; i++) {
        HDERROR[i] += data;
    }

    ti_CloseAll();

    return (bool)appvar;
}
