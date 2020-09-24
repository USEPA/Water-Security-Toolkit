#ifndef _TEVA_H_
#define _TEVA_H_

int writeTEVAIndexData(void *data, FILE *stream);
int readTEVAIndexData(PERD db, PQualitySim data, FILE *stream);
void freeTEVAIndexData(void **data);

#endif 
