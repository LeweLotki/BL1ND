#pragma once

#include "freertos/FreeRTOS.h"

void keypad_init();
void keypad_start();

bool numpad_receive_key(int& key, TickType_t timeout);

// true  -> pojawił się nowy klawisz
// false -> czas minął normalnie
bool wait_for_new_key(int milliseconds, int& new_key);
