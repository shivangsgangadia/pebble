#ifndef _GAIT_H
#define _GAIT_H

#include <vector>
#include <cstdint>

#define TRANSLATION_DIRECTION_FORWARD 1
#define TRANSLATION_DIRECTION_BACKWARD -1
#define TURN_DIRECTION_LEFT 0
#define TURN_DIRECTION_RIGHT 1
#define TURN_DIRECTION_NONE 2

#define LEG_COUNT 4
#define LEG_DIRECTION_FORWARD 1
#define LEG_DIRECTION_BACKWARD -1
#define LEG_ACCELERATION_FACTOR 0.1

#define STATE_OPENING 1
#define STATE_CLOSING 2
#define STATE_MOVING 3
#define SERVO_COMMAND_DELAY_USECS 100000

#define GAIT_STATE_MOVE 0
#define GAIT_STATE_STOP 1

#define TWO_PI (2 * M_PI)

using namespace std;

class CameraServo {
private:
    int servoPos, servoPosMax = 90, servoIndex;
public:
    CameraServo();
    void stepRight();
    void stepLeft();
};

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
    int zPos, xPos, zOpenPos = 0, xOpenPos = 0, zClosedPos = 0, xClosedPos = 30;
    int currentState, strideDirection;
    float maxStrideHeight = 20, maxStrideLength = 40, currentStrideHeight=0.0f, currentStrideLength=0.0f;
    float phaseAngleOffset, phaseAngle = 0.0f;
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
    
    void setStrideLength(float strideLength);
    float getStrideLength();
    
    void setStrideHeight(float strideHeight);
    float getStrideHeight();
    
    void accelerateStrideLength(float maxStrideLength);
    void decelerateStrideLength();
    
    void closeX();
    void closeZ();
    void closeLeg();
    
    void openX();
    void openZ();
    void openLeg();
};


class GaitControl {
private:
    int translationDirection, state, turnDirection;
    float incline = 0.0, speed;
    vector<Leg> legs;
public:
    /*!
     * Initializes legs and sets their offsets
     * */
    GaitControl();
    void setGaitState(int state);
    /*!
     * Set speed in frequency of oscillations
     * */
    void setSpeed(float speed);
    float getSpeed();
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
    
    void accelerate();
    void decelerate();
    
    void incrementIncline();
    void decrementIncline();
    
    void setTurnDirection(int turn);
};


#endif

