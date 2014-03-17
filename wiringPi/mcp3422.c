/*
 * mcp3422.c:
 *	Extend wiringPi with the MCP3422 I2C ADC chip
 *	Also works for the MCP3423 and MCP3224 (4 channel) chips
 *	Copyright (c) 2013 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "mcp3422.h"


/*
 * myAnalogRead:
 *	Read a channel from the device
 *********************************************************************************
 */

static int read18Bits (struct wiringPiNodeStruct *node)
{
  unsigned char buffer [4] ;
  int value = 0 ;
  read (node->fd, buffer, 4) ;
  while (buffer[3] & 128)
  {
    read (node->fd, buffer, 4) ;
  }
  value = ((buffer [0] & 0x1) << 16) | (buffer [1] << 8) | buffer [2] ;
  if (buffer [0] & 0x2)
  {
    value = ~(0x20000 - value) + 1 ;
  }
  return value;
}

static int read16Bits (struct wiringPiNodeStruct *node)
{
  unsigned char buffer [3] ;
  int value = 0 ;
  read (node->fd, buffer, 3) ;
  while (buffer [2] & 128)
  {
    read (node->fd, buffer, 3) ;
  }
  value = ((buffer [0] & 0x7F) << 8) | buffer [1] ;
  if (buffer [0] & 0x80)
  {
    value = ~(0x8000 - value) + 1 ;
  }
  return value;
}

static int read14Bits (struct wiringPiNodeStruct *node)
{
  unsigned char buffer [3] ;
  int value = 0 ;
  read (node->fd, buffer, 3) ;
  while (buffer [2] & 128)
  {
    read (node->fd, buffer, 3) ;
  }
  value = ((buffer [0] & 0x1F) << 8) | buffer [1] ;
  if (buffer [0] & 0x20)
  {
    value = ~(0x2000 - value) + 1 ;
  }
  return value;
}

static int read12Bits (struct wiringPiNodeStruct *node)
{
  unsigned char buffer [3] ;
  int value = 0 ;
  read (node->fd, buffer, 3) ;
  while (buffer [2] & 128)
  {
    read (node->fd, buffer, 3) ;
  }
  value = ((buffer [0] & 0x07) << 8) | buffer [1] ;
  if (buffer [0] & 0x8)
  {
    value = ~(0x800 - value) + 1 ;
  }
  return value;
}

int myAnalogRead (struct wiringPiNodeStruct *node, int chan)
{
  unsigned char config ;
  unsigned char buffer [4] ;
  int value = 0 ;

// One-shot mode, trigger plus the other configs.

  config = 0x80 | ((chan - node->pinBase) << 5) | (node->data0 << 2) | (node->data1) ;
  
  wiringPiI2CWrite (node->fd, config) ;
  
  switch (node->data0)	// Sample rate
  {
    case MCP3422_SR_3_75:			// 18 bits
      value = read18Bits(node);
      break ;

    case MCP3422_SR_15:				// 16 bits
      value = read16Bits(node);
      break ;

    case MCP3422_SR_60:				// 14 bits
      value = read14Bits(node);
      break ;

    case MCP3422_SR_240:			// 12 bits
      value = read12Bits(node);
      break ;
  }

  return value ;
}


/*
 * mcp3422Setup:
 *	Create a new wiringPi device node for the mcp3422
 *********************************************************************************
 */

int mcp3422Setup (int pinBase, int i2cAddress, int sampleRate, int gain)
{
  int fd ;
  struct wiringPiNodeStruct *node ;

  if ((fd = wiringPiI2CSetup (i2cAddress)) < 0)
    return fd ;

  node = wiringPiNewNode (pinBase, 4) ;

  node->fd         = fd ;
  node->data0      = sampleRate ;
  node->data1      = gain ;
  node->analogRead = myAnalogRead ;

  return 0 ;
}
