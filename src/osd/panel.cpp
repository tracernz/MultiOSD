/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "panel.h"
#include "../lib/max7456/max7456.h"
#include "../telemetry/telemetry.h"
#include <math.h>
#include <string.h>

namespace osd
{

namespace draw
{

	/*
	 * 012
	 * 3 7
	 * 456
	 */
	const uint8_t _rect_thin [] PROGMEM = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7};
	const uint8_t _rect_fill [] PROGMEM = {0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};

	void rect (uint8_t l, uint8_t t, uint8_t w, uint8_t h, bool filled, uint8_t attr)
	{
		if (w < 2 || h < 2) return;

		uint8_t r = w - 1;
		uint8_t b = h - 1;

		char _rect [8];
		memcpy_P (_rect, filled ? _rect_fill : _rect_thin, 8);
		char buffer [w + 1];

		for (uint8_t i = 0; i < h; i ++)
		{
			char spacer;
			if (i == 0)
			{
				buffer [0] = _rect [0];
				buffer [r] = _rect [2];
				spacer = _rect [1];
			}
			else if (i == b)
			{
				buffer [0] = _rect [4];
				buffer [r] = _rect [6];
				spacer = _rect [5];
			}
			else
			{
				buffer [0] = _rect [3];
				buffer [r] = _rect [7];
				spacer = ' ';
			}
			memset (buffer + 1, spacer, w - 2);
			buffer [w] = 0;
			max7456::puts (l, t + i, buffer, attr);
		}
	}

}  // namespace draw


namespace __panels
{

#define STD_DRAW void draw (uint8_t x, uint8_t y) \
{ \
	max7456::puts (x, y, buffer); \
}

#define terminate_buffer() { buffer [sizeof (buffer) - 1] = 0; }


namespace alt
{

	const char __name [] PROGMEM = "StableAlt";

	char buffer [8];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\x85%d\x8d"), (int16_t) telemetry::stable::altitude);
		terminate_buffer ();
	}

	STD_DRAW;

}  // namespace alt

namespace climb
{

	const char __name [] PROGMEM = "Climb";

	char buffer [8];

	void update ()
	{
		sprintf_P (buffer, PSTR ("%c%.1f\x8c"),
			telemetry::stable::climb < 0 ? 0x07 : 0x08, telemetry::stable::climb);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace climb_rate

namespace flight_mode
{

	const char __name [] PROGMEM = "FlightMode";

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		osd::draw::rect (x, y, 6, 3);
		max7456::puts_p (x + 1, y + 1, telemetry::status::flight_mode_name_p ? telemetry::status::flight_mode_name_p : PSTR ("\x09\x09\x09\x09"));
	}

}  // namespace flight_mode

namespace arming_state
{

	const char __name [] PROGMEM = "ArmState";

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		uint8_t attr = telemetry::status::armed ? 0 : MAX7456_ATTR_INVERT;
		osd::draw::rect (x, y, 3, 3, true, attr);
		max7456::put (x + 1, y + 1, 0xe0, attr);
	}

}  // namespace name

namespace connection_state
{

	const char __name [] PROGMEM = "ConState";

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		uint8_t attr = telemetry::status::connection != CONNECTION_STATE_CONNECTED
			? MAX7456_ATTR_INVERT
			: 0;

		osd::draw::rect (x, y, 3, 3, true, attr);
		max7456::put (x + 1, y + 1, 0xe1, attr);
	}

}  // namespace connection_state

namespace flight_time
{

	const char __name [] PROGMEM = "FlightTime";

	char buffer [8];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xb3%02u:%02u"), telemetry::status::flight_time / 60, telemetry::status::flight_time % 60);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace flight_time

namespace roll
{

	const char __name [] PROGMEM = "Roll";

	char buffer [7];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xb2%d\xb0"), (int16_t) telemetry::attitude::roll);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace roll

namespace pitch
{

	const char __name [] PROGMEM = "Pitch";

	char buffer [7];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xb1%d\xb0"), (int16_t) telemetry::attitude::pitch);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace pitch

namespace gps_state
{

	const char __name [] PROGMEM = "GPS";

#define _PAN_GPS_2D 0x01
#define _PAN_GPS_3D 0x02

	char buffer [4];

	void update ()
	{
		sprintf_P (buffer, PSTR ("%d"), telemetry::gps::satellites);
		terminate_buffer ();
	}

	void draw (uint8_t x, uint8_t y)
	{
		bool err = telemetry::gps::state == GPS_STATE_NO_FIX;
		max7456::puts_p (x, y, PSTR ("\x10\x11"), err ? MAX7456_ATTR_BLINK : 0);
		max7456::put (x + 2, y, telemetry::gps::state <  GPS_STATE_3D ? _PAN_GPS_2D : _PAN_GPS_3D,
			telemetry::gps::state < GPS_STATE_2D ? MAX7456_ATTR_BLINK : 0);
		if (err) max7456::puts_p (x + 3, y, PSTR ("ERR"), MAX7456_ATTR_BLINK);
		else max7456::puts (x + 3, y, buffer);
	}

}  // namespace gps_state

namespace gps_lat
{

