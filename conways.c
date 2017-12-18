#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> //has rand funtion for random numbers
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>  
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#include "calculate_next_board.h"

#define NUM_LEDS BOARD_SIZE
#define SPI_CLK_SPEED 8E6

typedef struct {
  uint8_t brightness; //leading three bits must be ones.
  uint8_t blue;
  uint8_t green;
  uint8_t red;
} __attribute__((packed)) led_frame_t;

typedef struct spi_ioc_transfer spi_ioc_transfer_t;

const uint8_t start_frame[4] = {0};
led_frame_t led_frames[NUM_LEDS] = {0xFF};
const uint8_t end_frame[8] = {0};

struct spi_ioc_transfer transfers[3];

void randomly_seed_board(bool * board, uint16_t board_size){
  for (int16_t i = 0; i < board_size ; i++) {
    for (int16_t j = 0; j < board_size ; j++) {
      board[i + board_size * j] = rand() % 10  > 6;
    }
  }
}

void print_board(bool * board, uint16_t board_size, spi_ioc_transfer_t* transfers, uint32_t fd){
  const uint8_t size = 50;
  int8_t str[size];

  //system("clear");
  for (int16_t i = 0; i < board_size ; i++) {
    for (int16_t j = 0; j < board_size ; j++) {
      bool cell = board[i + board_size * j];
      const char * character = cell ? "x" : "_";
      if(i == 0 && cell)
        led_frames[j].blue = 0x05;
      if(i == 1 && cell)
        led_frames[j].red = 0x05;
      if(i == 2 && cell)
        led_frames[j].green = 0x05;

      //printf("%s", character);
    }
    //printf("\n");
  }

  for(int16_t i = 0; i < NUM_LEDS; i++){
    if(led_frames[i].blue > 0) 
      led_frames[i].blue-= 1;
    if(led_frames[i].red > 0) 
      led_frames[i].red-= 1;
    if(led_frames[i].green > 0) 
      led_frames[i].green-= 1;
  }
  int32_t err = ioctl(fd, SPI_IOC_MESSAGE(3), transfers);
}

void init_transfers(spi_ioc_transfer_t * transfers){
  transfers[0].tx_nbits = 8;
  transfers[0].delay_usecs = 0;
  transfers[0].speed_hz = SPI_CLK_SPEED;
  transfers[0].bits_per_word = 8;
  transfers[0].len = sizeof(start_frame);
  transfers[0].tx_buf = (unsigned long)start_frame;
  transfers[0].cs_change = 0;

  transfers[1].tx_nbits = 8;
  transfers[1].delay_usecs = 0;
  transfers[1].speed_hz = SPI_CLK_SPEED;
  transfers[1].bits_per_word = 8;
  transfers[1].len = sizeof(led_frames);
  transfers[1].tx_buf = (unsigned long)led_frames;
  transfers[1].cs_change = 0;

  transfers[2].tx_nbits = 8;
  transfers[2].delay_usecs = 0;
  transfers[2].speed_hz = SPI_CLK_SPEED;
  transfers[2].bits_per_word = 8;
  transfers[2].len = sizeof(end_frame);
  transfers[2].tx_buf = (unsigned long)end_frame;
  transfers[2].cs_change = 0;
}

void init_led_frames(led_frame_t * frames){
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    frames[i].brightness = 0xFF;
    frames[i].red = 0x00;
    frames[i].green = 0x00;
    frames[i].blue = 0x0F;
  }
}

void init_spi(uint32_t * pFd){
  int32_t err;
  int32_t mode = SPI_MODE_0;
  uint8_t bits = 8;

  *pFd = open("/dev/spidev1.0", O_RDWR);
  printf("fd %i\n", *pFd);

  err = ioctl(*pFd, SPI_IOC_WR_MODE32, &mode);
	if (err == -1)
    printf("can't set mode bits %i\n", err);

  err = ioctl(*pFd, SPI_IOC_RD_MODE32, &mode);
	if (err == -1)
    printf("can't set mode bits %i\n", err);

  err = ioctl(*pFd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (err == -1)
    printf("can't set num bits %i\n", err);

  err = ioctl(*pFd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (err == -1)
    printf("can't set num bits %i\n", err);
}
int32_t main(){

  int32_t fd;
  init_spi(&fd);
  init_transfers(transfers);
  init_led_frames(led_frames);

  bool board_one[BOARD_SIZE * BOARD_SIZE] = {false};
  bool board_two[BOARD_SIZE * BOARD_SIZE] = {false};

  bool * current_board = board_one;
  bool * next_board = board_two;
  bool * old_current_board;

  randomly_seed_board(current_board, BOARD_SIZE);

	uint32_t start = clock(), diff;

  for (;;) {
    calculate_next_board(current_board, next_board, BOARD_SIZE); 
    print_board(current_board, BOARD_SIZE, transfers, fd);

    old_current_board = current_board;
    current_board = next_board;
    next_board = old_current_board;
  }
	diff = clock() - start;

	int msec = diff * 1000 / CLOCKS_PER_SEC;
	printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);
  return 0;
}
