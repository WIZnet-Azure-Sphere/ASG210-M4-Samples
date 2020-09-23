/*
 * (C) 2005-2020 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "mt3620.h"
#include "os_hal_uart.h"
#include "os_hal_gpt.h"
#include "os_hal_gpio.h"
#include "os_hal_spim.h"

#include "ioLibrary_Driver/Ethernet/socket.h"
#include "ioLibrary_Driver/Ethernet/wizchip_conf.h"
#include "ioLibrary_Driver/Ethernet/W5500/w5500.h"
#include "ioLibrary_Driver/Application/loopback/loopback.h"
#include "ioLibrary_Driver/Internet/DHCP/dhcp.h"

/******************************************************************************/
/* Configurations */
/******************************************************************************/
/* UART */
static const uint8_t uart_port_num = OS_HAL_UART_PORT0;

uint8_t spi_master_port_num = OS_HAL_SPIM_ISU1;
uint32_t spi_master_speed = 2 * 10 * 1000; /* KHz */

#define SPIM_CLOCK_POLARITY SPI_CPOL_0
#define SPIM_CLOCK_PHASE SPI_CPHA_0
#define SPIM_RX_MLSB SPI_MSB
#define SPIM_TX_MSLB SPI_MSB
#define SPIM_FULL_DUPLEX_MIN_LEN 1
#define SPIM_FULL_DUPLEX_MAX_LEN 16

/****************************************************************************/
/* Global Variables */
/****************************************************************************/
struct mtk_spi_config spi_default_config = {
    .cpol = SPIM_CLOCK_POLARITY,
    .cpha = SPIM_CLOCK_PHASE,
    .rx_mlsb = SPIM_RX_MLSB,
    .tx_mlsb = SPIM_TX_MSLB,
#if 1
    // 20200527 taylor
    // W5500 NCS
    .slave_sel = SPI_SELECT_DEVICE_1,
#else
    // Original
    .slave_sel = SPI_SELECT_DEVICE_0,
#endif
};
#if 0
uint8_t spim_tx_buf[SPIM_FULL_DUPLEX_MAX_LEN];
uint8_t spim_rx_buf[SPIM_FULL_DUPLEX_MAX_LEN];
#endif
static volatile int g_async_done_flag;

// Default Static Network Configuration for TCP Server //
#if 0
 wiz_NetInfo gWIZNETINFO = { {0x00, 0x08, 0xdc, 0xff, 0xfa, 0xfb},
                            {192, 168, 50, 10},
                            {255, 255, 255, 0},
                            {192, 168, 50, 1},
                            {8, 8, 8, 8},
                            NETINFO_STATIC };
#else
wiz_NetInfo gWIZNETINFO = {};
#endif

#define USE_READ_SYSRAM
#ifdef USE_READ_SYSRAM
uint8_t __attribute__((unused, section(".sysram"))) s1_Buf[2 * 1024];
uint8_t __attribute__((unused, section(".sysram"))) s2_Buf[2 * 1024];
uint8_t __attribute__((unused, section(".sysram"))) gDATABUF[DATA_BUF_SIZE];
#else
uint8_t s1_Buf[2048];
uint8_t s2_Buf[2048];
uint8_t gDATABUF[DATA_BUF_SIZE];
#endif

/* GPIO */
static const uint8_t gpio_w5500_reset = OS_HAL_GPIO_12;
static const uint8_t gpio_w5500_ready = OS_HAL_GPIO_15;

#define _MAIN_DEBUG_ 1
#define MY_MAX_DHCP_RETRY 2
uint8_t my_dhcp_retry = 0;
#define PRINTLINE() printf("%s(%d)\r\n", __FILE__, __LINE__)

/******************************************************************************/
/* Applicaiton Hooks */
/******************************************************************************/
/* Hook for "printf". */
void _putchar(char character)
{
  mtk_os_hal_uart_put_char(uart_port_num, character);
  if (character == '\n')
    mtk_os_hal_uart_put_char(uart_port_num, '\r');
}

/******************************************************************************/
/* Functions */
/******************************************************************************/
static int gpio_output(u8 gpio_no, u8 level)
{
    int ret;

    ret = mtk_os_hal_gpio_request(gpio_no);
    if (ret != 0) {
        printf("request gpio[%d] fail\n", gpio_no);
        return ret;
    }

    mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_OUTPUT);
    mtk_os_hal_gpio_set_output(gpio_no, level);
    ret = mtk_os_hal_gpio_free(gpio_no);
    if (ret != 0) {
        printf("free gpio[%d] fail\n", gpio_no);
        return 0;
    }
    return 0;
}

static int gpio_input(u8 gpio_no, os_hal_gpio_data* pvalue)
{
    u8 ret;

    ret = mtk_os_hal_gpio_request(gpio_no);
    if (ret != 0) {
        printf("request gpio[%d] fail\n", gpio_no);
        return ret;
    }
    mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_INPUT);
    mtk_os_hal_gpio_get_input(gpio_no, pvalue);
    ret = mtk_os_hal_gpio_free(gpio_no);
    if (ret != 0) {
        printf("free gpio[%d] fail\n", gpio_no);
        return ret;
    }
    return 0;
}

