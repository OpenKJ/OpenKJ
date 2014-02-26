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


#ifndef CDG_FRAME_IMAGE_H
#define CDG_FRAME_IMAGE_H
#include "libCDG_Color.h"

class CDG_Frame_Image
{
public:
	CDG_Frame_Image()
	{
		memset(&CDG_Map, 0, sizeof(CDG_Map));
	}
	void SetCDGMapData(char inmap[216][300])
	{
		memcpy(&CDG_Map, &inmap, sizeof(CDG_Map));
	}
	void SetColor(int x, int y, int color)
	{
		CDG_Map[y][x] = color;
	}
	char GetCDG_Color(int x, int y)
	{
		return CDG_Map[y][x];
	}
	unsigned char *Get_RGB_Data();
	unsigned char CDG_Map[216][300];
	CDG_Color colors[16];

private:
	bool Skip;
	unsigned int LastUpdate;
};

#endif // CDG_FRAME_IMAGE_H