	const char __name [] PROGMEM = "Lat";

	char buffer [11];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\x83%02.6f"), telemetry::gps::latitude);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace gps_lat

namespace gps_lon
{

	const char __name [] PROGMEM = "Lon";

	char buffer [11];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\x84%02.6f"), telemetry::gps::longitude);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace gps_lon

namespace horizon
{

#define PANEL_HORIZON_WIDTH 14
#define PANEL_HORIZON_HEIGHT 5

#define PANEL_HORIZON_LEFT_BORDER 0xb8
#define PANEL_HORIZON_LEFT_CENTER 0xc8
#define PANEL_HORIZON_RIGHT_BORDER 0xb9
#define PANEL_HORIZON_RIGHT_CENTER 0xc9
#define PANEL_HORIZON_LINE 0x16
#define PANEL_HORIZON_TOP 0x0e

#define _PAN_HORZ_CHAR_LINES 18
#define _PAN_HORZ_VRES 9
#define _PAN_HORZ_INT_WIDTH (PANEL_HORIZON_WIDTH - 2)
#define _PAN_HORZ_LINES (PANEL_HORIZON_HEIGHT * _PAN_HORZ_VRES)
#define _PAN_HORZ_TOTAL_LINES (PANEL_HORIZON_HEIGHT * _PAN_HORZ_CHAR_LINES)

#define _RADIAN 0.017453293

	const char __name [] PROGMEM = "Horizon";

	const char _line [PANEL_HORIZON_WIDTH + 1] PROGMEM = "\xb8            \xb9";
	const char _center [PANEL_HORIZON_WIDTH + 1] PROGMEM = "\xc8            \xc9";
	char buffer [PANEL_HORIZON_HEIGHT][PANEL_HORIZON_WIDTH + 1];

	void update ()
	{
		for (uint8_t i = 0; i < PANEL_HORIZON_HEIGHT; i ++)
		{
			memcpy_P (buffer [i], i == PANEL_HORIZON_HEIGHT / 2 ? _center : _line, PANEL_HORIZON_WIDTH);
			buffer [i][PANEL_HORIZON_WIDTH] = 0;
		}

		// code below was taken from minoposd
		int16_t pitch_line = tan (-_RADIAN * telemetry::attitude::pitch) * _PAN_HORZ_LINES;
		float roll = tan (_RADIAN * telemetry::attitude::roll);
		for (uint8_t col = 1; col <= _PAN_HORZ_INT_WIDTH; col ++)
		{
			// center X point at middle of each column
			int16_t middle = col * _PAN_HORZ_INT_WIDTH - (_PAN_HORZ_INT_WIDTH * _PAN_HORZ_INT_WIDTH / 2) - _PAN_HORZ_INT_WIDTH / 2;
			// calculating hit point on Y plus offset to eliminate negative values
			int8_t hit = roll * middle + pitch_line + _PAN_HORZ_LINES;
			if (hit > 0 && hit < _PAN_HORZ_TOTAL_LINES)
			{
				int8_t row = PANEL_HORIZON_HEIGHT - ((hit - 1) / _PAN_HORZ_CHAR_LINES);
				int8_t subval = (hit - (_PAN_HORZ_TOTAL_LINES - row * _PAN_HORZ_CHAR_LINES + 1)) * _PAN_HORZ_VRES / _PAN_HORZ_CHAR_LINES;
				if (subval == _PAN_HORZ_VRES - 1)
					buffer [row - 2][col] = PANEL_HORIZON_TOP;
				buffer [row - 1][col] = PANEL_HORIZON_LINE + subval;
			}
		}
	}

	void draw (uint8_t x, uint8_t y)
	{
		for (uint8_t i = 0; i < PANEL_HORIZON_HEIGHT; i ++)
			max7456::puts (x, y + i, buffer [i]);
	}

}  // namespace horizon

namespace throttle
{

	const char __name [] PROGMEM = "Throttle";

	char buffer [7];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\x87%d%%"), telemetry::input::throttle);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace throttle

namespace ground_speed
{

	const char __name [] PROGMEM = "GroundSpeed";

	char buffer [7];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\x0a%d\x81"), (int16_t) (telemetry::stable::ground_speed * 3.6));
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace ground_speed

namespace battery_voltage
{

	const char __name [] PROGMEM = "BatVoltage";

	char buffer [7];
	char _symbol;
	uint8_t _attr;

	void update ()
	{
		sprintf_P (buffer, PSTR ("%.2f\x8e"), telemetry::battery::voltage);
		terminate_buffer ();

		_symbol = 0xf4 + (uint8_t) round (telemetry::battery::level / 20.0);
		_attr = telemetry::messages::battery_low ? MAX7456_ATTR_BLINK : 0;
	}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::put (x, y, _symbol, _attr);
		max7456::puts (x + 1, y, buffer);
	}

}  // namespace battery_voltage

namespace battery_current
{

