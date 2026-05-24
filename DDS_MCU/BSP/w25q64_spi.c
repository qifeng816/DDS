//#include "w25q64_spi.h"

///**
// * @brief SPI 交换一个字节
// * @param data 要发送的字节
// * @return 接收到的字节
// */
//static uint8_t SPI_SwapByte(uint8_t data) {
//    // 1. 等待发送 FIFO 有空间 (在10MHz下通常瞬间完成)
//    while (DL_SPI_isBusy(W25Q64_SPI_INST));
//    
//    // 2. 发送数据
//    DL_SPI_transmitData8(W25Q64_SPI_INST, data);
//    
//    // 3. 等待数据发送完毕且接收 FIFO 收到数据
//    while (DL_SPI_isBusy(W25Q64_SPI_INST));
//    
//    // 4. 读取并返回接收到的数据
//    return DL_SPI_receiveData8(W25Q64_SPI_INST);
//}

//// 初始化
//void W25Q64_Init(void) {
//    W25Q64_CS_HIGH();  // 初始状态片选拉高
//}

//// 读取 24 位 JEDEC ID (厂商ID + 设备ID)
//uint32_t W25Q64_ReadID(void) {
//    uint8_t id1, id2, id3;
//    
//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_READ_ID);   // 发送 0x9F
//    id1 = SPI_SwapByte(W25Q_CMD_DUMMY); // 厂商ID (0xEF)
//    id2 = SPI_SwapByte(W25Q_CMD_DUMMY); // 设备类型
//    id3 = SPI_SwapByte(W25Q_CMD_DUMMY); // 容量 ID
//    W25Q64_CS_HIGH();
//    
//    return ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3;
//}

//// 读取状态寄存器 1
//uint8_t W25Q64_ReadStatus(void) {
//    uint8_t status;
//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_READ_STATUS1);
//    status = SPI_SwapByte(W25Q_CMD_DUMMY);
//    W25Q64_CS_HIGH();
//    return status;
//}

//// 等待芯片内部操作完成 (忙状态检查)
//void W25Q64_WaitBusy(void) {
//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_READ_STATUS1);
//    // 循环检查状态寄存器的最低位 (BUSY位)
//    // 如果为 1 则继续等待，为 0 则退出
//    while ((SPI_SwapByte(W25Q_CMD_DUMMY) & 0x01)); 
//    W25Q64_CS_HIGH();
//}

//// 写使能
//void W25Q64_WriteEnable(void) {
//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_WRITE_ENABLE);
//    W25Q64_CS_HIGH();
//}

//// 扇区擦除 (4KB)
//void W25Q64_SectorErase(uint32_t addr) {
//    W25Q64_WriteEnable();    // 擦除前必须写使能

//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_SECTOR_ERASE);  // 0x20
//    SPI_SwapByte((uint8_t)(addr >> 16));
//    SPI_SwapByte((uint8_t)(addr >> 8));
//    SPI_SwapByte((uint8_t)addr);
//    W25Q64_CS_HIGH();

//    W25Q64_WaitBusy();       // 等待擦除完成 (扇区擦除约需 45ms-400ms)
//}

//// 读取数据
//void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint32_t len) {
//    W25Q64_CS_LOW();
//    SPI_SwapByte(W25Q_CMD_READ_DATA);  // 0x03
//    SPI_SwapByte((uint8_t)(addr >> 16));
//    SPI_SwapByte((uint8_t)(addr >> 8));
//    SPI_SwapByte((uint8_t)addr);

//    for (uint32_t i = 0; i < len; i++) {
//        buf[i] = SPI_SwapByte(W25Q_CMD_DUMMY);
//    }
//    W25Q64_CS_HIGH();
//}

///**
// * @brief 写数据 (自动处理页写入逻辑)
// * @note W25Q64 写数据前必须先擦除，且单次 Page Program 不能跨过 256 字节边界
// */
//void W25Q64_WriteData(uint32_t addr, uint8_t *buf, uint32_t len) {
//    uint32_t remain = len;
//    uint32_t current_addr = addr;
//    uint8_t *p_buf = buf;

//    while (remain > 0) {
//        // 计算当前页还剩多少空间可以写
//        uint32_t page_offset = current_addr % 256;
//        uint32_t can_write = 256 - page_offset;
//        uint32_t write_len = (remain < can_write) ? remain : can_write;

//        W25Q64_WriteEnable();

//        W25Q64_CS_LOW();
//        SPI_SwapByte(W25Q_CMD_PAGE_PROG); // 0x02
//        SPI_SwapByte((uint8_t)(current_addr >> 16));
//        SPI_SwapByte((uint8_t)(current_addr >> 8));
//        SPI_SwapByte((uint8_t)current_addr);

