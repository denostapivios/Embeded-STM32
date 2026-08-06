# АЦП. Читання даних з Фоторезистора(LDR) (STM32)
**STM32 зчитує аналогову напругу з піна PA4, перетворює її в цифрове значення та передає RAW-дані й розраховану напругу на комп’ютер через USB**

**Налаштовано:**
- ADC1_IN4 на піні PA4;
- 12-бітний АЦП із діапазоном значень 0–4095;
- USB Device у режимі CDC;
- USB-піни:
  1. PA11 — USB D−;
  2. PA12 — USB D+.
<img width="500" height="480" alt="для датчика світла" src="https://github.com/user-attachments/assets/ba2b7f27-0207-4d79-a71c-5007effdbbf6" />

**У програмі реалізовано:**
- запуск перетворення АЦП через HAL_ADC_Start();
- очікування завершення вимірювання через HAL_ADC_PollForConversion();
- отримання RAW-значення через HAL_ADC_GetValue();
- перетворення RAW-значення в напругу за формулою:
  **voltageMillivolts = (rawValue * 3300UL + 2047UL) / 4095UL**;

**Формат повідомлення:**

<img width="694" height="464" alt="Знімок екрана 2026-08-06 о 16 03 35" src="https://github.com/user-attachments/assets/f710d61e-5e86-43b0-8a67-660103a017c9" />
