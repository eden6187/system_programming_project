#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "check.h"
#include "lcd.h"

int actuator_request()
{
  int client_fd = 0;
  struct sockaddr_in yh_server;

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create client socket \n");
    return 1;
  }
  memset(&yh_server, '0', sizeof(yh_server));

  yh_server.sin_family = AF_INET;
  yh_server.sin_port = htons(5000);

  if (inet_pton(AF_INET, "192.168.0.4", &yh_server.sin_addr) <= 0)
  {
    printf("\n inet_pton error occured\n");
    return 1;
  }
  if (connect(client_fd, (struct sockaddr *)&yh_server, sizeof(yh_server)) < 0)
  {
    printf("\n Error : Client Connect Failed \n");
    return 1;
  }
}

int main(int argc, char *argv[])
{
  pthread_t p_thread[2];
  int check_switch_thread;
  int switch_request_thread;
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(5000);

  char sendBuff[100];
  char receiveBuff[100];
  time_t ticks;

  bus_open();
  lcd_init();
  sleep(1);
  /* creates an UN-named socket inside the kernel and returns
	 * an integer known as socket descriptor
	 * This function takes domain/family as its first argument.
	 * For Internet family of IPv4 addresses we use AF_INET
	 */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff));
  memset(receiveBuff, 0, sizeof(receiveBuff));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  /* The call to the function "bind()" assigns the details specified
	 * in the structure 『serv_addr' to the socket created in the step above
	 */
  bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  /* The call to the function "listen()" with second argument as 10 specifies
	 * maximum number of client connections that server will queue for this listening
	 * socket.
	 */
  listen(listenfd, 10);
  controlLED(0);
  check_switch_thread = pthread_create(&p_thread[0], NULL, checkSwitch, NULL);
  if (check_switch_thread < 0)
  {
    perror("thread create error!");
    exit(0);
  }
  sleep(1);

  printf("Server started in port %s\n", argv[1]);
  while (1)
  {
    printf("전원 - %s\n", switch_power == 0 ? "OFF" : "ON");
    /* In the call to accept(), the server is put to sleep and when for an incoming
		 * client request, the three way TCP handshake* is complete, the function accept()
		 * wakes up and returns the socket descriptor representing the client socket.
		 */
    connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
    memset(sendBuff, 0, sizeof(sendBuff));
    memset(receiveBuff, 0, sizeof(receiveBuff));
    /* As soon as server gets a request from client, it prepares the date and time and
		 * writes on the client socket through the descriptor returned by accept()
		 */
    ticks = time(NULL);
    if (!switch_power)
    {
      strcpy(sendBuff, "power_off");
      write(connfd, sendBuff, strlen(sendBuff));
      printf("전원이 꺼져있으므로 요청을 무시합니다.\n");
      close(connfd);
      continue;
    }
    if (read(connfd, receiveBuff, sizeof(receiveBuff)))
    {
      printf("받은 data : %s\n", receiveBuff);
    }

    if (strcmp(receiveBuff, "move_detect") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string(" Move Detected!!! ", LCD_LINE_1);
      lcd_string("Check the pose", LCD_LINE_2);
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "watch_again");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else if (strcmp(receiveBuff, "temp_high") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string("    SO HOT!!!", LCD_LINE_1);
      lcd_string("   Check temp", LCD_LINE_2);
      actuator_request();
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "temp_rest");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else if (strcmp(receiveBuff, "temp_low") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string("    SO COLD!!!", LCD_LINE_1);
      lcd_string("   Check temp", LCD_LINE_2);
      actuator_request();
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "temp_rest");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else if (strcmp(receiveBuff, "humidity_high") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string("   SO HUMID!!!", LCD_LINE_1);
      lcd_string(" Check humidity", LCD_LINE_2);
      actuator_request();
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "temp_rest");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else if (strcmp(receiveBuff, "humidity_low") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string("    SO DRY!!!", LCD_LINE_1);
      lcd_string(" Check humidity", LCD_LINE_2);
      actuator_request();
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "temp_rest");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else if (strcmp(receiveBuff, "lux_high") == 0 && switch_power)
    {
      printf("Warning! LED ON!\n");
      controlLED(1);
      lcd_string("   SO BRIGHT!!!", LCD_LINE_1);
      lcd_string(" Check the Light", LCD_LINE_2);
      if (checkResetButton() == 1)
      {
        lcd_clear();
        memset(sendBuff, 0, sizeof(sendBuff));
        strcpy(sendBuff, "watch_again");
        write(connfd, sendBuff, sizeof(sendBuff));
      }
    }
    else
    {
      printf("\t—Bad Request—\n");
    }
    memset(receiveBuff, 0, sizeof(receiveBuff));
    sleep(1);

    close(connfd);
  }
}
// gcc -o server server.c -lpthread