	const char __name [] PROGMEM = "BatCurrent";

	char buffer [8];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xfa%.2f\x8f"), telemetry::battery::current);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace battery_current

namespace battery_consumed
{

	const char __name [] PROGMEM = "BatConsumed";

	char buffer [8];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xfb%u\x82"), (uint16_t) telemetry::battery::consumed);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace battery_consumed

namespace rssi_flag
{

	const char __name [] PROGMEM = "RSSIFlag";

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		if (telemetry::messages::rssi_low) max7456::put (x, y, 0xb4, MAX7456_ATTR_BLINK);
	}

}  // namespace rssi_flag

namespace home_distance
{

	const char __name [] PROGMEM = "HomeDistance";

	char buffer [8];
	uint8_t _attr, _i_attr;

	void update ()
	{
		_attr = telemetry::home::state == HOME_STATE_NO_FIX ? MAX7456_ATTR_BLINK : 0;
		_i_attr = telemetry::home::state != HOME_STATE_FIXED ? MAX7456_ATTR_BLINK : 0;
		if (_i_attr)
		{
			sprintf_P (buffer, PSTR ("%S"), telemetry::home::state == HOME_STATE_NO_FIX ? PSTR ("ERR") : PSTR ("\x09\x09\x09\x8d"));
			return;
		}
		if (telemetry::home::distance >= 10000)
			 sprintf_P (buffer, PSTR ("%.1f\x8b"), telemetry::home::distance / 1000);
		else sprintf_P (buffer, PSTR ("%u\x8d"), (uint16_t) telemetry::home::distance);
	}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::put (x, y, 0x12, _i_attr);
		max7456::puts (x + 1, y, buffer, _attr);
	}

}  // namespace home_distance

namespace home_direction
{

#define _PAN_HD_ARROWS 0x90

	const char __name [] PROGMEM = "HomeDirection";

	uint8_t _arrow;

	void update ()
	{
		_arrow = _PAN_HD_ARROWS + telemetry::home::direction * 2;
	}

	void draw (uint8_t x, uint8_t y)
	{
		if (telemetry::home::state != HOME_STATE_FIXED) return;
		max7456::put (x, y, _arrow);
		max7456::put (x + 1, y, _arrow + 1);
	}

}  // namespace home_direction

namespace callsign
{

	const char __name [] PROGMEM = "CallSign";

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::puts (x, y, telemetry::status::callsign);
	}

}  // namespace callsign

namespace temperature
{

	const char __name [] PROGMEM = "Temperature";

	char buffer [6];

	void update ()
	{
		sprintf_P (buffer, PSTR ("\xfd%d\xb0"), telemetry::stable::temperature);
		terminate_buffer ();
	}

	STD_DRAW

}  // namespace temperature


namespace rssi
{

	const char __name [] PROGMEM = "RSSI";

	const char _l0 [] PROGMEM = "\xe5\xe8\xe8";
	const char _l1 [] PROGMEM = "\xe2\xe8\xe8";
	const char _l2 [] PROGMEM = "\xe2\xe6\xe8";
	const char _l3 [] PROGMEM = "\xe2\xe3\xe8";
	const char _l4 [] PROGMEM = "\xe2\xe3\xe7";
	const char _l5 [] PROGMEM = "\xe2\xe3\xe4";

	const char * const levels [] PROGMEM = { _l0, _l1, _l2, _l3, _l4, _l5 };

	const char *_scale = NULL;

	void update ()
	{
		uint8_t level = round (telemetry::input::rssi / 20.0);
		if (level == 0 && telemetry::input::rssi > 0) level = 1;
		if (level > 5) level = 5;
		_scale = (const char *) pgm_read_ptr (&levels [level]);
	}

	void draw (uint8_t x, uint8_t y)
	{

		max7456::puts_p (x, y, _scale);
	}

}  // namespace rssi

}  // namespace __panels

namespace panel
{

#define _declare_panel(NS) { osd::__panels:: NS ::__name, osd::__panels:: NS ::update, osd::__panels:: NS ::draw }

const panel_t panels [] PROGMEM = {
	_declare_panel (alt),
	_declare_panel (climb),
	_declare_panel (flight_mode),
	_declare_panel (arming_state),
	_declare_panel (connection_state),
	_declare_panel (flight_time),
	_declare_panel (roll),
	_declare_panel (pitch),
	_declare_panel (gps_state),
	_declare_panel (gps_lat),
	_declare_panel (gps_lon),
	_declare_panel (horizon),
	_declare_panel (throttle),
	_declare_panel (ground_speed),
	_declare_panel (battery_voltage),
	_declare_panel (battery_current),
	_declare_panel (battery_consumed),
	_declare_panel (rssi_flag),
	_declare_panel (home_distance),
	_declare_panel (home_direction),
	_declare_panel (callsign),
	_declare_panel (temperature),
	_declare_panel (rssi),
};

const uint8_t count = sizeof (panels) / sizeof (panel_t);

}  // namespace panels

}  // namespace osd
