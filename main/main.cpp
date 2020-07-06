#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* MPU6050 */
#include "../components/MPU6050/include/mpu6050.hpp"
#include "../components/MPU6050/include/kalmanfilter.hpp"
#include <cmath>

/* SSD1306 */
#include "../components/SSD1306/include/ssd1306.h"
#include "../components/SSD1306/include/font8x8_basic.h"

/*  */
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "../components/camera/include/camera.h"
#include "../components/camera/include/bitmap.h"
#include "../components/http_server/include/my_http_server.h"
#include "../components/camera/include/qr_recoginize.h"
#include "../components/camera/include/led.h"
#define tag "TEST"

#define SCL_IO 15
#define SDA_IO 14
#define I2C_PORT_0 0

#define OLED_I2C_ADDRESS 0x3C

static const char *TAG = "camera_demo";

static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace; boundary=123456789000000000000987654321";

static const char *STREAM_BOUNDARY = "--123456789000000000000987654321";

EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static esp_ip4_addr_t s_ip_addr;
static camera_pixelformat_t s_pixel_format;

#define CAMERA_PIXEL_FORMAT CAMERA_PF_JPEG
#define CAMERA_FRAME_SIZE CAMERA_FS_SVGA

struct paramter
{
    i2c_config_t conf;
    i2c_port_t port;
};

SSD1306_t dev;

static void mpu6050_task(void *param)
{
    paramter p = *(paramter *)param;
    i2c_config_t conf = p.conf;
    i2c_port_t port = p.port;
    MPU6050 mpu(conf, port);

    if (!mpu.init())
    {
        ESP_LOGE("mpu6050", "init failed!");
        vTaskDelete(0);
    }
    ESP_LOGI("mpu6050", "init success!");

    float ax, ay, az, gx, gy, gz;
    float pitch, roll;
    float fpitch, froll;

    static KALMAN pfilter(0.005);
    static KALMAN rfilter(0.005);

    uint32_t lasttime = 0;
    int count = 0;

    while (1)
    {
        ax = -mpu.getAccX();
        ay = -mpu.getAccY();
        az = -mpu.getAccZ();
        gx = mpu.getGyroX();
        gy = mpu.getGyroY();
        gz = mpu.getGyroZ();
        pitch = atan(ax / az) * 57.2958;
        roll = atan(ay / az) * 57.2958;
        fpitch = pfilter.filter(pitch, gy);
        froll = rfilter.filter(roll, -gx);
        count++;
        if (esp_log_timestamp() / 1000 != lasttime)
        {
            lasttime = esp_log_timestamp() / 1000;
            ESP_LOGI("mpu6050", "Samples: %d", count);
            count = 0;
            ESP_LOGI("mpu6050", "Acc: ( %.3f, %.3f, %.3f)", ax, ay, az);
            ESP_LOGI("mpu6050", "Gyro: ( %.3f, %.3f, %.3f)", gx, gy, gz);
            ESP_LOGI("mpu6050", "Pitch: %.3f", pitch);
            ESP_LOGI("mpu6050", "Roll: %.3f", roll);
            ESP_LOGI("mpu6050", "FPitch: %.3f", fpitch);
            ESP_LOGI("mpu6050", "FRoll: %.3f", froll);
        }
    }
}

void ssd1306_task()
{
    int center, top, bottom;
    char lineChar[20];
    ESP_LOGI(tag, "Prepare ssd1306 128x64");
    // ssd1306初始化
    ssd1306_init(&dev, 128, 64, 0x3C);
}

