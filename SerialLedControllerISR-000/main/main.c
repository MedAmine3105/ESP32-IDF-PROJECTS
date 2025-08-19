/**
*
* Author : DABOUSSI MED AMINE
* Date : 12/08/25
* Project : serial led controller and interuption ISR
*
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "define.h" 

/**
 * Breif :
 * 
 * 
 * 
 */

 
 uint8_t MODE = __ModeOff ;
 uint8_t ButtonState = 0 ;

 static const char *TAG = "SERIAL_COMMAND : ";

 void INIT_UART (void)
 {
    const uart_config_t uart_config =
    {
        .baud_rate = 115200,
        .stop_bits = UART_STOP_BITS_1 ,
        .parity = UART_PARITY_DISABLE ,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE ,
        .data_bits = UART_DATA_8_BITS ,
    };
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
 }

 static void IRAM_ATTR button_isr_handler(void* arg) 
 {
    if(MODE == 0){MODE = 1;}
    else if (MODE == 1){MODE = 0;}
    else if (MODE == 2){MODE = 0;}
 }

void app_main () 
{
    uint8_t data[BUF_SIZE];

    INIT_UART () ;
    gpio_reset_pin(__GpioLed1) ;
    gpio_reset_pin(__GpioButton1) ;

    gpio_set_direction(__GpioLed1 , GPIO_MODE_OUTPUT) ;

    gpio_set_direction(__GpioButton1 , GPIO_MODE_INPUT) ;
    //gpio_input_enable(__GpioButton1) ;
    gpio_pulldown_en(__GpioButton1) ;

    // Install ISR service
    gpio_install_isr_service(0);
    gpio_set_intr_type(__GpioButton1, GPIO_INTR_POSEDGE); // falling edge (button press)
    gpio_isr_handler_add(__GpioButton1, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Button interrupt initialized");
    
    while(1)
    {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (len > 0) 
        {
            data[len] = '\0';
            if((char)data[0] == '0'){MODE = __ModeOff ;} // if data recieved is 0 turn led off
            else if((char)data[0] == '1'){MODE = __ModeOn ;} // if data recieved is 1 turn led on
            else if((char)data[0] == '2'){MODE = __ModeBlink ;} // if data recieved is 2 turn led blinking
            //ESP_LOGI(TAG, "MODE IS : %s", (char *)data);
            ESP_LOGI(TAG, "MODE IS : %c", data[0]);
        }
        if(MODE == __ModeOff ) 
        {
            gpio_set_level(__GpioLed1 , 0) ;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else if(MODE == __ModeOn ) 
        {
            gpio_set_level(__GpioLed1 , 1) ;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else if(MODE == __ModeBlink ) 
        {
            gpio_set_level(__GpioLed1 , 1) ;
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(__GpioLed1 , 0) ;
        }
        /*
        if((gpio_get_level(__GpioButton1) == 1)&&(ButtonState == 0)) // change state of led in each click on button
        {
            ESP_LOGI(TAG, "BUTTON CLICKED !!");
            if(MODE == __ModeOff){MODE = __ModeOn;}
            else if (MODE == __ModeOn){MODE = __ModeOff;}
            else if (MODE == __ModeBlink){MODE = __ModeOff;}
            ButtonState = 1;
        }
        else if(gpio_get_level(__GpioButton1) == 0)
        {
            ButtonState = 0;
        }
        */
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
