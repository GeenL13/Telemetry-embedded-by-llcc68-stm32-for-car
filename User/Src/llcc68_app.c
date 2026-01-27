/*
 * Wireless.c
 *
 *  Created on: Jan 24, 2026
 *      Author: Geen
 */

#include <string.h>
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "gpio.h"

#include "llcc68.h"
#include "llcc68_app.h"




// -------- 配置项 -------- //
#define MOST_BUFFER		1	// 最大化芯片缓存区  1：开启；0：关闭	（开启后可以让芯片缓存区被发送或者接收全部占用，芯片功能变成单一发送或者接收模式）

#define TX_BASE_ADDRESS 0x00	// 发送地址起始位置，在开启MOST_BUFFER后不生效
#define RX_BASE_ADDRESS 0x80	// 接收地址起始位置，在开启MOST_BUFFER后不生效


llcc68_chip_status_t chip_status;
llcc68_pkt_params_lora_t pkt_params   // LoRa包参数
    = {
        .preamble_len_in_symb = 8,
        .header_type          = LLCC68_LORA_PKT_EXPLICIT,
        .pld_len_in_bytes     = 16,
        .crc_is_on            = true,
        .invert_iq_is_on      = false,
    };

uint32_t tx_base_address;
uint32_t rx_base_address;

// -------- 外部接口 -------- //

