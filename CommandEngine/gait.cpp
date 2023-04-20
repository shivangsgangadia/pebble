#include "gait.h"
#include <cmath>
#include <unistd.h>
#include "ServoDriver.h"
#include <iostream>

using namespace std;


char *servoPositions;

CameraServo::CameraServo() {
    this->servoIndex = SERVO_COUNT * 2;
    this->servoPos = 0;
    servoPositions[this->servoIndex] = (uint8_t) (this->servoPos + this->servoPosMax);
}

void CameraServo::stepLeft() {
    this->servoPos--;
    if (this->servoPos < -this->servoPosMax) {
        this->servoPos = -this->servoPosMax;
    }
    servoPositions[this->servoIndex] = (uint8_t) (this->servoPos + this->servoPosMax);
}

void CameraServo::stepRight() {
    this->servoPos++;
    if (this->servoPos > this->servoPosMax) {
        this->servoPos = this->servoPosMax;
    }
    servoPositions[this->servoIndex] = (uint8_t) (this->servoPos + this->servoPosMax);
}

Leg::Leg() {
}

Leg::Leg(uint8_t legNumber, float phaseAngleOffset) {
    this->servoZIndex = legNumber * 2;
    this->servoXIndex = this->servoZIndex + 1;
    this->phaseAngleOffset = phaseAngleOffset;
    this->strideDirection = LEG_DIRECTION_FORWARD;
}


void Leg::moveLegTo_Z(int angle) {
    servoPositions[this->servoZIndex] = (uint8_t)(angle + this->maxStrideLength + this->zOpenPos);
    // cout << "Z: " << (unsigned int)servoPositions[this->servoZIndex] << '\n';
}


void Leg::moveLegTo_X(int angle) {
    servoPositions[this->servoXIndex] = (uint8_t)(angle + this->maxStrideHeight + this->xOpenPos);
}

void Leg::moveByPhase(float deltaPhaseAngle, float incline) {
    this->phaseAngle += deltaPhaseAngle * this->strideDirection;
    float offsettedPhaseAngle = this->phaseAngle + this->phaseAngleOffset;
    // Normalize to prevent overflows
    offsettedPhaseAngle = fmodf(fmodf(offsettedPhaseAngle, TWO_PI) + TWO_PI, TWO_PI);
    // cout << "Phase angle for leg: " << offsettedPhaseAngle << '\n';
    // Convert to degrees
    int zPos = this->currentStrideLength * cos(offsettedPhaseAngle);
    int xPos = this->maxStrideHeight * sin(offsettedPhaseAngle + ((M_PI / 4) * incline));
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

void Leg::setStrideHeight(float strideHeight) {
    this->maxStrideHeight = strideHeight;
}

float Leg::getStrideHeight() {
    return this->maxStrideHeight;
}

void Leg::setStrideLength(float strideLength) {
    this->maxStrideLength = strideLength;
}

float Leg::getStrideLength() {
    return this->maxStrideLength;
}

void Leg::closeX() {
    servoPositions[this->servoXIndex] = this->xClosedPos;
}

void Leg::closeZ() {
    servoPositions[this->servoZIndex] = this->zClosedPos + this->maxStrideLength;
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
    servoPositions[this->servoZIndex] = this->zOpenPos + this->maxStrideLength;
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

void Leg::accelerateStrideLength(float maxStrideLength) {
    this->currentStrideLength += LEG_ACCELERATION_FACTOR * (maxStrideLength - this->currentStrideLength);
    if ((maxStrideLength - this->currentStrideLength) * this->strideDirection < 0.0f) {
        this->currentStrideLength = maxStrideLength * this->strideDirection;
    }
}

void Leg::decelerateStrideLength() {
    this->currentStrideLength += LEG_ACCELERATION_FACTOR * (0 - this->currentStrideLength);
    if ((0 - this->currentStrideLength) * this->strideDirection < 0.0f) {
        this->currentStrideLength = 0;
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
    this->setGaitState(GAIT_STATE_STOP);
}

void GaitControl::setGaitState(int state) {
    this->state = state;
}

void GaitControl::setSpeed(float speed) {
    this->speed = speed;
}

float GaitControl::getSpeed() {
    return this->speed;
}

// 2pi * period * time
void GaitControl::updateGait(float deltaTime) {
    if (this->state == GAIT_STATE_MOVE) {
        float deltaPhaseAngle = TWO_PI * (this->speed * deltaTime * 10) * this->translationDirection;
        // cout << deltaPhaseAngle << '\n';
        for (int i = 0; i < LEG_COUNT; i++) {
            if (this->legs[i].isEnabled()) {
                this->legs[i].moveByPhase(deltaPhaseAngle, this->incline);
            }
        }
    }
}

void GaitControl::accelerate() {
    if (this->turnDirection == TURN_DIRECTION_NONE) {
        for (int i = 0; i < LEG_COUNT; i++) {
            this->legs[i].accelerateStrideLength(this->legs[i].getStrideLength());
        }
    }
    else if (this->turnDirection == TURN_DIRECTION_LEFT) {
        this->legs[0].accelerateStrideLength(this->legs[0].getStrideLength() / 2);
        this->legs[1].accelerateStrideLength(this->legs[1].getStrideLength() / 2);
        this->legs[2].accelerateStrideLength(this->legs[2].getStrideLength());
        this->legs[3].accelerateStrideLength(this->legs[3].getStrideLength());
    }
    else if (this->turnDirection == TURN_DIRECTION_RIGHT) {
        this->legs[0].accelerateStrideLength(this->legs[0].getStrideLength());
        this->legs[1].accelerateStrideLength(this->legs[1].getStrideLength());
        this->legs[2].accelerateStrideLength(this->legs[2].getStrideLength() / 2);
        this->legs[3].accelerateStrideLength(this->legs[3].getStrideLength() / 2);
    }
}

void GaitControl::decelerate() {
    for (int i = 0; i < LEG_COUNT; i++) {
        this->legs[i].decelerateStrideLength();
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
