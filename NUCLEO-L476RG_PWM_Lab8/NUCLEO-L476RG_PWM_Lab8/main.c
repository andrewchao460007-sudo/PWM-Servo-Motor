#include "stm32l476xx.h"

/*
 * Lab 8: PWM and Servo Motor Control
 *
 * Select the part to run:
 *   1 = Part A: LED PWM at fixed 50% duty cycle
 *   2 = Part B: LED continuously fades 0% -> 100% -> 0%
 *   3 = Part C: Servo 0 deg -> +90 deg -> -90 deg -> 0 deg, then stop
 *   4 = Part D: Keypad control (Key 1 = Servo sequence, Key 2 = LED fade toggle)
 */
#define LAB_PART 4

#define LED_PIN    5U      /* PA5 = TIM2_CH1 */
#define SERVO_PIN  0U      /* PA0 = TIM5_CH1 */

/* 100 kHz TIM5 clock -> 1 timer tick = 10 us */ 
#define SERVO_NEG_90 100U  /* 1.0 ms pulse */
#define SERVO_CENTER 150U  /* 1.5 ms pulse */
#define SERVO_POS_90 200U  /* 2.0 ms pulse */

static volatile uint32_t msTicks = 0U;

void LED_PWM_Pin_Init(void);
void TIM2_Init(void);
void Servo_Pin_Init(void);
void TIM5_Init(void);
void Servo_Set_Position(uint32_t ccr1_value);
void SysTick_Delay_Init(void);
void Delay_ms(uint32_t ms);
void Keypad_Matrix_Init(void);
char Keypad_GetKey(void);

void SysTick_Handler(void)
{
    msTicks++;
}

int main(void)
{
#if LAB_PART == 1

    LED_PWM_Pin_Init();
    TIM2_Init();

    while (1)
    {
    }

#elif LAB_PART == 2

    int32_t brightness = 0;
    int32_t direction = 1;
    volatile uint32_t i;
    
    LED_PWM_Pin_Init();
    TIM2_Init();

    TIM2->CCR1 = 0U;

    while (1)
    {
        brightness += direction;

        if (brightness >= 999)      { brightness = 999; direction = -1; }
        else if (brightness <= 0)   { brightness = 0;   direction = 1;  }

        TIM2->CCR1 = (uint32_t)brightness;

        for (i = 0U; i < 1000U; i++)
        {
            __NOP();
        }
    }

#elif LAB_PART == 3

    Servo_Pin_Init();
    TIM5_Init();
    SysTick_Delay_Init();

    Servo_Set_Position(SERVO_CENTER);
    Delay_ms(1000U);

    Servo_Set_Position(SERVO_POS_90);
    Delay_ms(1000U);

    Servo_Set_Position(SERVO_NEG_90);
    Delay_ms(1000U);

    Servo_Set_Position(SERVO_CENTER);

    while (1)
    {
    }

#elif LAB_PART == 4

    LED_PWM_Pin_Init();
    TIM2_Init();
    TIM2->CCR1 = 0U; // Start with LED turned off

    Servo_Pin_Init();
    TIM5_Init();
    SysTick_Delay_Init();
    Keypad_Matrix_Init();

    uint8_t led_enabled = 0;
    int32_t brightness = 0;
    int32_t direction = 1;

    Servo_Set_Position(SERVO_CENTER);

    while (1)
    {
        char key = Keypad_GetKey();

        /* --- Key '1': Servo Sequence --- */
        if (key == '1')
        {
            Servo_Set_Position(SERVO_CENTER);
            Delay_ms(1000U);

            Servo_Set_Position(SERVO_POS_90);
            Delay_ms(1000U);

            Servo_Set_Position(SERVO_NEG_90);
            Delay_ms(1000U);

            Servo_Set_Position(SERVO_CENTER);
        }

        /* --- Key '2': Toggle LED Fade ON/OFF --- */
        if (key == '2')
        {
            led_enabled = !led_enabled;
            if (!led_enabled)
            {
                TIM2->CCR1 = 0U; // Clear LED output when toggled off
            }
        }

        /* --- Non-blocking LED Fading --- */
        if (led_enabled)
        {
            brightness += direction;

            if (brightness >= 999)      { brightness = 999; direction = -1; }
            else if (brightness <= 0)   { brightness = 0;   direction = 1;  }

            TIM2->CCR1 = (uint32_t)brightness;
            Delay_ms(1U);
        }
    }

#else
#error "LAB_PART must be 1, 2, 3, or 4"
#endif
}

