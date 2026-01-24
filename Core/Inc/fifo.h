/*
 * FIFO header
 * Written by Robert Blatner
 *
 */
#ifndef FIFO
#define FIFO

#include <string.h>

// FIFO definitions
#define FIFO_SIZE 32

// FIFO variables
static uint8_t fifo_buffer[FIFO_SIZE];
static uint8_t fifo_read;
static uint8_t fifo_write;

// FIFO function definitions
void fifo_init()
{
  memset(&fifo_buffer[0], 0, FIFO_SIZE);
  fifo_read = 0;
  fifo_write = 0;
}

#endif // FIFO
