#include "STM32DuinoPWM_PinDefs.hpp"
#include <Arduino.h>

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
    float frequency;
    float dutyCycle;

  public:
    OutputPWM(uint8_t pin, float freq = 128, float duty = 50.0f);

    void begin() override;
    void   end() override;

    void enable();
    void disable();

    void setFrequency(float freq);
    void setDutyCycle(float duty);

    float getFrequency() const { return frequency; }
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