void w5500_init() {
    // W5500 reset
    gpio_output(gpio_w5500_reset, OS_HAL_GPIO_DATA_HIGH);

    // W5500 ready check
    os_hal_gpio_data w5500_ready;
    gpio_input(gpio_w5500_ready, &w5500_ready);

    while (1) {
        if (w5500_ready) break;
    }

    osai_delay_ms(100);
}

// check w5500 network setting
void InitPrivateNetInfo(void)
{
  uint8_t tmpstr[6];
  uint8_t i = 0;
  ctlwizchip(CW_GET_ID, (void *)tmpstr);

#if 0
  if (ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO) < 0) {
    printf("ERROR: ctlnetwork SET\r\n");
  }
#endif

  memset((void *)&gWIZNETINFO, 0, sizeof(gWIZNETINFO));

  ctlnetwork(CN_GET_NETINFO, (void *)&gWIZNETINFO);

  printf("\r\n=== %s NET CONF ===\r\n", (char *)tmpstr);
  printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2],
         gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);

  printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
  printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
  printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
  printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
  printf("======================\r\n");

  // socket 0-7 closed
  // lawrence
  for (i = 0; i < 8; i++)
  {
    setSn_CR(i, 0x10);
  }
  printf("Socket 0-7 Closed \r\n");
}

/*******************************************************
 * @ brief Call back for ip assing & ip update from DHCP
 *******************************************************/
void my_ip_assign(void)
{
  getIPfromDHCP(gWIZNETINFO.ip);
  getGWfromDHCP(gWIZNETINFO.gw);
  getSNfromDHCP(gWIZNETINFO.sn);
  getDNSfromDHCP(gWIZNETINFO.dns);
  gWIZNETINFO.dhcp = NETINFO_DHCP;
  /* Network initialization */
  ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
#ifdef _MAIN_DEBUG_
  InitPrivateNetInfo();
  printf("DHCP LEASED TIME : %ld Sec.\r\n", getDHCPLeasetime());
  printf("\r\n");
#endif
}

/************************************
 * @ brief Call back for ip Conflict
 ************************************/
void my_ip_conflict(void)
{
#ifdef _MAIN_DEBUG_
  printf("CONFLICT IP from DHCP\r\n");
#endif
  //halt or reset or any...
  while (1)
    ; // this example is halt.
}

_Noreturn void RTCoreMain(void)
{
  u32 i = 0;
  bool run_user_applications = false;
  int32_t ret;

  /* Init Vector Table */
  NVIC_SetupVectorTable();

  /* Init UART */
  mtk_os_hal_uart_ctlr_init(uart_port_num);
  //printf("\nUART Inited (port_num=%d)\n", uart_port_num);

  /* Init SPIM */
  mtk_os_hal_spim_ctlr_init(spi_master_port_num);

  printf("------------------------------------------\r\n");
  printf(" ASG210_DHCP_Client_RTApp_MT3620_BareMetal \r\n");
  printf(" App built on: " __DATE__ " " __TIME__ "\r\n");

  w5500_init();

  /* DHCP client Initialization */
  if (gWIZNETINFO.dhcp == NETINFO_DHCP)
  {
    /* Init DHCP */
    DHCP_init(0, gDATABUF);
    // if you want different action instead default ip assign, update, conflict.
    // if cbfunc == 0, act as default.
    reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);

    run_user_applications = false; // flag for running user's code
  }
  else
  {
    // Static
#ifdef _MAIN_DEBUG_
    InitPrivateNetInfo();
#endif
    run_user_applications = true; // flag for running user's code
  }

  while (1)
  {

    /* DHCP */
    /* DHCP IP allocation and check the DHCP lease time (for IP renewal) */
    if (gWIZNETINFO.dhcp == NETINFO_DHCP)
    {
      switch (DHCP_run())
      {
      case DHCP_IP_ASSIGN:
      case DHCP_IP_CHANGED:
        /* If this block empty, act with default_ip_assign & default_ip_update */
        //
        // This example calls my_ip_assign in the two case.
        //
        // Add to ...
        //
        break;
      case DHCP_IP_LEASED:
        //
        // TODO: insert user's code here
        run_user_applications = true;
        //
        break;
      case DHCP_FAILED:
        /* ===== Example pseudo code =====  */
        // The below code can be replaced your code or omitted.
        // if omitted, retry to process DHCP
        my_dhcp_retry++;
        if (my_dhcp_retry > MY_MAX_DHCP_RETRY)
        {
          gWIZNETINFO.dhcp = NETINFO_STATIC;
          DHCP_stop(); // if restart, recall DHCP_init()
#ifdef _MAIN_DEBUG_
          printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
          ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
          InitPrivateNetInfo(); // print out static netinfo to serial
#endif
          my_dhcp_retry = 0;
        }
        break;
      default:
        break;
      }
    }

    // TODO: insert user's code here
    if (run_user_applications)
    {
      // Loopback test : TCP Server
      if ((ret = loopback_tcps(1, s1_Buf, 50000)) < 0) // TCP server loopback test
      {
#ifdef _MAIN_DEBUG_
        printf("SOCKET ERROR : %ld\r\n", ret);
#endif
      }

      if ((ret = loopback_tcps(2, s2_Buf, 50001)) < 0) // TCP server loopback test
      {
#ifdef _MAIN_DEBUG_
        printf("SOCKET ERROR : %ld\r\n", ret);
#endif
      };
    } // End of user's code
  }
}
