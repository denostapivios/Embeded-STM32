
---

###  UART Communication — STM32F401

У цьому проєкті реалізовано UART-комунікацію між **STM32F401 та ESP32-S3**.

STM32 може передавати команду ESP32 для керування LED, а також приймати команду від ESP32 для керування власним LED.

## Реалізовано

- UART-зв'язок з **ESP32-S3**.
- Передачу та прийом даних через UART.
- Передачу ASCII-команди `'2'` → ESP32.
- Прийом ASCII-команди `'1'` ← ESP32.
- Керування LED через UART-команди.
- Обробку натискання кнопки через EXTI.
- Debounce кнопки.
- Прийом UART через interrupt (`HAL_UART_Receive_IT`).
- UART-конфігурацію **115200, 8N1**.

## Принцип роботи

### STM32 → ESP32

При натисканні кнопки на STM32:

```text
STM32 Button
     ↓
UART: '2'
     ↓
ESP32
     ↓
Toggle Blue LED
```
## Демо:https://youtu.be/7Owqmrdz_9s


<img width="598" height="545" alt="Знімок екрана 2026-10-06 о 17 27 02" src="https://github.com/user-attachments/assets/ad1913b6-92f7-4619-83cd-755664b60eb7" />

