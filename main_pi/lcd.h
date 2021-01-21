#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <string.h>

#define I2C_ADDR 0x27 // I2C device address
#define LCD_WIDTH 16  // Maximum characters per line

//Define some device constants
#define LCD_CHR 1 // Mode - Sending data
#define LCD_CMD 0 // Mode - Sending command

#define LCD_LINE_1 0x80 // LCD RAM address for the 1st line
#define LCD_LINE_2 0xC0 // LCD RAM address for the 2nd line

#define LCD_BACKLIGHT 0x08 // On
//LCD_BACKLIGHT 0x00 //Off

#define ENABLE 0b00000100 // Enable bit

//Timing constants
#define E_PULSE 0.005
#define E_DELAY 0.005

void lcd_init();
void lcd_byte(char bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_string(char *message, int line);
void bus_open();
void bus_read();
void bus_write_text(char a);

int file_i2c;
int length;
unsigned int buffer[60] = {0};
unsigned char text_buffer[10] = {0};

// int main()
// {
//   bus_open();
//   lcd_init();

//   sleep(1);

//   lcd_string("   SO BRIGHT!!!", LCD_LINE_1);
//   lcd_string(" Check the Light", LCD_LINE_2);

//   sleep(5);

//   lcd_string("   SO HUMID!!!", LCD_LINE_1);
//   lcd_string(" Check humidity", LCD_LINE_2);
//   return 0;
// }

void bus_open()
{
  char *filename = (char *)"/dev/i2c-1";
  if ((file_i2c = open(filename, O_RDWR)) < 0)
  {
    printf("Failed to open the i2c bus");
    return;
  }
  else
  {
    printf("Bus open success!\n");
  }
  if (ioctl(file_i2c, I2C_SLAVE, I2C_ADDR) < 0)
  {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    return;
  }
  else
  {
    printf("Get access to bus\n");
  }
}

void lcd_clear()
{
  lcd_byte(0x01, LCD_CMD);

  sleep(E_DELAY);
  printf("lcd clear ------------\n");
}

void lcd_init()
{
  printf("lcd_init start ------------\n");
  lcd_byte(0x33, LCD_CMD);
  lcd_byte(0x32, LCD_CMD);
  lcd_byte(0x06, LCD_CMD);
  lcd_byte(0x0C, LCD_CMD);
  lcd_byte(0x28, LCD_CMD);
  lcd_byte(0x01, LCD_CMD);

  sleep(E_DELAY);
  printf("lcd_init finished ---------\n");
}

void lcd_byte(char bits, int mode)
{
  char bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
  char bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

  bus_write_text(bits_high);
  lcd_toggle_enable(bits_high);

  bus_write_text(bits_low);
  lcd_toggle_enable(bits_low);
}

void bus_write(int bits)
{
  buffer[0] = bits;
  length = 1;
  int return_value;
  if ((return_value = write(file_i2c, buffer, length)) != length)
  {
    printf("Failed to write to the i2c bus.\n");
  }
  else
  {
    // printf("written bytes : %d\twrite value : %x\n", return_value, bits);
  }
}

void bus_write_text(char word)
{
  text_buffer[0] = word;
  length = 1;
  int return_value;
  if ((return_value = write(file_i2c, text_buffer, length)) != length)
  {
    printf("Failed to write to the i2c bus.\n");
  }
  else
  {
    // printf("written bytes : %d\twrite value : %c\n", return_value, word);
  }
}

void lcd_toggle_enable(int bits)
{
  sleep(E_DELAY);
  bus_write((bits | ENABLE));
  sleep(E_PULSE);
  bus_write((bits & ~ENABLE));
  sleep(E_DELAY);
}

void lcd_string(char *message, int line)
{
  lcd_byte(line, LCD_CMD);
  for (int i = 0; i < strlen(message); i++)
  {
    lcd_byte(message[i], LCD_CHR);
  }

  for (int i = strlen(message); i < LCD_WIDTH; i++)
  {
    lcd_byte(' ', LCD_CHR);
  }
}