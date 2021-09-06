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

extern volatile unsigned char *IO_SPIRXTX;
extern volatile unsigned char *IO_UARTRXTX;
extern volatile unsigned int *IO_UARTRXByteCount;
extern volatile unsigned int *IO_UARTTXFifoFull;
extern volatile unsigned int *PRAMStart;
extern volatile unsigned int *GRAMStart;
extern volatile unsigned int *SRAMStart;

uint64_t ReadClock();
uint64_t ClockToMs(uint64_t clock);
uint64_t ClockToUs(uint64_t clock);