# Датчик освітленості LDR на STM32 з фільтрацією SMA
Реалізував систему автоматичного керування LED залежно від рівня освітленості за допомогою LDR та ADC на STM32.

**Що реалізовано:**

- Підключення LDR через voltage divider до ADC1_IN0 (PA0).
- Налаштування 12-bit ADC з програмним запуском одиночного вимірювання.
- Реалізація Circular Buffer для зберігання останніх ADC вимірювань.
- Фільтрація сигналу за допомогою Simple Moving Average (SMA).
- Оптимізований розрахунок SMA через підтримку поточної суми buffer_sum.
- Керування LED через enum-based state machine.
- Реалізація hysteresis з двома порогами для запобігання мерехтінню LED.
- Hardware initialization виконується через STM32CubeMX / STM32 HAL.

  <img width="424" height="412" alt="Знімок екрана 2026-09-10 о 11 50 40" src="https://github.com/user-attachments/assets/5bfae1e1-24c3-4c3f-85a0-594f587cf024" />


  Демо: https://youtube.com/shorts/VRtfo6BWUYg