// 无线发送接收初始化
// 1:发送   0:接收
llcc68_status_t LLCC68_Init(uint8_t model)
{
	  // 复位
    llcc68_reset(NULL);


    // 初始化代码放这里
    llcc68_status_t Res;
    // 1. 如果不在STDBY_RC模式下，则使用SetStandby(...)命令进入该模式
    LLCC68_Wait_Busy();
    Res = llcc68_get_status(NULL, &chip_status); // 读取状态以确保芯片处于活动状态
    if (Res != LLCC68_STATUS_OK) return Res;
    if (chip_status.chip_mode != LLCC68_CHIP_MODE_STBY_RC)
    {
        Res = llcc68_set_standby(NULL, 0x00);
        if (Res != LLCC68_STATUS_OK) return Res;
    }



    Res = llcc68_clear_irq_status(NULL, LLCC68_IRQ_TX_DONE);
    if (Res != LLCC68_STATUS_OK) return Res;



    // 2. 使用SetPacketType(...)命令定义协议（LoRa®或FSK）
    LLCC68_Wait_Busy();
    Res = llcc68_set_pkt_type(NULL, LLCC68_PKT_TYPE_LORA);  // 设置为LoRa协议
    if (Res != LLCC68_STATUS_OK) return Res;

    // 3. 使用SetRfFrequency(...)命令定义射频频率
    LLCC68_Wait_Busy();
    Res = llcc68_set_rf_freq(NULL, 433000000); // 设置为433 MHz
    if (Res != LLCC68_STATUS_OK) return Res;

    // 4. 使用SetPaConfig(...)命令定义功率放大器配置
    LLCC68_Wait_Busy();
    llcc68_pa_cfg_params_t pa_cfg = {
        .pa_duty_cycle = 0x04,
        .hp_max = 0x07,
        .device_sel = 0, // 使用高功率PA
        .pa_lut = 0,
    };
    Res = llcc68_set_pa_cfg(NULL, &pa_cfg);
    if (Res != LLCC68_STATUS_OK) return Res;

    // 5.使用SetTxParams(...)命令定义输出功率和 ramp 时间
    LLCC68_Wait_Busy();
    Res = llcc68_set_tx_params(NULL, 22, LLCC68_RAMP_40_US); // 设置为22 dBm输出功率，40微秒ramp时间
    if (Res != LLCC68_STATUS_OK) return Res;

    // 6. 使用SetBufferBaseAddress(...)命令定义数据载荷的存储位置
    if (MOST_BUFFER == 1)
    {
    	tx_base_address = 0x00;
    	rx_base_address = 0x00;
    }
    else
    {
    	tx_base_address = TX_BASE_ADDRESS;
    	rx_base_address = RX_BASE_ADDRESS;
    }
    LLCC68_Wait_Busy();
    Res = llcc68_set_buffer_base_address(NULL, tx_base_address, rx_base_address); // 设置TX和RX缓冲区基地址为0x00和0x40
    if (Res != LLCC68_STATUS_OK) return Res;

    // 8. 使用SetModulationParams(...)命令根据所选协议定义调制参数。
    LLCC68_Wait_Busy();
    llcc68_mod_params_lora_t mod_params = {
        .sf = LLCC68_LORA_SF7,
        .bw = LLCC68_LORA_BW_125,
        .cr = LLCC68_LORA_CR_4_5,
        .ldro = 0,		// 根据上面数据选择
    };
    Res = llcc68_set_lora_mod_params(NULL, &mod_params);
    if (Res != LLCC68_STATUS_OK) return Res;

    // 9. 使用命令SetPacketParams(...)2定义要使用的帧格式
    LLCC68_Wait_Busy();
    // 设置包参数
    pkt_params = (llcc68_pkt_params_lora_t){
      .preamble_len_in_symb = 16,
      .header_type = LLCC68_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 32,
      .crc_is_on = true,
      .invert_iq_is_on = false,
    };
    Res = llcc68_set_lora_pkt_params(NULL, &pkt_params);
    if (Res != LLCC68_STATUS_OK) return Res;

    // 10. 配置DIO和IRQ：使用命令SetDioIrqParams(...)选择TxDone中断请求（IRQ），并将该中断请求映射到一个DIO（DIO1、DIO2或DIO3）。
    LLCC68_Wait_Busy();
    Res = llcc68_set_dio_irq_params(NULL, LLCC68_IRQ_TX_DONE | LLCC68_IRQ_RX_DONE | LLCC68_IRQ_TIMEOUT,
                                    LLCC68_IRQ_TX_DONE | LLCC68_IRQ_TIMEOUT | LLCC68_IRQ_RX_DONE,
									LLCC68_IRQ_NONE,
									LLCC68_IRQ_NONE);
    if (Res != LLCC68_STATUS_OK) return Res;

    // 11. 定义同步字值：使用WriteReg(...)命令通过直接寄存器访问来写入寄存器的值
    LLCC68_Wait_Busy();
    uint8_t buffer_1[1] = {0x14};
    uint8_t buffer_2[1] = {0x24};
    Res = llcc68_write_register(NULL, 0x0740, buffer_1, 1); // 公共同步字高位字节
    if (Res != LLCC68_STATUS_OK) return Res;
    Res = llcc68_write_register(NULL, 0x0741, buffer_2, 1); // 公共同步字低位字节
    if (Res != LLCC68_STATUS_OK) return Res;

    // 12. 将电路设置为发射(接收)模式，使用SetTx()命令开始传输。使用该参数启用超时功能。
    LLCC68_Wait_Busy();
    if (model)	// 1:发送   0:接收
    {
      // 发射模式
      Res = llcc68_set_tx(NULL, 3000); // 设置超时时间为3000毫秒
      if (Res != LLCC68_STATUS_OK) return Res;
    }
    else
    {
      // 接收模式
      Res = LLCC68_Receive();
      if (Res != LLCC68_STATUS_OK) return Res;
    }

    // 测试：获取芯片状态
    //Res = llcc68_get_status(NULL, &chip_status);


    // 测试：发射后接收（自发自收）
    uint8_t send_buf[] = "hello llcc68!";
    uint8_t length = sizeof(send_buf) -1;
    //Res = LLCC68_Send(send_buf, length);
    //if (Res != LLCC68_STATUS_OK) return Res;
    //Res = LLCC68_Receive();
    //if (Res != LLCC68_STATUS_OK) return Res;

    // 测试：sleep
    //HAL_Delay(1000);
    //Res = llcc68_set_sleep(NULL, LLCC68_SLEEP_CFG_WARM_START);
    //if (Res != LLCC68_STATUS_OK) return Res;
    return LLCC68_STATUS_OK;
}



