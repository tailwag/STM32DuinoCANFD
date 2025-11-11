#include "STM32DuinoPWM_PinDefs.hpp"
#include "stm32g474xx.h"
#include <Arduino.h>
#include <cstdint>
#include <sys/types.h>

enum PwmRange {
  LOWFREQ, 
  HIGHFREQ,
};

TIM_TypeDef *getTimerForPin(uint8_t pin);
int8_t getChannelForPin(int8_t pin);
int8_t getOutputIdForPin(uint8_t pin);

/*  ---------------------------  *
 *  -- BASE TIMER CLASS      --  *
 *  ---------------------------  */
class STM32HALPWM {
  protected:
    HardwareTimer *halTimer;
    uint8_t pwmPin;
    int8_t channel;
    int8_t outputId; 
    bool initialized;

  public:
    STM32HALPWM(uint8_t pin);
    virtual ~STM32HALPWM();

    virtual void begin() = 0;
    virtual void   end() = 0;

    TIM_TypeDef *getTimerInstance() const;
    uint8_t getChannel() const { return channel; }

};

/*  ---------------------------  *
 *  -- PWM GENERATOR CLASS   --  *
 *  ---------------------------  */
class OutputPWM : public STM32HALPWM {
  private:
    uint32_t frequency;
    float dutyCycle;

  public:
    OutputPWM(uint8_t pin, uint32_t freq = 128, float duty = 50.0f);

    void begin() override;
    void   end() override;

    void enable();
    void disable();

    void setFrequency(uint32_t freq);
    void setDutyCycle(float duty);

    float getFrequency() const { return static_cast<float>(frequency); }
    float getDutyCycle() const { return dutyCycle; }
};

/*  ---------------------------  *
 *  -- PWM MEASUREMENT CLASS --  *
 *  ---------------------------  */
class InputPWM : public STM32HALPWM {
  private:
    volatile uint32_t lastRising;
    volatile uint32_t lastFalling;
    volatile uint32_t period;
    volatile uint32_t pulseWidth;

    uint16_t prescalerValue; 
    static void captureCallback(void *arg);

  public:
    InputPWM(uint8_t pin, PwmRange range=LOWFREQ);
    void handleCapture(); 

    void begin() override;
    void   end() override;

    float getFrequency();
    float getDutyCycle();
};
