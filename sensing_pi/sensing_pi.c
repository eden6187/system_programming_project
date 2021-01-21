#include <wiringPi.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "spi_connect.h"

#define USING_DHT11 true
#define DHT_GPIO 17
#define LH_THRESHOLD 26

int t_humid;
int t_temp;
int t_lux;
int t_press;
int lux = 0;
int press = 0;

int sockfd = 0, n = 0;
char recvBuff[1024];
char sendBuff[1024];
struct sockaddr_in serv_addr;


int sleep_state = 0;
int do_rest = 0;
// extern int power_state; // 전원 on :1 // 전원 off: 0
// extern int sleep_state; //온습도 센싱 잠시 멈춤 // 센싱중 :0 //센싱 멈춤:1
// extern int do_rest;
// extern int do_off;  //
// extern int again;
// extern int do_again;

int spi(){
    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0) {
        printf("Device %s not found\n", DEVICE);
        return -1;
    }

    if (prepare(fd) == -1) {
        return -1;
    }

    usleep(1000000);
    uint8_t i;

    t_lux = readadc(fd,0);
    t_press = readadc(fd,7);
    
    close(fd);

    return 0;
}

int temp_humid_sensor(){
    int validity = 0;
    int humid = 0, temp = 0;

    wiringPiSetupGpio();
    piHiPri(99); // 타이밍 코드를 위해서 가장 높은 우선 순위를 사용한다.

    /*
    * data[0] = 온도 소수점 윗자리
    * data[1] = 온도 소수
    * data[2] = 습도 소수점 윗자리
    * data[3] = 습도 소수
    * data[4] = 패리티 비트
    */
    while(validity == 0){
        unsigned char data[5] = {0,0,0,0,0};

        pinMode(DHT_GPIO, OUTPUT);
        digitalWrite(DHT_GPIO, LOW);
        usleep(18000);
        digitalWrite(DHT_GPIO, HIGH);
        pinMode(DHT_GPIO, INPUT);

        do{ delayMicroseconds(1); } while(digitalRead(DHT_GPIO) == HIGH);
        do{ delayMicroseconds(1); } while(digitalRead(DHT_GPIO) == LOW);
        do{ delayMicroseconds(1); } while(digitalRead(DHT_GPIO) == HIGH);

        for(int d = 0; d < 5; d++){
            for(int i = 0 ; i < 8; i++){
                do {delayMicroseconds(1);}while(digitalRead(DHT_GPIO) == LOW); // Low 무시

                int width = 0;
                do{
                    width++;
                    delayMicroseconds(1);
                    if(width > 1000) break; // HIGH가 지속됨 -> 무효!!
                }while (digitalRead(DHT_GPIO) == HIGH);

                data[d] = data[d] | ((width > LH_THRESHOLD) << (7-i));
            }
        }

        humid = data[0];
        temp = data[2];

        unsigned char chk = 0;
        for(int i = 0; i < 4; i++){
            chk += data[i];
        }

        if(chk == data[4]){
            validity = 1;

            t_humid = humid;
            t_temp = temp;
            
        }else{
            usleep(2000000);
            printf("the checksum is bad retry!\n");
        }
    }
    printf("humid/temp correct\n");
}

void socket_connect(){
    memset(recvBuff, 0, sizeof(recvBuff));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
	}

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
	}

	write(sockfd, sendBuff, sizeof(sendBuff));
	while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
	{
		recvBuff[n] = 0;
        if(strcmp(recvBuff, "temp_rest") == 0){
            do_rest = 1;
        }
        // if(strcmp(recvBuff,"power_off") == 0){
        //     do_off = 1;
        // }
        // if(strcmp(recvBuff,"power_on") == 0){
        //     do_off = 0;
        // }
        // if(strcmp(recvBuff,"watch_again")== 0){
        //     again = 1;
        // }
		if(fputs(recvBuff, stdout) == EOF)
		{
			printf("\n Error : Fputs error\n");
		}
        memset(recvBuff, 0, sizeof(recvBuff));
	}

	if(n < 0)
	{
		printf("\n Read complete \n");
	}
}   

int main(int argc, char *argv[])
{
    pthread_t p_thread[2];
    int sleeping_thread;
    sleeping_thread = pthread_create(&p_thread[0],NULL,state_change_sensor,sleep_state);
    if(sleeping_thread<0){
        perror("thread create error...");
        exit(0);
    }
    
    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return 1;
	}

    while(1){ 
        
        printf("now sleep state : %d\n", sleep_state);
        temp_humid_sensor();
        printf("온도 : %d\n",t_temp);        
        spi();
        printf("습도 : %d\n",t_humid);
        if(t_lux > 500) { //"lux_high"
            strcpy(sendBuff, "lux_high");
            printf("lux high\n");
            socket_connect();
            if(do_again=1){
                continue;
            }
        }
        if(sleep_state==0){
            if(t_humid > 90){  //"humidity_high"
                printf("1\n");
                strcpy(sendBuff, "humidity_high");
                printf("temp 1\n");
                socket_connect();
                if(do_again=1){
                    continue;
                }
            }
            if(t_humid<= 10){ //"humidity_low"
                strcpy(sendBuff, "humidity_low");
                printf("temp 2\n");
                socket_connect();
                if(do_again=1){
                    continue;
                }
            }
            if(t_temp >26){ //"temp_high"  //>17
                strcpy(sendBuff, "temp_high");
                printf("temp 3\n");
                socket_connect();
                if(do_again=1){
                    continue;
                }
            }
            if(t_temp < 20){ //"temp_low"  //<13
                strcpy(sendBuff, "temp_low");
                printf("temp 4\n");
                socket_connect();
                if(do_again=1){
                    continue;
                }
            }
        }
        if(t_press < 400){ //"move_detect"
            strcpy(sendBuff, "move_detect");
            printf("pressure 1\n");
            socket_connect();
            if(do_again=1){
                    continue;
            }
        }
        printf("sensing\n");
    
    }
    return 0;
}

// gcc -o sensing_pi sensing_pi.c -lwiringPi -lpthread
