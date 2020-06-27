#ifndef MAIN_DELAY_H_
#define MAIN_DELAY_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif
void delay_ms(unsigned short nms);
#ifdef __cplusplus
}
#endif

#endif