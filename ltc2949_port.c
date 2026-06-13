/*
 * ltc2949_port.c
 *
 *  Created on: 13 Jun 2026
 *      Author: Aswin
 */

#include "ltc2949.h"
#include "mibspi.h"
#include "gio.h"
#include "rti.h"

#define LTC_TG            0u
#define RTI_TICKS_PER_US  10u

int ltc2949_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    uint16 t16[8], r16[8];
    size_t i;
    if (len > 8u)
        return -1;
    for (i = 0; i < 8u; i++)
        t16[i] = (i < len) ? (uint16)tx[i] : 0xFFu;

    mibspiSetData(mibspiREG1, LTC_TG, t16);
    mibspiTransfer(mibspiREG1, LTC_TG);
    while (mibspiIsTransferComplete(mibspiREG1, LTC_TG) == 0u) { }
    mibspiGetData(mibspiREG1, LTC_TG, r16);

    for (i = 0; i < len; i++)
        rx[i] = (uint8_t)(r16[i] & 0xFFu);
    return 0;
}

void ltc2949_spi_wake_pulse(void)
{
    uint8_t d[8], r[8];
    int i;
    for (i = 0; i < 8; i++)
        d[i] = 0xFFu;
    (void)ltc2949_spi_transfer(d, r, 8);
}

/*** RTI free-running-counter delays. ***/
void ltc2949_delay_us(uint32_t us)
{
    uint32 start = rtiREG1->CNT[0u].FRCx;
    uint32 ticks = us * RTI_TICKS_PER_US;
    while ((rtiREG1->CNT[0u].FRCx - start) < ticks) { }
}

void ltc2949_delay_ms(uint32_t ms)
{
    while (ms--)
        ltc2949_delay_us(1000u);
}
