#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
 
#define EVENT_BIT_NEW_DATA (1 << 0)
typedef struct{
    int temp;
    int hum;
} sensor_data;
 
QueueHandle_t queue_sensor;
QueueHandle_t queue_alert;
EventGroupHandle_t event_group;
 
sensor_data latest_avg_data;
void task_a(void *prm)
{
    while(1)
    {
        sensor_data data;
        data.temp = (rand() % 18 ) +20; // Tạo giá trị bất kì cho nhiệt độ
        data.hum = (rand() % 30 ) + 40; //tạo giá trị bất kì cho độ ẩm
        xQueueSend(queue_sensor,&data,portMAX_DELAY); // Gửi vào queue
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms se lap lai message.
    }
}
 
void task_b(void *prm)
{
    sensor_data a[10];
    int count = 0;
    while(1)
    {
        sensor_data received;
        if(xQueueReceive(queue_sensor,&received,portMAX_DELAY) == pdPASS)
        {
            a[count] = received;
            count ++;
        if(count == 10 )
        {
            int sum_temp=0; int sum_hum=0;
            sum_temp += a[count].temp;
            sum_hum += a[count].hum;
        latest_avg_data.temp= sum_temp / 10;
        latest_avg_data.hum = sum_hum /10 ;
        count=0;
        xEventGroupSetBits(event_group, EVENT_BIT_NEW_DATA);
        }
        }
    }
}
void task_c(void *prm)
{
    while(1)
    {
        xEventGroupWaitBits(event_group, EVENT_BIT_NEW_DATA,pdTRUE, pdFALSE,
        portMAX_DELAY);
        printf(" [DOC GIA TRI] Nhiet do: %dC, Do Am: %d%%\n",latest_avg_data.temp,latest_avg_data.hum);
    }
}
void task_d(void *prm) {
    while (1) {
        xEventGroupWaitBits(event_group, EVENT_BIT_NEW_DATA, pdTRUE, pdFALSE, portMAX_DELAY);
        xQueueSend(queue_alert, &latest_avg_data, portMAX_DELAY);
    }
}
void task_e(void *prm) {
    while (1) {
        sensor_data data;
        if (xQueueReceive(queue_alert, &data, portMAX_DELAY) == pdPASS) {// Kiểm tra 
            if (data.temp > 28 && data.hum> 55) {
                printf("[ALERT] Nhiet do va do am qua cao!!! Nhiet do: %d°C, Do am: %d%%\n",
                       data.temp, data.hum);
            }
        }
    }
}
 
extern "C" void app_main(void) {
    queue_sensor = xQueueCreate(10, sizeof(sensor_data));
    queue_alert = xQueueCreate(10, sizeof(sensor_data));
    event_group = xEventGroupCreate();
    xTaskCreate(task_a, "task_a", 2048, NULL, 5, NULL);
    xTaskCreate(task_b, "Task_B", 2048, NULL, 5, NULL);
    xTaskCreate(task_c, "Task_C", 2048, NULL, 4, NULL);
    xTaskCreate(task_d, "Task_D", 2048, NULL, 4, NULL);
    xTaskCreate(task_e, "Task_E", 2048, NULL, 3, NULL);
}