void LED_PWM_Pin_Init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (2 * 5));
    GPIOA->MODER |=  (2U << (2 * 5));

    GPIOA->AFR[0] &= ~(0xFU << (4 * 5));
    GPIOA->AFR[0] |=  (1U << (4 * 5));
        
    GPIOA->PUPDR &= ~(0x3U << (2U * LED_PIN));
}

void TIM2_Init(void)
{        
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    TIM2->CR1 &= ~TIM_CR1_DIR;
    TIM2->PSC = 39U;
    TIM2->ARR = 999U;

    TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM2->CCER &= ~TIM_CCER_CC1P;
    TIM2->CCER |= TIM_CCER_CC1E;

    TIM2->CCR1 = 500U;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void Servo_Pin_Init(void)
{        
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (2 * 0));
    GPIOA->MODER |=  (2U << (2 * 0));

    GPIOA->AFR[0] &= ~(0xFU << (4 * 0));
    GPIOA->AFR[0] |=  (2U << (4 * 0));

    GPIOA->PUPDR &= ~(0x3U << (2U * SERVO_PIN));
}

void TIM5_Init(void)
{        
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;

    TIM5->CR1 &= ~TIM_CR1_DIR;
    TIM5->PSC = 39U;
    TIM5->ARR = 1999U;

    TIM5->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM5->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM5->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM5->CCER &= ~TIM_CCER_CC1P;
    TIM5->CCER |= TIM_CCER_CC1E;

    TIM5->CCR1 = SERVO_CENTER;
    TIM5->CR1 |= TIM_CR1_CEN;
}

void Servo_Set_Position(uint32_t ccr1_value)
{
    TIM5->CCR1 = ccr1_value;
}

void SysTick_Delay_Init(void)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = 4000U - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = msTicks;

    while ((msTicks - start) < ms)
    {
    }
}

void Keypad_Matrix_Init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    GPIOC->MODER &= ~((3U << (2 * 0)) | (3U << (2 * 1)) | (3U << (2 * 2)) | (3U << (2 * 3)));
    GPIOC->MODER |=  ((1U << (2 * 0)) | (1U << (2 * 1)) | (1U << (2 * 2)) | (1U << (2 * 3)));

    GPIOC->ODR |= (0xFU << 0);

    GPIOC->MODER &= ~((3U << (2 * 4)) | (3U << (2 * 10)) | (3U << (2 * 11)) | (3U << (2 * 12)));

    GPIOC->PUPDR &= ~((3U << (2 * 4)) | (3U << (2 * 10)) | (3U << (2 * 11)) | (3U << (2 * 12)));
    GPIOC->PUPDR |=  ((1U << (2 * 4)) | (1U << (2 * 10)) | (1U << (2 * 11)) | (1U << (2 * 12)));
}

char Keypad_GetKey(void)
{
    const char keymap[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    const uint8_t col_pins[4] = {4, 10, 11, 12};

    for (uint8_t row = 0; row < 4; row++)
    {
        GPIOC->ODR |= (0xFU << 0);
        GPIOC->ODR &= ~(1U << row);

        for (volatile int d = 0; d < 20; d++) { __NOP(); }

        for (uint8_t col = 0; col < 4; col++)
        {
            uint8_t col_pin = col_pins[col];
            
            if ((GPIOC->IDR & (1U << col_pin)) == 0)
            {
                Delay_ms(20);

                if ((GPIOC->IDR & (1U << col_pin)) == 0)
                {
                    uint32_t timeout = 100000;
                    while (((GPIOC->IDR & (1U << col_pin)) == 0) && (--timeout > 0));

                    GPIOC->ODR |= (0xFU << 0);
                    return keymap[row][col];
                }
            }
        }
    }

    GPIOC->ODR |= (0xFU << 0);
    return 0;
}