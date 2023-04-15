// #include "OLED.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <math.h>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <csignal>

#define DISPLAY_ROW 0
#define REQUEST_LENGTH 6
#define PACKET_DELAY 1
#define SERVER_PORT 8090
#define IMAGE_WIDTH 500
#define IMAGE_HEIGHT 500

using namespace std;

mutex imageBufferMutex;
mutex cameraThreadMutex;
mutex mainThreadMutex;
bool quit_camera_thread;
bool quit_server_thread;
int bytesUsed;
uint8_t *dataBuffer;

struct buffer {
  void* start;
  int length;
  struct v4l2_buffer inner;
  struct v4l2_plane plane;
};

// mmaps the buffers for the given type of device (capture or output).
void map(int fd, uint32_t type, struct buffer* buffer) {
  struct v4l2_buffer *inner = &buffer->inner;

  memset(inner, 0, sizeof(*inner));
  inner->type = type;
  inner->memory = V4L2_MEMORY_MMAP;
  inner->index = 0;

  ioctl(fd, VIDIOC_QUERYBUF, inner);

  buffer->length = inner->length;
  buffer->start = mmap(NULL, buffer->length, PROT_READ | PROT_WRITE,
      MAP_SHARED, fd, inner->m.offset);
}

/**
 * Reads image data into dataBuffer in JPEG format
 * */
int imageReader() {
  dataBuffer = (uint8_t*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT);
  int frameCounter = 0;
  // 1.  Open the device
  int raspiCamFileDescriptor; // A file descriptor to the video device
  raspiCamFileDescriptor = open("/dev/video0", O_RDWR);
  if (raspiCamFileDescriptor < 0) {
    perror("Failed to open device, OPEN");
    return 1;
  }
  cout << "Camera opened" << endl;

  // 2. Ask the device if it can capture frames
  v4l2_capability capability;
  if (ioctl(raspiCamFileDescriptor, VIDIOC_QUERYCAP, &capability) < 0) {
    // something went wrong... exit
    perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
    return 1;
  }

  cout << "Got camera capabilities, camera can capture frames." << endl;

  // 3. Set Image format
  v4l2_format imageFormat;
  v4l2_pix_format_mplane *mp = &imageFormat.fmt.pix_mp;
  //mp->field = V4L2_FIELD_NONE;

  // For capture
  imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctl(raspiCamFileDescriptor, VIDIOC_G_FMT, &imageFormat);

  mp->width = IMAGE_WIDTH;
  mp->height = IMAGE_HEIGHT;
  mp->pixelformat = V4L2_PIX_FMT_MJPEG;
  if(ioctl(raspiCamFileDescriptor, VIDIOC_S_FMT, &imageFormat) < 0) {
	  perror("Error setting format, VIDIOC_S_FMT");
	  return 1;
  }

  cout << "Image format set." << endl;
  
  // Buffer setup
  struct v4l2_requestbuffers reqBuf;
  reqBuf.memory = V4L2_MEMORY_MMAP;
  reqBuf.count = 1;
  
  struct buffer capture;
  reqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(raspiCamFileDescriptor, VIDIOC_REQBUFS, &reqBuf) < 0) {
	  perror("Requesting buffers failed, VIDIOC_REQBUFS");
	  return 1;
  }
  // printf("map_beg\n");
  map(raspiCamFileDescriptor, V4L2_BUF_TYPE_VIDEO_CAPTURE, &capture);
  // printf("map_end\n");

  // Queue buffers
  if(ioctl(raspiCamFileDescriptor, VIDIOC_QBUF, &capture.inner) < 0) {
	  perror("Unable to queue buffers, VIDIOC_QBUF");
	  return 1;
  }

  // Activate streaming
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(raspiCamFileDescriptor, VIDIOC_STREAMON, &type) < 0) {
	  perror("Stream on error, VIDIOC_STREAMON");
	  return 1;
  }

  struct v4l2_buffer buf;
  int encoded_len = 0;
  bool keepRunning = true;
  /***************************** Begin looping here *********************/
  while(keepRunning) {
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = 1;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(raspiCamFileDescriptor, VIDIOC_DQBUF, &buf);

    encoded_len = buf.bytesused;
    // printf("Bytes captured: %d \n", encoded_len);
    
    imageBufferMutex.lock();
    //printf("icb\n");
    bytesUsed = encoded_len;
    memcpy(dataBuffer, capture.start, encoded_len);
    imageBufferMutex.unlock();
    //printf("ice\n");

    ioctl(raspiCamFileDescriptor, VIDIOC_QBUF, &buf);

    cameraThreadMutex.lock();
    if (quit_camera_thread) {
	    keepRunning = false;
    }
    cameraThreadMutex.unlock();

  }

  /******************************** end looping here **********************/

  // end streaming
  int type2 = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  ioctl(raspiCamFileDescriptor, VIDIOC_STREAMOFF, &type2);

  close(raspiCamFileDescriptor);
  cout << "Camera thread quit successfully." << '\n';
  return 0;
}

