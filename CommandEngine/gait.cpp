#include "gait.h"
#include <cmath>
#include <unistd.h>
#include "ServoDriver.h"
#include <iostream>

using namespace std;


char *servoPositions;

Leg::Leg() {
}

Leg::Leg(uint8_t legNumber, float phaseAngleOffset) {
    this->servoZIndex = legNumber * 2;
    this->servoXIndex = this->servoZIndex + 1;
    this->phaseAngleOffset = phaseAngleOffset;
    this->strideDirection = LEG_DIRECTION_FORWARD;
}


void Leg::moveLegTo_Z(int angle) {
    servoPositions[this->servoZIndex] = angle + this->strideLength + this->zOpenPos;
    cout << "Z: " << (unsigned int)servoPositions[this->servoZIndex] << '\n';
}


void Leg::moveLegTo_X(int angle) {
    servoPositions[this->servoXIndex] = angle + this->strideHeight + this->xOpenPos;
}

void Leg::moveByPhase(float deltaPhaseAngle, int incline) {
    //this->phaseAngle += deltaPhaseAngle * this->strideDirection;
    this->phaseAngle += deltaPhaseAngle;
    float offsettedPhaseAngle = this->phaseAngle + this->phaseAngleOffset;
    // Normalize to prevent overflows
    offsettedPhaseAngle = fmodf(fmodf(offsettedPhaseAngle, TWO_PI) + TWO_PI, TWO_PI);
    // Convert to degrees
    int zPos = this->strideLength * cos(offsettedPhaseAngle);
    int xPos = this->strideHeight * sin(offsettedPhaseAngle + ((M_PI / 4) * incline));
    this->moveLegTo_Z(zPos);
    this->moveLegTo_X(xPos);
}

void Leg::setDirectionForward() {
    this->strideDirection = LEG_DIRECTION_FORWARD;
}

void Leg::setDirectionBackward() {
    this->strideDirection = LEG_DIRECTION_BACKWARD;
}

void Leg::enableLeg() {
    this->enabled = true;
}

void Leg::disableLeg() {
    this->enabled = false;
}

bool Leg::isEnabled() {
    return this->enabled;
}

void Leg::setStrideHeight(int strideHeight) {
    this->strideHeight = strideHeight;
}

int Leg::getStrideHeight() {
    return this->strideHeight;
}

void Leg::setStrideLength(int strideLength) {
    this->strideLength = strideLength;
}

int Leg::getStrideLength() {
    return this->strideLength;
}

void Leg::closeX() {
    servoPositions[this->servoXIndex] = this->xClosedPos;
}

void Leg::closeZ() {
    servoPositions[this->servoZIndex] = (this->zClosedPos);
}

void Leg::closeLeg() {
    if (this->currentState == STATE_CLOSING) {
        this->closeX();
        this->currentState = STATE_MOVING;
    }
    else {
        this->currentState = STATE_CLOSING;
        this->closeZ();
    }
}

void Leg::openX() {
    servoPositions[this->servoXIndex] = (this->xOpenPos);
}

void Leg::openZ() {
    servoPositions[this->servoZIndex] = this->zOpenPos;
}

void Leg::openLeg() {
    if (this->currentState == STATE_OPENING) {
        this->openZ();
        this->currentState = STATE_MOVING;
    }
    else {
        this->currentState = STATE_OPENING;
        this->openX();
    }
}


GaitControl::GaitControl() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs.push_back(Leg(i, (M_PI / 8) * i));
    }
}

void GaitControl::setSpeed(int speed) {
    this->speed = speed;
}

int GaitControl::getSpeed() {
    return this->speed;
}

// 2pi * period * time
void GaitControl::updateGait(unsigned int deltaTime) {
    float deltaPhaseAngle = TWO_PI * ((1 / this->speed) * deltaTime) * this->translationDirection;
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].moveByPhase(deltaPhaseAngle, incline);
        }
    }
}

void GaitControl::openPebble() {
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].openLeg();
        }
    }
    servoDriverWriteCommands();
    usleep(SERVO_COMMAND_DELAY_USECS);
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].openLeg();
        }
    }
    servoDriverWriteCommands();
}


void GaitControl::closePebble() {
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].closeLeg();
        }
    }
    servoDriverWriteCommands();
    usleep(SERVO_COMMAND_DELAY_USECS);
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].closeLeg();
        }
    }
    servoDriverWriteCommands();
}


void GaitControl::setDirection(int direction) {
    this->translationDirection = direction;
}
