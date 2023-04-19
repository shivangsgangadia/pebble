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
    servoPositions[this->servoZIndex] = finalAngle;
    // cout << "Z: " << (unsigned int)servoPositions[this->servoZIndex] << '\n';
}


void Leg::moveLegTo_X(int angle) {
    uint8_t finalAngle = angle + this->strideHeight + this->xOpenPos;
    servoPositions[this->servoXIndex] = finalAngle;
}

void Leg::moveByPhase(float deltaPhaseAngle, float incline) {
    //this->phaseAngle += deltaPhaseAngle * this->strideDirection;
    this->phaseAngle += deltaPhaseAngle * this->strideDirection;
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
    servoPositions[this->servoZIndex] = this->zClosedPos + this->strideLength;
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
    servoPositions[this->servoZIndex] = this->zOpenPos + this->strideLength;
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
    this->legs.push_back(Leg(0, 0));
    this->legs.push_back(Leg(1, M_PI));
    this->legs.push_back(Leg(2, M_PI));
    this->legs.push_back(Leg(3, M_PI + M_PI));
    // for (int i = 0; i < LEG_COUNT; i++) {
        // this->legs.push_back(Leg(i, (M_PI / 2) * i));
	// this->legs[i].disableLeg();
    // }
    // this->legs[0].enableLeg();
    this->legs[2].setDirectionBackward();
    this->legs[3].setDirectionBackward();
    this->setDirection(TRANSLATION_DIRECTION_FORWARD);
}

void GaitControl::setSpeed(float speed) {
    this->speed = speed;
}

float GaitControl::getSpeed() {
    return this->speed;
}

// 2pi * period * time
void GaitControl::updateGait(float deltaTime) {
    float deltaPhaseAngle = TWO_PI * (this->speed * deltaTime * 10) * this->translationDirection;
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

void GaitControl::incrementStrideHeight() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs[i].setStrideHeight(this->legs[i].getStrideHeight() + 1);
    }
}

void GaitControl::decrementStrideHeight() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs[i].setStrideHeight(this->legs[i].getStrideHeight() - 1);
    }
}

void GaitControl::incrementStrideLength() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs[i].setStrideLength(this->legs[i].getStrideLength() + 1);
    }
}

void GaitControl::decrementStrideLength() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs[i].setStrideLength(this->legs[i].getStrideLength() - 1);
    }
}

void GaitControl::incrementIncline() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->incline += 0.1;
    }
}

void GaitControl::decrementIncline() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->incline -= 0.1;
    }
}
