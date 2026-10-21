#ifndef APP_H
#define APP_H

#include <stdint.h>

void setup(void);
void loop(void);

void App_USB_Receive(const uint8_t *data, uint32_t len);

#endif /* APP_H */
