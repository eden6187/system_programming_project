#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>


#define IN  0
#define OUT 1
#define PIN  20 
#define POUT 21 
#define VALUE_MAX    256 

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

int power_state = 1; // 전원 on :1 // 전원 off: 0
// int do_off = 0; 
extern int sleep_state;
extern int do_rest;
// int again = 0;
// int do_again = 0;


/*
 * Ensure all settings are correct for the ADC
 */
static int prepare(int fd) {

  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
    perror("Can't set MODE");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
    perror("Can't set number of BITS");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set write CLOCK");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set read CLOCK");
    return -1;
  }

  return 0;
}

/*
 * (SGL/DIF = 0, D2=D1=D0=0)
 */ 
uint8_t control_bits_differential(uint8_t channel) {
  return (channel & 7) << 4;
}

/*
 * (SGL/DIF = 1, D2=D1=D0=0)
 */ 
uint8_t control_bits(uint8_t channel) {
  return 0x8 | control_bits_differential(channel);
}

/*
 * Given a prep'd descriptor, and an ADC channel, fetch the
 * raw ADC value for the gi
 * ven channel.
 */
int readadc(int fd, uint8_t channel) {
  uint8_t tx[] = {1, control_bits(channel), 0};
  uint8_t rx[3];

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)tx, // 데이터 전송 버퍼
    .rx_buf = (unsigned long)rx, // 데이터 수신 버퍼
    .len = ARRAY_SIZE(tx),       // 버퍼의 길이
    .delay_usecs = DELAY,        // us 단위의 지연시간
    .speed_hz = CLOCK,           // Hz 단위의 속도
    .bits_per_word = BITS,       // 단어의 비트 수
  };
 
  // SPI 메시지(버퍼를 포함한 위의 모든 필드)를 송신
  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
    perror("IO Error");
    abort();
  }

  return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

void* state_change_sensor(void* data){
  sleep_state = (int)(data);
  while(1){
    if(do_rest == 1){
      sleep_state = 1;
      
      printf("온도측정 중지 10분\n");
      printf("thread 2 //sleep_state : %d\n",sleep_state);
      usleep(30000000);
      
      
      sleep_state = 0;
      printf("온도측정 시작\n");
      printf("sleep_state : %d\n",sleep_state);
      do_rest = 0;
    }
    usleep(100000);
  }
}



// int main(int argc, char **argv) {

 
// }


