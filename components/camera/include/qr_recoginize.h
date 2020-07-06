/*
 * qr_recoginize.h
 *
 *  Created on: 2017年12月31日
 *      Author: sky
 */

#ifndef MAIN_QR_RECOGINIZE_H_
#define MAIN_QR_RECOGINIZE_H_

enum{
	RECONGIZE_OK,
	RECONGIZE_FAIL
};

#ifdef __cplusplus
extern "C" {
#endif

void qr_recoginze(void *pdata);
//int qr_recoginze() ;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_QR_RECOGINIZE_H_ */
