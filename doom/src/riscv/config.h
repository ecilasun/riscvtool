/*
 * config.h
 *
 * Copyright (C) 2021 Sylvain Munaut
 * All rights reserved.
 *
 * LGPL v3+, see LICENSE.lgpl3
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

// Modified for NekoIchi by Engin Cilasun

#pragma once

#include <inttypes.h>

extern volatile unsigned int *IO_AudioOutput;
extern volatile unsigned int *IO_SwitchByteCount;
extern volatile unsigned char *IO_SwitchState;
extern volatile unsigned char *IO_SPIOutput;
extern volatile unsigned char *IO_SPIInput;
extern volatile unsigned char *IO_UARTTX;
extern volatile unsigned char *IO_UARTRX;
extern volatile unsigned int *IO_UARTRXByteCount;
extern volatile unsigned int *IO_GPUFIFO;

uint64_t ReadClock();
uint64_t ClockToMs(uint64_t clock);
uint64_t ClockToUs(uint64_t clock);