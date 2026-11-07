#ifndef CONFIG_H
#define CONFIG_H

// Direcciones Base de Periféricos (AT91SAM3X8E - Cortex-M3)[cite: 2, 5]
#define PMC_BASE         0x400E0600
#define PIOB_BASE        0x400E1200
#define PIOC_BASE        0x400E1400
#define TWI0_BASE        0x40018000
#define ADC_BASE         0x40038000

// Direcciones de registros específicos para configuración de E/S y TWI (I2C)
#define PIO_PER_OFFSET   0x0000
#define PIO_OER_OFFSET   0x0010
#define PIO_SODR_OFFSET  0x0030
#define PIO_CODR_OFFSET  0x0034
#define PIO_PDSR_OFFSET  0x003C

#endif