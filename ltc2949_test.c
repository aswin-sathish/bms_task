/*
 *  ltc2949_test.c
 *
 *  Created on: 13 Jun 2026
 *      Author: Aswin
 */
#include "ltc2949.h"
#include <stdio.h>

static uint8_t mock_op_seq[8];     /* SLEEP/CONT bits returned per OPCTRL read */
static int     mock_idx;
static int     mock_corrupt_read;  /* 0-based read index to corrupt, -1 = none */

int ltc2949_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len) 
{
    (void)len;
    if (tx[4] == 0x80u && tx[1] == LTC2949_OPCTRL) 	 /* an OPCTRL read */
	{     
        int idx = mock_idx;
        uint8_t d = mock_op_seq[mock_idx++];
        int bad   = (mock_corrupt_read == idx);
        uint16_t p = bad ? 0xBAD0u : ltc2949_pec15(&d, 1);
        rx[5] = d;
        rx[6] = (uint8_t)(p >> 8);
        rx[7] = (uint8_t)p;
    }
    return 0;                         
}

void ltc2949_spi_wake_pulse(void)  { }
void ltc2949_delay_us(uint32_t u)  { (void)u; }
void ltc2949_delay_ms(uint32_t m)  { (void)m; }

/*** Test Verification ***/
static int g_run = 0, g_fail = 0;
#define CHECK(cond)                                    		\
    do                                                 		\
    {                                                  		\
        g_run++;                                       		\
															\
        if (!(cond))                                   		\
        {                                              		\
            printf("FAIL %s:%d %s\n", __FILE__,  __LINE__, 	\
                   #cond);                             		\
															\
            g_fail++;                                  		\
        }                                              		\
    } while (0)

static void reset(void) 
{
    int i; 
	mock_idx = 0; 
	mock_corrupt_read = -1;
    for (i = 0; i < 8; i++) 
		mock_op_seq[i] = 0x00u;
}

static void test_pec_anchors(void) 
{
    uint8_t fe_f0[2] = {0xFEu, 0xF0u}, d0 = 0x00u, d1 = 0x01u;
    CHECK(ltc2949_pec15(fe_f0, 2) == 0xD1FCu);
    CHECK(ltc2949_pec15(&d0, 1)   == 0x2000u);
    CHECK(ltc2949_pec15(&d1, 1)   == 0xAB32u);
}
static void test_verify_ok(void) 
{   
	/* Reads back 0x08 */           
    reset(); 
	mock_op_seq[1] = 0x08u;
    CHECK(ltc2949_configure_continuous_verified() == LTC2949_OK);
}

static void test_verify_revert(void) 
{  
	/* Reads back 0x00 (SLEEP) */       
    reset(); 
	mock_op_seq[1] = 0x00u;
    CHECK(ltc2949_configure_continuous_verified() == LTC2949_ERR_VERIFY);
}

static void test_verify_corrupt(void) 
{         
    reset(); 
	mock_op_seq[1] = 0x08u; 
	mock_corrupt_read = 1;
    CHECK(ltc2949_configure_continuous_verified() == LTC2949_ERR_PEC);
}
static void test_wake_timeout(void) 
{ 
	/* Never leaves SLEEP */          
    reset(); 
	mock_op_seq[0]=0x01u; 
	mock_op_seq[1]=0x01u; 
	mock_op_seq[2]=0x01u;
    CHECK(ltc2949_configure_continuous_verified() == LTC2949_ERR_WAKE);
}

int main(void) 
{
    ltc2949_pec15_init();
    test_pec_anchors();
    test_verify_ok();
    test_verify_revert();
    test_verify_corrupt();
    test_wake_timeout();
    printf("\n%d checks run, %d failed -> %s\n", g_run, g_fail, g_fail ? "FAIL" : "PASS");
    return g_fail ? 1 : 0;
}
