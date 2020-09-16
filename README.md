# ASG210 M4 real-time samples

Provides real-time application samples for using a private network in M4 core. W5500 is connected with SPI and uses ioLibrary.

### Current Status

- Avaiable sample code
  - Bare Metal: GPIO / I2C / UART / TCP Loopback / DHCP Server / DHCP Client / SNTP Server 

- Supported Azure Sphere SDK/API Version
  - SDK Version: 20.07 (Download latest version [here](https://docs.microsoft.com/en-ca/azure-sphere/install/install-sdk?pivots=visual-studio#install-the-azure-sphere-sdk))


### To Clone this repository:
```
$ git clone https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples
```

### Description
This repository maintains the MT3620 M4 driver and real-time application sample code, which divided into the following directories:

- MT3620_M4_BSP/
  - This folder includes the CMSIS-Core APIs and the configuration of the interrupt vector table.
  - Current BSP supports Bare Metal and FreeRTOS.

- MT3620_M4_Driver/
  - The MT3620 M4 driver provides the APIs to access the peripheral interfaces, ex GPIO / SPI / I2S / I2C / UART...
  - This driver could be divided into two layers
     - Upper layer: M-HAL (MediaTek Hardware AbstractionLayer), which provides the high-level API to the real-time application.
     - Lower layer: HDL (Hardware Driving Layer), which handles the low-level hardware control.
- Sample_Code/
  - This is the executable CMake project sample code that utilizes the OS_HAL APIs to access the peripheral interfaces.
  - Bare Metal code is included.

Please refer to the MT3620 M4 API Reference Manual for the detailed API description.

### Sample Code Guide

All Sample Code Guide is included in each Sample Code project.

* [DHCP Client](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/DHCP_Client)
* [DHCP Server](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/DHCP_Server)
* [TCP Loopback](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/TCP_Loopback)
* [SNTP Server](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/SNTP_Server)
* [GPIO RTApp mt3620 BareMetal](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/GPIO_RTApp_mt3620_BareMetal)
* [I2C RTApp mt3620 BareMetal](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/I2C_RTApp_mt3620_BareMetal)
* [I2C SSD1306 RTApp mt3620 BareMetal](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/I2C_SSD1306_RTApp_mt3620_BareMetal)
* [UART RTApp mt3620 BareMetal](https://github.com/WIZnet-Azure-Sphere/ASG210-M4-Samples/tree/master/Sample_Code/BareMetal/UART_RTApp_mt3620_BareMetal)