#pragma once

int rdc_init(void);
int rdc_bus_control(void);
int rdc_Dump80(unsigned long address, unsigned char *buffer);
int rdc_Dump2000(unsigned long address, unsigned char *buffer);
int rdc_DumpSector(unsigned long address, unsigned char *buffer);
void rdc_EonSectorErase(unsigned long addr);
int rdc_EonProgram(unsigned long addr, unsigned short value);
unsigned long rdc_eax(void);
unsigned long rdc_Detect(void);
