#include "delay.h"

#ifdef __cplusplus
extern "C" {
#endif

void delay_ms(unsigned short nms){
    vTaskDelay(nms / portTICK_PERIOD_MS);
}
#ifdef __cplusplus
}
#endif
