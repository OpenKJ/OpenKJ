/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
 *
 *
 * This file is part of libCDG.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
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

#ifndef LIBCDG_COLOR_H
#define LIBCDG_COLOR_H

#include <string.h>

class CDG_Color
{
public:
	CDG_Color()
	{
		memset(&rgb, 0, 3);
		IndexNo = 0;
		// cout << "Color() constructor called" << endl;
	}
	CDG_Color(int red, int green, int blue, int indexno = 0)
	{
		rgb[0] = red;
		rgb[1] = green;
		rgb[2] = blue;
		IndexNo = indexno;
	}
	bool operator== (CDG_Color incolor)
	{
		if ((rgb[0] == incolor.rgb[0]) && (rgb[1] == incolor.rgb[1]) && (rgb[2] == incolor.rgb[2])) return true;
		else return false;
	}
	bool operator!= (CDG_Color incolor)
	{
		if ((rgb[0] == incolor.rgb[0]) && (rgb[1] == incolor.rgb[1]) && (rgb[2] == incolor.rgb[2])) return false;
		else return true;
	}
	void operator= (CDG_Color incolor)
	{
		memcpy(&rgb, &incolor.rgb, 3);
		IndexNo = incolor.IndexNo;
	}
	void SetRGB(int red, int green, int blue)
	{
		rgb[0] = red;
		rgb[1] = green;
		rgb[2] = blue;
	}
	void SetIndex(int indexno)
	{
		IndexNo = indexno;
	}
	int GetIndex()
	{
		return IndexNo;
	}
	int GetRed()
	{
		return rgb[0];
	}
	int GetGreen()
	{
		return rgb[1];
	}
	int GetBlue()
	{
		return rgb[2];
	}
	int IndexNo;
	unsigned char rgb[3];

private:

};

#endif // LIBCDG_COLOR_H
