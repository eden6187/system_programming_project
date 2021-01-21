#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_MAX 4
#define PATH_MAX 128
#define VALUE_MAX 64

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 2
#define SWITCH 17
#define BUTTON 22
#define LED 23

bool switch_power = true;
/*
* 사용 하고자 하는 GPIO의 pin 번호를 입력하여 사용 권한을 획득한다.
*/

static int GPIOExport(int pin)
{

  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open export for writing!!\n");
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  //buffer에 BUFFER_MAX만큼 "%d" 형식으로 pin을 저장
  write(fd, buffer, bytes_written);
  //fd에 buffer에 들어 있는 값을 출력된 바이트 만큼 저장한다.
  close(fd);

  return (0);
}

/*
* 사용이 끝난 GPIO의 pin 번호를 입력하여 사용 권한 반납한다.
*/
static int GPIOUNExport(int pin)
{
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;

  int fd;
  fd = open("/sys/class/gpio/unexport", O_WRONLY);

  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open unexport for writing! \n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIODirection(int pin, int dir)
{
  static const char s_direction_str[] = "in\0out";
  char path[PATH_MAX];
  int fd;

  snprintf(path, PATH_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (fd == -1)
  {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return -1;
  }

  if (write(fd, &s_direction_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3) == -1)
  {
    fprintf(stderr, "Failed to set direction\n");
    return -1;
  }

  close(fd);
  return (0);
}

static int GPIORead(int pin)
{
  char path[PATH_MAX];
  char value_str[VALUE_MAX];
  int fd;

  snprintf(path, PATH_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);

  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return -1;
  }

  if (-1 == read(fd, value_str, VALUE_MAX))
  {
    fprintf(stderr, "Failed to read value!\n");
    return -1;
  }

  // printf("%s\n", value_str);

  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
  char path[PATH_MAX];
  char value_str[VALUE_MAX] = "01";
  int fd;

  snprintf(path, PATH_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);

  if (fd == -1)
  {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return -1;
  }

  if (write(fd, &value_str[value == LOW ? 0 : 1], 1) != 1)
  {
    fprintf(stderr, "Failed to write value!\n");
    return -1;
  }

  close(fd);
  return (0);
}

int controlLED(int turn_flag)
{
  if (GPIOExport(LED) == -1)
  {
    return -1;
  }
  if (GPIODirection(LED, OUT) == -1)
  {
    return -2;
  }
  GPIOWrite(LED, turn_flag);
}

void *checkSwitch(void *data)
{
  if (GPIOExport(SWITCH) == -1)
  {
    exit(0);
  }
  if (GPIODirection(SWITCH, IN) == -1)
  {
    exit(0);
  }

  int previous_switch_state = GPIORead(SWITCH);
  switch_power = previous_switch_state;
  int now_switch_state;
  while (1)
  {
    now_switch_state = GPIORead(SWITCH);
    switch_power = now_switch_state;
    if (previous_switch_state != now_switch_state)
    {
      previous_switch_state = now_switch_state;
      // printf("change to %d\n", now_switch_state);
      if (now_switch_state == 1)
      {
        switch_power = true;
        printf("SWITCH TURN ON\n");
      }
      else if (now_switch_state == 0)
      {
        switch_power = false;
        printf("SWITCH TURN OFF\n");
        controlLED(0);
      }
    }
    usleep(100000);
  }
}

int checkResetButton()
{
  if (GPIOExport(BUTTON) == -1)
  {
    return -1;
  }
  if (GPIODirection(BUTTON, IN) == -1)
  {
    return -2;
  }

  int previous_button_state = GPIORead(BUTTON);
  int now_button_state;
  while (1)
  {
    if (!switch_power)
    {
      controlLED(0);
      return 1;
    }
    now_button_state = GPIORead(BUTTON);
    if (previous_button_state != now_button_state)
    {
      printf("Reset Button Pressed\n");
      controlLED(0);
      return 1;
      // 버튼이 눌렸을 때, 온/습도의 경우를 따로 알려줘서 15분간 온습도로 인한
      // warning이 안생기도록 한다.
    }
    usleep(100000);
  }
}