// 填充并发送数据
// 发送数据地址，发送数据大小
llcc68_status_t LLCC68_Send(uint8_t* send_buf, uint8_t send_buf_size)
{
	llcc68_status_t Res;
	// 更新包长度配置
	pkt_params.pld_len_in_bytes = send_buf_size;
	LLCC68_Wait_Busy();
	Res = llcc68_set_lora_pkt_params(NULL, &pkt_params);
	// 填充数据
	LLCC68_Wait_Busy();
	Res = llcc68_write_buffer(NULL, 0x00, send_buf, send_buf_size);
	if (Res != LLCC68_STATUS_OK) return Res;
	// 设置为发射模式（发射）
	LLCC68_Wait_Busy();
	Res = llcc68_set_tx(NULL, 3000); // 设置超时时间为3000毫秒
	if (Res != LLCC68_STATUS_OK) return Res;
	return LLCC68_STATUS_OK;
}

// 进入接收模式
llcc68_status_t LLCC68_Receive(void)
{
	llcc68_status_t Res;
    // 设置为接收模式
    LLCC68_Wait_Busy();
    Res = llcc68_set_rx(NULL, 5000); // 设置超时时间为5000毫秒
    if (Res != LLCC68_STATUS_OK) return Res;
    return LLCC68_STATUS_OK;
}


void LLCC68_Wait_Busy(void)
{
	Driver_Wait_Busy();
}

void LLCC68_TransmitCallback(void)
{
	Driver_TransmitCallback();
}

void LLCC68_ReceiveCallback(uint8_t* receive_buf, uint8_t buf_size)
{
	Driver_ReceiveCallback(receive_buf, buf_size);
}


// ------------  底层区，需要根据平台进行配置 ---------------//
// 共享变量
extern uint32_t tick, pb12_tick, pb13_tick, pb14_tick;


// 等待忙状态结束
void Driver_Wait_Busy(void)
{
    while (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin) == GPIO_PIN_SET) // 表示忙状态
    {
        // 等待
        __NOP();
    }
}



// 发送回调逻辑
void Driver_TransmitCallback(void)
{
	llcc68_status_t Res;
	// 处理发送完成事件
	// 清除TxDone中断标志

	Res = llcc68_clear_irq_status(NULL, LLCC68_IRQ_TX_DONE);

    uint8_t send_buf[] = "blue";
    uint8_t length = sizeof(send_buf) -1;

    // 重启发送模式
    //LLCC68_Wait_Busy();
    //Res = LLCC68_Send(send_buf, length);
    //if (Res != LLCC68_STATUS_OK)
    //{
    //	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, SET);
    //}
    // 发送成功指示灯
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
    //pb12_tick = tick;
}

// 接收回调逻辑
// 数据目标提取缓存区，缓存区大小
void Driver_ReceiveCallback(uint8_t* receive_buf, uint8_t buf_size)
{
	llcc68_status_t Res;
	// 获取芯片接收缓冲区状态：数据长度和起始位置
	llcc68_rx_buffer_status_t rx_buffer_status;
	Res = llcc68_get_rx_buffer_status(NULL, &rx_buffer_status);	// 通过函数把状态信息放进结构体，后续可以直接访问
	uint8_t pld_len = rx_buffer_status.pld_len_in_bytes;
	uint8_t start_ptr = rx_buffer_status.buffer_start_pointer;
	// 读取接收到的数据
	memset(receive_buf, 0, buf_size);
	Res = llcc68_read_buffer(NULL, start_ptr, receive_buf, pld_len);	// 仅读取接收到的有效数据，不读取因未填满缓冲区而产生的空数据
	// 清除RxDone中断标志
	Res = llcc68_clear_irq_status(NULL, LLCC68_IRQ_RX_DONE);
	// 重启接收模式
	Res = LLCC68_Receive();

	// 读取到的数据发送到串口
    HAL_UART_Transmit_DMA(&huart1, receive_buf, pld_len);

	// 接收成功指示灯

	// 测试
	if (strncmp((char*)receive_buf, "blue", pld_len) == 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, SET);
		pb13_tick = tick;
	}
	if (strncmp((char*)receive_buf, "red", pld_len) == 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);
		pb14_tick = tick;
	}
}

// -------------  回调函数，根据平台底层编写 ---------------- //
// 本文件回调底层放进主程序代码中






