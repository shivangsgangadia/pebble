#include "ServoDriver.h"
#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <csignal>
#include "commands.h"
#include "gait.h"


#define ROTATION_SPEED 50


#define COMMAND_TIMEOUT_SECONDS 1
#define COMMAND_TIMEOUT_MICROSECONDS 0
#define COMMAND_SIZE 2

using namespace std;

void commandInterpreter(uint8_t[], float);
clock_t timer;
GaitControl gaitController;
bool keepRunning;

void ctrl_c_handler(int signum) {
  keepRunning = false;
}

int main() {
  // Register signal for graceful shutdown
  signal(SIGINT, ctrl_c_handler);
  gaitController.setSpeed(0.5);
  // Initialize driver
  int servoControllerFd = servoDriverInit(0);
  #ifndef TEST_MODE
  if (servoControllerFd < 0) {
    perror("Unable to init servo driver, exiting...");
    return 1;
  }
  #endif
  servoDriverWriteCommands();

  #ifndef TEST_MODE
  // Initialize UDP server
  int socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
  // Set timeout in case connection is broken mid command sequence
  struct timeval timeout;
  timeout.tv_sec = COMMAND_TIMEOUT_SECONDS;
  timeout.tv_usec = COMMAND_TIMEOUT_MICROSECONDS;
  if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Error setting socket timeout\n");
    return 0;
  }
  
  struct sockaddr_in server, client;
  unsigned int clientAddressSize = sizeof(client);
  uint8_t recvBuffer[COMMAND_SIZE];
  char receivedByteString[9];

  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  server.sin_port = htons(8080);

  if (bind(socketFileDescriptor, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Binding failed\n");
    return 0;
  }
  #endif

  int receivedBytesCount = 0;
  float deltaTime = 0.0f;
  timer = clock();
  cout << "Loop starting...\n";
  keepRunning = true;

  while (keepRunning) {
    #ifndef TEST_MODE
    //printf("Listening at %d\n", server.sin_port);

    receivedBytesCount = recvfrom(
        socketFileDescriptor,
        recvBuffer,
        2,
        0,
        (struct sockaddr *)&client,
        (socklen_t *)&clientAddressSize);
    
    // if (receivedBytesCount > 0) {
    // }
    
    clientAddressSize = sizeof(client);
    commandInterpreter(recvBuffer, deltaTime);
    // Clear the receive buffer
    memset(recvBuffer, 0, sizeof(uint8_t) * COMMAND_SIZE);
    
    #else
    // Put testing code here
    gaitController.setDirection(TRANSLATION_DIRECTION_FORWARD);
    gaitController.updateGait(deltaTime);
    servoDriverWriteCommands();
    usleep(200000);
    #endif
    
    deltaTime = ((float)(clock() - timer)) / CLOCKS_PER_SEC;
    timer = clock();
    

    // for (int i = 0; i < 8; i++) {
    //   receivedByteString[i] = ((recvBuffer[1] & (0b10000000 >> i)) == 0b10000000 >> i) + '0';
    // }

    // printf("%s\n", receivedByteString);
  }
  #ifndef TEST_MODE
  close(socketFileDescriptor);
  #endif
  // Release i2c channel
  servoDriverDeInit(servoControllerFd);
  cout << "User interrupt, shutting down...\n";
  return 0;
}

bool hasCommand(uint8_t commandByte, uint8_t mask) {
  return ((commandByte & mask) == mask);
}

/**
 * The first byte is used to detect mouse click events and the second byte is used to detect keyboard and mouse clicks
 */
void commandInterpreter(uint8_t commandBytes[], float dT) {

  //cout << "intr_str\n";
  
  if (hasCommand(commandBytes[1], OPEN_PEBBLE)) {
    gaitController.openPebble();
    cout << "Open\n";
  }
  else if (hasCommand(commandBytes[1], CLOSE_PEBBLE)) {
    gaitController.closePebble();
    cout << "Close\n";
  }
  else {
    // Up down
    if (hasCommand(commandBytes[0], TRANSLATE_FORWARD)) {
      gaitController.setDirection(TRANSLATION_DIRECTION_FORWARD);
      gaitController.updateGait(dT);
      // cout << "forward\n";
    } else if (hasCommand(commandBytes[0], TRANSLATE_BACKWARD)) {
      gaitController.setDirection(TRANSLATION_DIRECTION_BACKWARD);
      gaitController.updateGait(dT);
      // cout << "backward\n";
    }
    
    if (hasCommand(commandBytes[1], INCREMENT_STRIDE_HEIGHT)) {
      gaitController.incrementStrideHeight();
    }
    else if (hasCommand(commandBytes[1], DECREMENT_STRIDE_HEIGHT)) {
      gaitController.decrementStrideHeight();
    }
    
    if (hasCommand(commandBytes[1], INCREMENT_STRIDE_LENGTH)) {
      gaitController.incrementStrideLength();
    }
    else if (hasCommand(commandBytes[1], DECREMENT_STRIDE_LENGTH)) {
      gaitController.decrementStrideLength();
    }
    
    if (hasCommand(commandBytes[1], INCREMENT_INCLINE)) {
      gaitController.incrementIncline();
    }
    else if (hasCommand(commandBytes[1], DECREMENT_INCLINE)) {
      gaitController.decrementIncline();
    }
    
  
    servoDriverWriteCommands();
    //cout << "intr_fin\n";
    
  }
  
}
