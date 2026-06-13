/*
 * ltc2949.c
 *
 *  Created on: 13 Jun 2026
 *      Author: Aswin
 */

#include "ltc2949.h"

#define PEC_SEED  0x0010u
#define PEC_POLY  0x4599u
static uint16_t pecTable[256];

void ltc2949_pec15_init(void)
{
    int i, b;
    for (i = 0; i < 256; i++)
    {
        uint16_t rem = (uint16_t)(i << 7);
        for (b = 8; b > 0; --b)
            rem = (rem & 0x4000u) ? (uint16_t)((rem << 1) ^ PEC_POLY)
                                  : (uint16_t)(rem << 1);
        pecTable[i] = rem;
    }
}

uint16_t ltc2949_pec15(const uint8_t *d, size_t len)
{
    uint16_t rem = PEC_SEED;
    size_t i;
    for (i = 0; i < len; i++)
    {
        uint8_t a = (uint8_t)((rem >> 7) ^ d[i]);
        rem = (uint16_t)((rem << 8) ^ pecTable[a]);
    }
    return (uint16_t)(rem << 1);
}

static ltc2949_status_t write_reg8(uint8_t raddr, uint8_t data)
{
    uint8_t cmd[2] = {0xFEu, raddr};
    uint16_t cpec = ltc2949_pec15(cmd, 2);
    uint16_t dpec = ltc2949_pec15(&data, 1);
    uint8_t tx[8] = {0xFEu, raddr, (uint8_t)(cpec>>8), (uint8_t)cpec,
                     0x40u, data, (uint8_t)(dpec>>8), (uint8_t)dpec};
    uint8_t rx[8];
    return (ltc2949_spi_transfer(tx, rx, 8) == 0) ? LTC2949_OK : LTC2949_ERR_PEC;
}

static ltc2949_status_t read_reg8(uint8_t raddr, uint8_t *out)
{
    uint8_t cmd[2] = {0xFEu, raddr};
    uint16_t cpec = ltc2949_pec15(cmd, 2);
    uint8_t tx[8] = {0xFEu, raddr, (uint8_t)(cpec>>8), (uint8_t)cpec,
                     0x80u, 0xFFu, 0xFFu, 0xFFu};
    uint8_t rx[8], data; uint16_t rxpec;
    if (ltc2949_spi_transfer(tx, rx, 8) != 0)
        return LTC2949_ERR_PEC;
    data  = rx[5];
    rxpec = (uint16_t)(((uint16_t)rx[6] << 8) | rx[7]);
    if (rxpec != ltc2949_pec15(&data, 1))
        return LTC2949_ERR_PEC;
    *out = data;
    return LTC2949_OK;
}

static ltc2949_status_t wake(void)
{
    uint32_t n; uint8_t op;
    for (n = 0; n < LTC2949_WAKE_RETRIES; n++)
    {
        ltc2949_spi_wake_pulse();
        ltc2949_delay_us(LTC2949_TREADY_US);   /* tREADY */
        ltc2949_delay_ms(LTC2949_TBOOT_MS);    /* tBOOT  */
        if (read_reg8(LTC2949_OPCTRL, &op) == LTC2949_OK &&
            (op & OPCTRL_SLEEP_BIT) == 0u)
        {
            return write_reg8(LTC2949_WKUPACK, 0x00u);
        }
    }
    return LTC2949_ERR_WAKE;
}

/*** 1.3 ***/
ltc2949_status_t ltc2949_configure_continuous(void)
{
    ltc2949_status_t s = wake();
    if (s != LTC2949_OK)
        return s;
    s = write_reg8(LTC2949_OPCTRL, OPCTRL_CONT_BIT);        /* CONT = 0x08 */
    if (s != LTC2949_OK)
        return s;
    ltc2949_delay_ms(LTC2949_TIDLE_CORE_MS);
    return LTC2949_OK;
}

/*** 1.4 ***/
ltc2949_status_t ltc2949_configure_continuous_verified(void)
{
    uint8_t op;
    ltc2949_status_t s = ltc2949_configure_continuous();
    if (s != LTC2949_OK)
        return s;
    s = read_reg8(LTC2949_OPCTRL, &op);
    if (s != LTC2949_OK)
        return LTC2949_ERR_PEC;
    if ((op & OPCTRL_CONT_BIT) && !(op & OPCTRL_SLEEP_BIT))
        return LTC2949_OK;
    return LTC2949_ERR_VERIFY;
}


