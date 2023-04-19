#ifndef _GAIT_H
#define _GAIT_H

#include <vector>
#include <cstdint>

#define TRANSLATION_DIRECTION_FORWARD 1
#define TRANSLATION_DIRECTION_BACKWARD -1

#define LEG_COUNT 4
#define LEG_DIRECTION_FORWARD 1
#define LEG_DIRECTION_BACKWARD -1

#define STATE_OPENING 1
#define STATE_CLOSING 2
#define STATE_MOVING 3
#define SERVO_COMMAND_DELAY_USECS 100000

#define TWO_PI (2 * M_PI)

using namespace std;

/*!
 * This is meant to be an interface layer between gait logic and motor control.
 * The indices refer to the data positions (0 - 180) in a global array (servoPositions)
 * The Pos's refer to the angular positions of the legs (-90 - + 90)
 * Stride length and height refer to the maximum range of motion (-90 - + 90)
 * All use degrees as units.
 * */

class Leg {
private:
    int servoZIndex, servoXIndex;
    int zPos, xPos, zOpenPos = 0, xOpenPos = 20, zClosedPos = 0, xClosedPos = 0;
    int currentState;
    int strideHeight = 20, strideLength = 20, strideDirection;
    float phaseAngleOffset, phaseAngle = 0;
    // These convert angles in range (-90to+90) into angles in range(0to180)
    void moveLegTo_Z(int angle);
    void moveLegTo_X(int angle);
    bool enabled=true;
    
public:
    Leg();
    Leg(uint8_t legNumber, float phaseAngleOffset);
    int stepLegPositive_Z();
    int stepLegNegative_Z();
    /*!
     * Here, phase angle is in radians.
     * */
    void moveByPhase(float deltaPhaseAngle, float incline);
    
    void setDirectionForward();
    void setDirectionBackward();
    
    void enableLeg();
    void disableLeg();
    bool isEnabled();
    
    void setStrideLength(int strideLength);
    int getStrideLength();
    
    void setStrideHeight(int strideHeight);
    int getStrideHeight();
    
    void closeX();
    void closeZ();
    void closeLeg();
    
    void openX();
    void openZ();
    void openLeg();
};


class GaitControl {
private:
    int speed, translationDirection;
    float incline = 0.0;
    vector<Leg> legs;
public:
    /*!
     * Initializes legs and sets their offsets
     * */
    GaitControl();
    /*!
     * Set speed in frequency of oscillations
     * */
    void setSpeed(int speed);
    int getSpeed();
    /*!
     * Periodic function to be called once every game loop.
     * @param time is in seconds
     * */
    void updateGait(float deltaTime);
    void setDirection(int direction);
    
    void openPebble();
    void closePebble();
    
    void incrementStrideHeight();
    void decrementStrideHeight();
    
    void incrementStrideLength();
    void decrementStrideLength();
    
    void incrementIncline();
    void decrementIncline();
};


#endif

