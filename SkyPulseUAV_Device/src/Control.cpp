#include <iostream>
#include <cmath>

#include "Control.h"

Control::Control() : Control(defaultSampleFreq) {}

Control::Control(float userDefinedFreq) :
    currentAngle{0.0f, 0.0f, 0.0f},
    currentRate{0.0f, 0.0f, 0.0f},

    prevRateErr{0.0f, 0.0f, 0.0f},
    prevRateIterm{0.0f, 0.0f, 0.0f},

    prevAngleErr{0.0f, 0.0f, 0.0f},
    prevAngleIterm{0.0f, 0.0f, 0.0f},

    desireAngle{0.0f, 0.0f, 0.0f},
    errorRate{0.0f, 0.0f, 0.0f},
    errorAngle{0.0f, 0.0f, 0.0f},
    outputAngle{0.0f, 0.0f, 0.0f},
    outputRate{0.0f, 0.0f, 0.0f},

    desireAlt(0.0f),
    altVelocity(0.0f),
    errorAlt(0.0f),
    currentAlt(0.0f),

    Kp_out{0.0f, 0.0f, 0.0f},
    Kp_in{0.0f, 0.0f, 0.0f},

    Ki_out{0.0f, 0.0f, 0.0f},
    Ki_in{0.0f, 0.0f, 0.0f},
    
    Kd{0.0f, 0.0f, 0.0f},

    inputThrottle(0.0f),
    PIDFreq(sampleFreq),
    motorOutput{0, 0, 0, 0},

    IRObstacleDetected(0),
    USDistance(0)
    {

        PIDFreqInv = 1000000 / PIDFreq;

        cout << "PID controller initialized successfully!" << endl;

    }

void Control::setMode(bool mode) 
{

    this->mode = mode;

}

void Control::readCur(float rate[3], float angle[3], float alt) 
{
    
    for(short i = 0;i < 3;i++) {

        currentAngle[i] = angle[i];

        // The data of gyroscope can be used here. 
        currentRate[i] = rate[i];
    }

    currentAlt = alt;

}

// Reference update (roll, pitch, yaw and altitude).
void Control::readRef(float refAngle[3], float alt) 
{

    for(short i = 0;i < 3;i++) {
    
        desireAngle[i] = refAngle[i];
        
    }

    desireAlt = alt;

}

// Read current data by callback.
void Control::readDataFromFilter(float roll, float pitch, float yaw, float rate[3]) 
{

    desireAngle[0] = roll;
    desireAngle[1] = pitch;
    desireAngle[2] = yaw;

    for (short i = 0; i < 3; i++) {

        currentRate[i] = rate[i];

    }
    
}

void Control::readDataFromBMP180(float altitude) 
{

    currentAlt= altitude;

}

void Control::readDataFromIR(uint8_t IRObstacleDetected) 
{

    this->IRObstacleDetected = IRObstacleDetected;

}

void Control::readDataFromUS(int USDistance) 
{

    this->USDistance = USDistance;

}

/* 
PID Control function of altitude.
Outter loop - altitude control
Inner loop - altitude rate control 
*/
void Control::altControl() 
{

    errorAlt = desireAlt - currentAlt;

}

/* 
PID double loop control function of three Euler angles.
Outter loop - angle control
Inner loop - rate control 
*/
void Control::doublePIDControl() 
{

    // Limit the roll and pitch angles.
    if (desireAngle[0] > MAX_ROLL_ANGLE || desireAngle[1] > MAX_PITCH_ANGLE) {

        desireAngle[0] = MAX_ROLL_ANGLE;
        desireAngle[1] > MAX_PITCH_ANGLE;

    }

    for(short i = 0; i < 3; i++) {

        errorAngle[i] = desireAngle[i] - currentAngle[i];

        prevAngleIterm[i] += errorAngle[i] * (0.5f / PIDFreq);

        if(prevAngleIterm[i] > ITERM_MAX) prevAngleIterm[i] = 0;
        else if(prevAngleIterm[i] < -ITERM_MAX) prevAngleIterm[i] = 0;

        outputAngle[i] = Kp_out[i] * errorAngle[i] + Ki_out[i] * prevAngleIterm[i];

        prevAngleErr[i] = errorAngle[i];

        errorRate[i] = outputAngle[i] - currentRate[i];

        prevRateIterm[i] += errorRate[i] * (0.5f / PIDFreq);
        if(prevRateIterm[i] > ITERM_MAX) prevRateIterm[i] = 0;
        else if(prevRateIterm[i] < -ITERM_MAX) prevRateIterm[i] = 0;

        outputRate[i] = Kp_in[i] * errorRate[i] + Ki_in[i] * prevRateIterm[i]
        + Kd[i] * (errorRate[i] - prevRateErr[i]) * (1.0f / PIDFreq);

        prevRateErr[i] = errorRate[i];
        
    }
}

void Control::getControlOuput() 
{

    doublePIDControl();
    //pidControlRate();

    // + or - depends on your motor rotation and position.
    motorOutput[0] = inputThrottle + outputRate[ROLL] - outputRate[PITCH] + outputRate[YAW];
    motorOutput[1] = inputThrottle - outputRate[ROLL] - outputRate[PITCH] - outputRate[YAW];
    motorOutput[2] = inputThrottle - outputRate[ROLL] + outputRate[PITCH] + outputRate[YAW];
    motorOutput[3] = inputThrottle + outputRate[ROLL] + outputRate[PITCH] - outputRate[YAW];

    if(callback_)
    {

        callback_(motorOutput);

    }

    // Sleep for several seconds.
    this_thread::sleep_for(microseconds(PIDFreqInv));

}

void Control::setCallback(CallbackFunction callback)
{

    callback_ = callback;

}

// int Control::getControlFreqInv()
// {

//     return (PIDFreqInv);

// }