void task_ssd1306_display_clear(void *param)
{
    int top = 2;
    int center = 3;
    int bottom = 8;
    char lineChar[20];
    while (1)
    {
        ssd1306_clear_screen(&dev, false);
        ssd1306_contrast(&dev, 0xff);
        ssd1306_display_text(&dev, 0, "SSD1306 128x64", 14, false);
        ssd1306_display_text(&dev, 1, "ABCDEFGHIJKLMNOP", 16, false);
        ssd1306_display_text(&dev, 2, "abcdefghijklmnop", 16, false);
        ssd1306_display_text(&dev, 3, "Hello World!!", 13, false);
        ssd1306_clear_line(&dev, 4, true);
        ssd1306_clear_line(&dev, 5, true);
        ssd1306_clear_line(&dev, 6, true);
        ssd1306_clear_line(&dev, 7, true);
        ssd1306_display_text(&dev, 4, "SSD1306 128x64", 14, true);
        ssd1306_display_text(&dev, 5, "ABCDEFGHIJKLMNOP", 16, true);
        ssd1306_display_text(&dev, 6, "abcdefghijklmnop", 16, true);
        ssd1306_display_text(&dev, 7, "Hello World!!", 13, true);

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display Count Down
        uint8_t image[24];
        memset(image, 0, sizeof(image));
        ssd1306_display_image(&dev, top, (6 * 8 - 1), image, sizeof(image));
        ssd1306_display_image(&dev, top + 1, (6 * 8 - 1), image, sizeof(image));
        ssd1306_display_image(&dev, top + 2, (6 * 8 - 1), image, sizeof(image));
        for (int font = 0x39; font > 0x30; font--)
        {
            memset(image, 0, sizeof(image));
            ssd1306_display_image(&dev, top + 1, (7 * 8 - 1), image, 8);
            memcpy(image, font8x8_basic_tr[font], 8);
            ssd1306_display_image(&dev, top + 1, (7 * 8 - 1), image, 8);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif

    static void handle_grayscale_pgm(http_context_t http_ctx, void *ctx);
    static void handle_rgb_bmp(http_context_t http_ctx, void *ctx);
    static void handle_rgb_bmp_stream(http_context_t http_ctx, void *ctx);
    static void handle_jpg(http_context_t http_ctx, void *ctx);
    static void handle_jpg_stream(http_context_t http_ctx, void *ctx);
    static esp_err_t event_handler(void *ctx, system_event_t *event);
    static void initialise_wifi(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    static esp_err_t i2c_master_init_tmp(gpio_num_t scl, gpio_num_t sda, i2c_port_t port, i2c_config_t conf)
    {
        int i2c_master_port = port;
        // i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sda;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = scl;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 400000;
        i2c_param_config(i2c_master_port, &conf);
        return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    }

    void app_main()
    {
        paramter param = {
            .port = I2C_PORT_0};
        ESP_ERROR_CHECK(i2c_master_init_tmp((gpio_num_t)SCL_IO, (gpio_num_t)SDA_IO, param.port, param.conf));
        ssd1306_task();
        void *i2c_param = &param;

        esp_log_level_set("wifi", ESP_LOG_WARN);
        esp_log_level_set("gpio", ESP_LOG_WARN);
        esp_err_t err = nvs_flash_init();
        if (err != ESP_OK)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }

        camera_config_t camera_config = {
            .pin_reset = CONFIG_RESET,
            .pin_xclk = CONFIG_XCLK,
            .pin_sscb_sda = CONFIG_SDA,
            .pin_sscb_scl = CONFIG_SCL,
            .pin_d7 = CONFIG_D7,
            .pin_d6 = CONFIG_D6,
            .pin_d5 = CONFIG_D5,
            .pin_d4 = CONFIG_D4,
            .pin_d3 = CONFIG_D3,
            .pin_d2 = CONFIG_D2,
            .pin_d1 = CONFIG_D1,
            .pin_d0 = CONFIG_D0,
            .pin_vsync = CONFIG_VSYNC,
            .pin_href = CONFIG_HREF,
            .pin_pclk = CONFIG_PCLK,
            .xclk_freq_hz = CONFIG_XCLK_FREQ,

            .ledc_timer = LEDC_TIMER_0,
            .ledc_channel = LEDC_CHANNEL_0,
        };

        camera_model_t camera_model;
        err = camera_probe(&camera_config, &camera_model);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Camera probe failed with error 0x%x", err);
            return;
        }

        if (camera_model == CAMERA_OV7725)
        {
            s_pixel_format = CAMERA_PIXEL_FORMAT;
            camera_config.frame_size = CAMERA_FRAME_SIZE;
            ESP_LOGI(TAG, "Detected OV7725 camera, using %s bitmap format",
                     CAMERA_PIXEL_FORMAT == CAMERA_PF_GRAYSCALE ? "grayscale" : "RGB565");
        }
        else if (camera_model == CAMERA_OV2640)
        {
            ESP_LOGI(TAG, "Detected OV2640 camera, using JPEG format");
            s_pixel_format = CAMERA_PIXEL_FORMAT;
            camera_config.frame_size = CAMERA_FRAME_SIZE;
            if (s_pixel_format == CAMERA_PF_JPEG)
                camera_config.jpeg_quality = 15;
        }
        else
        {
            ESP_LOGE(TAG, "Camera not supported");
            return;
        }

        camera_config.pixel_format = s_pixel_format;
        err = camera_init(&camera_config);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
            return;
        }
        //    databuf = (char *) malloc(BUF_SIZE);
        initialise_wifi();

        http_server_t server;
        http_server_options_t http_options = HTTP_SERVER_OPTIONS_DEFAULT();
        ESP_ERROR_CHECK(http_server_start(&http_options, &server));

        if (s_pixel_format == CAMERA_PF_GRAYSCALE)
        {
            ESP_ERROR_CHECK(http_register_handler(server, "/pgm", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_grayscale_pgm, &camera_config));
            ESP_LOGI(TAG, "Open http://" IPSTR "/pgm for a single image/x-portable-graymap image", IP2STR(&s_ip_addr));
        }
        if (s_pixel_format == CAMERA_PF_RGB565)
        {
            ESP_ERROR_CHECK(http_register_handler(server, "/bmp", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp, NULL));
            ESP_LOGI(TAG, "Open http://" IPSTR "/bmp for single image/bitmap image", IP2STR(&s_ip_addr));
            ESP_ERROR_CHECK(http_register_handler(server, "/bmp_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp_stream, NULL));
            ESP_LOGI(TAG, "Open http://" IPSTR "/bmp_stream for multipart/x-mixed-replace stream of bitmaps", IP2STR(&s_ip_addr));
        }
        if (s_pixel_format == CAMERA_PF_JPEG)
        {
            ESP_ERROR_CHECK(http_register_handler(server, "/jpg", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg, NULL));
            ESP_LOGI(TAG, "Open http://" IPSTR "/jpg for single image/jpg image", IP2STR(&s_ip_addr));
            ESP_ERROR_CHECK(http_register_handler(server, "/jpg_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg_stream, NULL));
            ESP_LOGI(TAG, "Open http://" IPSTR "/jpg_stream for multipart/x-mixed-replace stream of JPEGs", IP2STR(&s_ip_addr));
        }
        ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());
        ESP_LOGI(TAG, "Camera demo ready");

        // ssd1306_init_test();
        xTaskCreatePinnedToCore(&mpu6050_task, "mpu6050_task", 2048, i2c_param, 5, NULL, 0);
        // xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);
        xTaskCreatePinnedToCore(&task_ssd1306_display_clear, "ssd1306_display_clear", 2048, NULL, 5, NULL, 0);
    }

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    static esp_err_t write_frame(http_context_t http_ctx)
    {
        http_buffer_t fb_data = {
            .data = camera_get_fb(),
            .size = camera_get_data_size(),
            .data_is_persistent = true};
        return http_response_write(http_ctx, &fb_data);
    }

    static void handle_grayscale_pgm(http_context_t http_ctx, void *ctx)
    {
        esp_err_t err = camera_run();
        if (err != ESP_OK)
        {
            ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
            return;
        }
        char *pgm_header_str;
        asprintf(&pgm_header_str, "P5 %d %d %d\n",
                 camera_get_fb_width(), camera_get_fb_height(), 255);
        if (pgm_header_str == NULL)
        {
            return;
        }

        size_t response_size = strlen(pgm_header_str) + camera_get_data_size();
        http_response_begin(http_ctx, 200, "image/x-portable-graymap", response_size);
        http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.pgm");
        http_buffer_t pgm_header = {.data = pgm_header_str};
        http_response_write(http_ctx, &pgm_header);
        free(pgm_header_str);

        write_frame(http_ctx);
        http_response_end(http_ctx);
        ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());
#if CONFIG_QR_RECOGNIZE
        camera_config_t *camera_config = (camera_config_t *)ctx;
        xTaskCreate(qr_recoginze, "qr_recoginze", 111500, camera_config, 5, NULL);
#endif
    }

    static void handle_rgb_bmp(http_context_t http_ctx, void *ctx)
    {
        esp_err_t err = camera_run();
        if (err != ESP_OK)
        {
            ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
            return;
        }

        bitmap_header_t *header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
        if (header == NULL)
        {
            return;
        }

        http_response_begin(http_ctx, 200, "image/bmp", sizeof(*header) + camera_get_data_size());
        http_buffer_t bmp_header = {
            .data = header,
            .size = sizeof(*header)};
        http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.bmp");
        http_response_write(http_ctx, &bmp_header);
        free(header);

        write_frame(http_ctx);
        http_response_end(http_ctx);
    }

    static void handle_jpg(http_context_t http_ctx, void *ctx)
    {
        if (get_light_state())
            led_open();
        esp_err_t err = camera_run();
        if (err != ESP_OK)
        {
            ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
            return;
        }

        http_response_begin(http_ctx, 200, "image/jpeg", camera_get_data_size());
        http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.jpg");
        write_frame(http_ctx);
        http_response_end(http_ctx);
        led_close();
    }

    static void handle_rgb_bmp_stream(http_context_t http_ctx, void *ctx)
    {
        http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);
        bitmap_header_t *header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
        if (header == NULL)
        {
            return;
        }
        http_buffer_t bmp_header = {
            .data = header,
            .size = sizeof(*header)};

        while (true)
        {
            esp_err_t err = camera_run();
            if (err != ESP_OK)
            {
                ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
                return;
            }

            err = http_response_begin_multipart(http_ctx, "image/bitmap",
                                                camera_get_data_size() + sizeof(*header));
            if (err != ESP_OK)
            {
                break;
            }
            err = http_response_write(http_ctx, &bmp_header);
            if (err != ESP_OK)
            {
                break;
            }
            err = write_frame(http_ctx);
            if (err != ESP_OK)
            {
                break;
            }
            err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
            if (err != ESP_OK)
            {
                break;
            }
        }

        free(header);
        http_response_end(http_ctx);
    }

    static void handle_jpg_stream(http_context_t http_ctx, void *ctx)
    {
        http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);
        if (get_light_state())
            led_open();
        while (true)
        {
            esp_err_t err = camera_run();
            if (err != ESP_OK)
            {
                ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
                return;
            }
            err = http_response_begin_multipart(http_ctx, "image/jpg",
                                                camera_get_data_size());
            if (err != ESP_OK)
            {
                break;
            }
            err = write_frame(http_ctx);
            if (err != ESP_OK)
            {
                break;
            }
            err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
            if (err != ESP_OK)
            {
                break;
            }
        }
        http_response_end(http_ctx);
        led_close();
    }

    static esp_err_t event_handler(void *ctx, system_event_t *event)
    {
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            s_ip_addr = event->event_info.got_ip.ip_info.ip;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
        }
        return ESP_OK;
    }

    static void initialise_wifi(void)
    {
        tcpip_adapter_init();
        s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        //    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
        wifi_config_t wifi_config = {
            .sta = {
                {.ssid = CONFIG_WIFI_SSID},
                {.password = CONFIG_WIFI_PASSWORD},
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_LOGI(TAG, "Connecting to \"%s\"", wifi_config.sta.ssid);
        xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected");
    }
#ifdef __cplusplus
}
#endif