//        for (uint32_t i = 0; i < write_len; i++) {
//            SPI_SwapByte(*p_buf++);
//        }
//        W25Q64_CS_HIGH();
//				
//        W25Q64_WaitBusy(); // 等待页写入完成 (约需 0.4ms-3ms)

//        current_addr += write_len;
//        remain -= write_len;
//    }
//}
#include "w25q64_spi.h"

/**
 * @brief SPI 交换一个字节 (底层硬件操作)
 * 改进：使用 FIFO 状态标志确保通信可靠性
 */
static uint8_t SPI_SwapByte(uint8_t data) {
    // 1. 等待发送 FIFO 有空间 (如果 FIFO 满了则等待)
    while (DL_SPI_isTXFIFOFull(W25Q64_SPI_INST));
    
    // 2. 发送数据
    DL_SPI_transmitData8(W25Q64_SPI_INST, data);
    
    // 3. 等待数据交换完成：只要接收 FIFO 为空，说明还没收到回传字节
    // 增加一个超时的万能保险，防止硬件断线卡死
    uint32_t timeout = 1000000; 
    while (DL_SPI_isRXFIFOEmpty(W25Q64_SPI_INST) && timeout > 0) {
        timeout--;
    }
    
    // 4. 返回接收到的数据
    return DL_SPI_receiveData8(W25Q64_SPI_INST);
}

// 初始化
void W25Q64_Init(void) {
    W25Q64_CS_HIGH();  // 初始状态片选拉高
}

/**
 * @brief 等待芯片内部操作完成 (忙状态检查)
 * 改进：增加 timeout 机制，防止 MISO 引脚断路导致程序永久死循环
 */
void W25Q64_WaitBusy(void) {
    uint32_t timeout = 800000; 
    
    while (timeout--) {
        W25Q64_CS_LOW();
        SPI_SwapByte(W25Q_CMD_READ_STATUS1);      // 发送 05h
        uint8_t status = SPI_SwapByte(W25Q_CMD_DUMMY); // 读取真正的状态
        W25Q64_CS_HIGH(); // 每次读完拉高 CS，强制 Flash 刷新状态寄存器输出
        
        if (!(status & 0x01)) {
            return; // 真正的不忙了，退出
        }
    }
    // 如果走到这里，说明超时了
}

// 读取 24 位 JEDEC ID
uint32_t W25Q64_ReadID(void) {
    uint8_t id1, id2, id3;
    
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q_CMD_READ_ID);     // 0x9F
    id1 = SPI_SwapByte(W25Q_CMD_DUMMY); 
    id2 = SPI_SwapByte(W25Q_CMD_DUMMY); 
    id3 = SPI_SwapByte(W25Q_CMD_DUMMY); 
    W25Q64_CS_HIGH();
    
    return ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3;
}

// 写使能
void W25Q64_WriteEnable(void) {
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q_CMD_WRITE_ENABLE); // 0x06
    W25Q64_CS_HIGH();
}

// 扇区擦除 (4KB)
void W25Q64_SectorErase(uint32_t addr) {
    W25Q64_WriteEnable();    

    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q_CMD_SECTOR_ERASE); // 0x20
    SPI_SwapByte((uint8_t)(addr >> 16));
    SPI_SwapByte((uint8_t)(addr >> 8));
    SPI_SwapByte((uint8_t)addr);
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();       // 等待擦除完成
}

// 读取数据
void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint32_t len) {
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q_CMD_READ_DATA);    // 0x03
    SPI_SwapByte((uint8_t)(addr >> 16));
    SPI_SwapByte((uint8_t)(addr >> 8));
    SPI_SwapByte((uint8_t)addr);

    for (uint32_t i = 0; i < len; i++) {
        buf[i] = SPI_SwapByte(W25Q_CMD_DUMMY);
    }
    W25Q64_CS_HIGH();
}

/**
 * @brief 写数据 (自动处理页写入逻辑)
 */
void W25Q64_WriteData(uint32_t addr, uint8_t *buf, uint32_t len) {
    uint32_t remain = len;
    uint32_t current_addr = addr;
    uint8_t *p_buf = buf;

    while (remain > 0) {
        uint32_t page_offset = current_addr % 256;
        uint32_t can_write = 256 - page_offset;
        uint32_t write_len = (remain < can_write) ? remain : can_write;

        W25Q64_WriteEnable();

        W25Q64_CS_LOW();
        SPI_SwapByte(W25Q_CMD_PAGE_PROG); // 0x02
        SPI_SwapByte((uint8_t)(current_addr >> 16));
        SPI_SwapByte((uint8_t)(current_addr >> 8));
        SPI_SwapByte((uint8_t)current_addr);

        for (uint32_t i = 0; i < write_len; i++) {
            SPI_SwapByte(*p_buf++);
        }
        W25Q64_CS_HIGH();
				
        W25Q64_WaitBusy(); // 等待写入完成

        current_addr += write_len;
        remain -= write_len;
    }
}