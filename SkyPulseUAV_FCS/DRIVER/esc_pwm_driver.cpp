#include "esc_pwm_driver.h"
#include <pigpio.h>
#include <QDebug>

ESC_PWM_Driver::ESC_PWM_Driver(QObject *parent)
{
    initial();
}

void ESC_PWM_Driver::initial()
{
    qDebug() << "PWM Driver Working!";
    if (gpioInitialise() < 0) {
        //
        qWarning("Failed to initialize pigpio library");
    }
    gpioSetMode(PWM0, PI_OUTPUT);
    gpioSetMode(PWM1, PI_OUTPUT);
    gpioSetMode(PWM2, PI_OUTPUT);
    gpioSetMode(PWM3, PI_OUTPUT);
    gpioSetPWMfrequency(PWM0, 100);
    gpioSetPWMfrequency(PWM1, 100);
    gpioSetPWMfrequency(PWM2, 100);
    gpioSetPWMfrequency(PWM3, 100);
}

void ESC_PWM_Driver::setPwmSignal(int gpioPin, int dutyCycle)
{
    // Check the legitimacy of parameters, e.g. dutyCycle is usually in the range 0 to 255
    if (dutyCycle < 0 || dutyCycle > 255) {
        qWarning("Invalid duty cycle");
        return;
    }
    qDebug() << "Gpio Pin: " << gpioPin << "," << "Duty cycle: " << dutyCycle;
    // Setting the PWM signal
    gpioPWM(gpioPin, dutyCycle);
}