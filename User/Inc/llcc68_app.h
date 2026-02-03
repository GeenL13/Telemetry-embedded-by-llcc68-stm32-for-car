/*
 * Wireless.h
 *
 *  Created on: Jan 24, 2026
 *      Author: Geen
 */

#ifndef INC_LLCC68_APP_H_
#define INC_LLCC68_APP_H_

#include "llcc68.h"

extern volatile uint8_t able_to_send;

llcc68_status_t LLCC68_Init(uint8_t model);
llcc68_status_t LLCC68_Send(uint8_t* send_buf, uint8_t send_buf_size);
llcc68_status_t LLCC68_Receive(void);
void LLCC68_Wait_Busy(void);
void LLCC68_TransmitCallback(void);
void LLCC68_ReceiveCallback(uint8_t* receive_buf, uint8_t buf_size);


void Driver_Wait_Busy(void);
void Driver_TransmitCallback(void);
void Driver_ReceiveCallback(uint8_t* receive_buf, uint8_t buf_size);


#endif /* INC_LLCC68_APP_H_ */
