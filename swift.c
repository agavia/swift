
/* Project Swift - High altitude balloon flight software                 */
/*=======================================================================*/
/* Copyright 2010-2012 Philip Heron <phil@sanslogic.co.uk>               */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/crc16.h>
#include "rtty.h"
#include "gps.h"
#include "geofence.h"

#define LEDBIT(b) PORTB = (PORTB & (~_BV(7))) | ((b) ? _BV(7) : 0)

uint16_t crccat(char *msg)
{
	uint16_t x;
	for(x = 0xFFFF; *msg; msg++)
		x = _crc_xmodem_update(x, *msg);
	snprintf(msg, 8, "*%04X\n", x);
	return(x);
}

int main(void)
{
	uint32_t count = 0;
	int32_t lat, lon, alt;
	uint8_t hour, minute, second;
	char msg[100];
	
	/* Set the LED pin for output */
	DDRB |= _BV(DDB7);
	
	rtx_init();
	gps_setup();
	
	sei();
	
	/* Enable the radio and let it settle */
	rtx_enable(1);
	_delay_ms(1000);
	
	rtx_string_P(PSTR(RTTY_CALLSIGN " starting up"));
	
	while(1)
	{
		if(!gps_get_pos(&lat, &lon, &alt))
		{
			rtx_string_P(PSTR("$$" RTTY_CALLSIGN ",No or invalid GPS response\n"));
			continue;
		}
		
		if(!gps_get_time(&hour, &minute, &second))
		{
			rtx_string_P(PSTR("$$" RTTY_CALLSIGN ",No or invalid GPS response\n"));
			continue;
		}

		if(geofence_test(lat, lon))
		{
			rtx_string_P(PSTR("$$" RTTY_CALLSIGN ",APRS GEOFENCE ACTIVE\n"));
		}

		else
		{
			rtx_string_P(PSTR("$$" RTTY_CALLSIGN ",OUTSIDE UK AIRSPACE, APRS GEOFENCE DEACTIVED\n"));

		}
		
		rtx_wait();
		
		snprintf(msg, 100, "$$%s,%li,%02i:%02i:%02i,%s%li.%05li,%s%li.%05li,%li",
			RTTY_CALLSIGN, count++,
			hour, minute, second,
			(lat < 0 ? "-" : ""), labs(lat) / 10000000, labs(lat) % 10000000 / 100,
			(lon < 0 ? "-" : ""), labs(lon) / 10000000, labs(lon) % 10000000 / 100,
			alt / 1000);
		crccat(msg + 2);
		rtx_string(msg);
	}
}

