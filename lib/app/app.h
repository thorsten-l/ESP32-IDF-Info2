#pragma once

#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h> // Add this line to include the SemaphoreHandle_t type

extern SemaphoreHandle_t mutex;
extern bool setup_finished;

extern void delay(uint32_t ms);
extern void wifi_init_sta(void);
