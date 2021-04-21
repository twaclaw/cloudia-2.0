#if defined(CFG_b_l072Z_lrwan1_board)
#define BRD_I2C 1
#define GPIO_I2C_SCL	BRD_GPIO_AF(PORT_B, 8, 4)
#define GPIO_I2C_SDA	BRD_GPIO_AF(PORT_B, 9, 4)


#define GPIO_RST        BRD_GPIO(PORT_C, 0)
#define GPIO_DIO0       BRD_GPIO_AF_EX(PORT_B, 4, 4, BRD_GPIO_CHAN(1))
#define GPIO_DIO1       BRD_GPIO(PORT_B, 1)
#define GPIO_DIO2       BRD_GPIO(PORT_B, 0)
#define GPIO_DIO3       BRD_GPIO(PORT_C, 13)
#define GPIO_DIO4       BRD_GPIO(PORT_A, 5)
#define GPIO_DIO5       BRD_GPIO(PORT_A, 4)

#define GPIO_TCXO_PWR   BRD_GPIO(PORT_A, 12)
#define GPIO_RX         BRD_GPIO(PORT_A, 1) // PA_RFI
#define GPIO_TX         BRD_GPIO(PORT_C, 1) // PA_BOOST
#define GPIO_TX2        BRD_GPIO(PORT_C, 2) // PA_RFO

#define GPIO_LED1       BRD_GPIO(PORT_B, 5) // grn
#define GPIO_LED2       BRD_GPIO(PORT_A, 5) // red -- used by bootloader
#define GPIO_LED3       BRD_GPIO(PORT_B, 6) // blu
#define GPIO_LED4       BRD_GPIO(PORT_B, 7) // red

#define GPIO_BUTTON     BRD_GPIO_EX(PORT_B, 2, BRD_GPIO_ACTIVE_LOW)

// button PB2

#define BRD_sx1276_radio
#define BRD_PABOOSTSEL(f,p) ((p) > 15)
#define BRD_TXANTSWSEL(f,p) ((BRD_PABOOSTSEL(f,p)) ? HAL_ANTSW_TX : HAL_ANTSW_TX2)

#define BRD_RADIO_SPI   1
#define GPIO_NSS        BRD_GPIO(PORT_A, 15)
#define GPIO_SCK        BRD_GPIO_AF(PORT_B, 3, 0)
#define GPIO_MISO       BRD_GPIO_AF(PORT_A, 6, 0)
#define GPIO_MOSI       BRD_GPIO_AF(PORT_A, 7, 0)

// Enabled USART peripherals
#define BRD_USART       (BRD_USART1 | BRD_USART2)

// USART1
#define BRD_USART1_DMA  BRD_DMA_CHANS(2,3)
#define GPIO_USART1_TX  BRD_GPIO_AF(PORT_A,  9, 4)
#define GPIO_USART1_RX  BRD_GPIO_AF(PORT_A, 10, 4)

// USART2
#define BRD_USART2_DMA  BRD_DMA_CHANS(4,5)
#define GPIO_USART2_TX  BRD_GPIO_AF(PORT_A, 2, 4)
#define GPIO_USART2_RX  BRD_GPIO_AF(PORT_A, 3, 4)

// Debug LED / USART
#define GPIO_DBG_LED    GPIO_LED4
#define BRD_DBG_UART    BRD_USART2_PORT

// Personalization UART
#define BRD_PERSO_UART  BRD_USART2_PORT
#define GPIO_PERSO_DET  GPIO_BUTTON

// power consumption

#ifndef BRD_PWR_RUN_UA
#define BRD_PWR_RUN_UA 6000
#endif

#ifndef BRD_PWR_S0_UA
#define BRD_PWR_S0_UA  2000
#endif

#ifndef BRD_PWR_S1_UA
#define BRD_PWR_S1_UA  12
#endif

#ifndef BRD_PWR_S2_UA
#define BRD_PWR_S2_UA  5
#endif

// brown-out
#define BRD_borlevel   9 // RM0376, pg 116: BOR level 2, around 2.0 V
#endif