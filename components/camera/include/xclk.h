#pragma once

#include "camera_common.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t camera_enable_out_clock();

void camera_disable_out_clock();

#ifdef __cplusplus
}
#endif