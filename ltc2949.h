/*
 * ltc2949.h
 *
 *  Created on: 13 Jun 2026
 *      Author: Aswin
 */

#ifndef INCLUDE_LTC2949_H_
#define INCLUDE_LTC2949_H_

#include <stdint.h>
#include <stddef.h>

/*** LTC2949 - Registers / bits ***/
#define LTC2949_OPCTRL     0xF0u
#define LTC2949_WKUPACK    0x70u
#define OPCTRL_SLEEP_BIT   0x01u    /* bit 0 */
#define OPCTRL_CONT_BIT    0x08u    /* bit 3 */

/*** LTC2949 - Timings ***/
#define LTC2949_TREADY_US      10u
#define LTC2949_TBOOT_MS       100u
#define LTC2949_TIDLE_CORE_MS  20u
#define LTC2949_WAKE_RETRIES   3u

typedef enum
{
    LTC2949_OK = 0,
    LTC2949_ERR_PEC,
    LTC2949_ERR_WAKE,
    LTC2949_ERR_VERIFY
} ltc2949_status_t;

/*** SEAM ***/
extern int  ltc2949_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len);
extern void ltc2949_spi_wake_pulse(void);
extern void ltc2949_delay_us(uint32_t us);
extern void ltc2949_delay_ms(uint32_t ms);

/*** API ***/
void             ltc2949_pec15_init(void);
uint16_t         ltc2949_pec15(const uint8_t *d, size_t len);
ltc2949_status_t ltc2949_configure_continuous(void);           /* Task 1.3 */
ltc2949_status_t ltc2949_configure_continuous_verified(void);  /* Task 1.4 */

#endif /* INCLUDE_LTC2949_H_ */
