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
    uint8_t finalAngle = (angle + this->strideLength + this->zOpenPos);
    // servoPositions[this->servoZIndex] = ((finalAngle % 180) + 180) % 180;
    servoPositions[this->servoZIndex] = finalAngle;
    // cout << "Z: " << (unsigned int)servoPositions[this->servoZIndex] << '\n';
}


void Leg::moveLegTo_X(int angle) {
    uint8_t finalAngle = angle + this->strideHeight + this->xOpenPos;
    // servoPositions[this->servoXIndex] = ((finalAngle % 180) + 180) % 180;
    servoPositions[this->servoXIndex] = finalAngle;
}

void Leg::moveByPhase(float deltaPhaseAngle, int incline) {
    //this->phaseAngle += deltaPhaseAngle * this->strideDirection;
    this->phaseAngle += deltaPhaseAngle;
    float offsettedPhaseAngle = this->phaseAngle + this->phaseAngleOffset;
    // Normalize to prevent overflows
    offsettedPhaseAngle = fmodf(fmodf(offsettedPhaseAngle, TWO_PI) + TWO_PI, TWO_PI);
    // cout << "Phase angle for leg: " << offsettedPhaseAngle << '\n';
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
    this->setDirection(TRANSLATION_DIRECTION_FORWARD);
}

void GaitControl::setSpeed(int speed) {
    this->speed = speed;
}

int GaitControl::getSpeed() {
    return this->speed;
}

// 2pi * period * time
void GaitControl::updateGait(float deltaTime) {
    float deltaPhaseAngle = TWO_PI * (this->speed * deltaTime * 100) * this->translationDirection;
    // cout << deltaPhaseAngle << '\n';
    for (int i = 0; i < LEG_COUNT; i++) {
        if (this->legs[i].isEnabled()) {
            this->legs[i].moveByPhase(deltaPhaseAngle, this->incline);
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
