/*
 * led.h
 *
 *  Created on: 2017年12月11日
 *      Author: ai-thinker
 */

#ifndef MAIN_LED_H_
#define MAIN_LED_H_

#ifdef __cplusplus
extern "C" {
#endif
void led_init() ;
void led_open();
void led_close();
bool get_light_state(void) ;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_LED_H_ */