void ctrl_c_handler(int signum) {
	mainThreadMutex.lock();
	quit_server_thread = true;
	mainThreadMutex.unlock();
	cout << "User interrupt, shutting down..." << '\n';
}

int main() {
  // Setup ctrl c handler
  mainThreadMutex.lock();
  quit_server_thread = false;
  mainThreadMutex.unlock();
  signal (SIGINT, ctrl_c_handler);

  // Let the image reader thread do it's job
  cameraThreadMutex.lock();
  quit_camera_thread = false;
  cameraThreadMutex.unlock();
  thread imageReaderThread(imageReader);
  // imageReaderThread.detach();

  // Display local IP address for convenience
  ifaddrs *allAddrs;
  getifaddrs(&allAddrs);
  ifaddrs *tmp = allAddrs;

  while (tmp) {
    // Sort out the one address that is associated with wifi
    if (tmp->ifa_addr->sa_family == AF_INET && strcmp(tmp->ifa_name, "wlan0") == 0) {
      struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
    }
    tmp = tmp->ifa_next;
  }
  freeifaddrs(allAddrs);

  // Begin image server on network
  int imageServerFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  int connectedSocketFileDescriptor;
  if (imageServerFileDescriptor < 0) {
    cout << "Unable to open socket";
    return 1;
  }
  sockaddr_in address;
  int receivedBytesCount;
  vector<char> requestBuffer(REQUEST_LENGTH);

  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_family = AF_INET;
  address.sin_port = htons(SERVER_PORT);
  int opt = 1;
  if (setsockopt(imageServerFileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    perror("Error setting address reuse");
    return 0;
  }
  // Setting socket timeout
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  if (setsockopt(imageServerFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Error setting socket timeout\n");
    return 0;
  }

  if (bind(imageServerFileDescriptor, (struct sockaddr *)&address, sizeof(address)) < 0) {
    cout << "Unable to bind" << endl;
    return 1;
  }

  bool continueServing = true;

  printf("Listening at %d\n", address.sin_port);
  while (continueServing) {
    mainThreadMutex.lock();
    if (quit_server_thread) {
	    continueServing = false;
    }
    mainThreadMutex.unlock();

    listen(imageServerFileDescriptor, 5);
    connectedSocketFileDescriptor = accept(imageServerFileDescriptor, NULL, 0);
    receivedBytesCount = recv(connectedSocketFileDescriptor, requestBuffer.data(), requestBuffer.size(), 0);

    if (receivedBytesCount > 0) {
      printf("serv req\n");

      switch (requestBuffer[0]) {
      case 'I': {
        imageBufferMutex.lock();
        // Send image data as a stream
        send(connectedSocketFileDescriptor, dataBuffer, bytesUsed, 0);
        imageBufferMutex.unlock();
        close(connectedSocketFileDescriptor);
        break;
      }

      case 'E':
        continueServing = false;
	close(connectedSocketFileDescriptor);
        close(imageServerFileDescriptor);
        break;
      }
    }
  }
  close(imageServerFileDescriptor);

  cout << "Server Closed successfully." << '\n';

  // Gracefully terminate image reader thread
  cameraThreadMutex.lock();
  quit_camera_thread = true;
  cameraThreadMutex.unlock();

  imageReaderThread.join();

}
