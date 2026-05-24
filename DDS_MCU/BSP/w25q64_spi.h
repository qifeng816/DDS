#ifndef W25Q64_SPI_H
#define W25Q64_SPI_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* 
 * 硬件定义 - 请确保 SysConfig 中：
 * SPI 实例名为：SPI_W25Q64 (或修改下方宏)
 * GPIO 端口名为：GPIO_CS
 * 引脚名为：PIN_W25Q64_CS
 */
#define W25Q64_SPI_INST         SPI_1_INST
#define W25Q64_CS_PORT          W25Q64_PORT
#define W25Q64_CS_PIN           W25Q64_CS_W25Q64_PIN

// 片选控制宏
#define W25Q64_CS_LOW()  DL_GPIO_clearPins(W25Q64_PORT, W25Q64_CS_W25Q64_PIN)
#define W25Q64_CS_HIGH() DL_GPIO_setPins(W25Q64_PORT, W25Q64_CS_W25Q64_PIN)

// W25Q64 常用指令集
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS1       0x05
#define W25Q_CMD_WRITE_STATUS1      0x01
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_PAGE_PROG          0x02
#define W25Q_CMD_SECTOR_ERASE       0x20
#define W25Q_CMD_BLOCK_ERASE_32K    0x52
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_READ_ID            0x9F    // JEDEC ID (厂商+设备信息)
#define W25Q_CMD_DUMMY              0xFF    // 读数据时发送的无关字节

// 函数原型
void W25Q64_Init(void);
uint32_t W25Q64_ReadID(void);               // 返回格式: 0xEF4017
uint8_t  W25Q64_ReadStatus(void);           // 读取状态寄存器
void W25Q64_WaitBusy(void);                 // 等待 Flash 内部操作完成
void W25Q64_WriteEnable(void);              // 写使能
void W25Q64_SectorErase(uint32_t addr);     // 擦除一个扇区 (4KB)
void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);
void W25Q64_WriteData(uint32_t addr, uint8_t *buf, uint32_t len); // 注意：写之前必须先擦除

#endif