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

#include "../include/libCDG.h"

#define UNUSED(x) (void)x

CDG::CDG()
{
    masks[0] = 0x20;
    masks[1] = 0x10;
    masks[2] = 0x08;
    masks[3] = 0x04;
    masks[4] = 0x02;
    masks[5] = 0x01;
    CDGImage = new CDG_Frame_Image();
    LastCDGCommandMS = 0;
    Open = false;
    CDGFileOpened = false;
    CDGFile = NULL;
    CurPos = 0;
    needupdate = true;
    NeedFullUpdate = true;
}

bool CDG::FileOpen(string filename)
{
    CDGFile = fopen(filename.c_str(), "rb");
    CurPos = 0;
    if (CDGFile != NULL)
    {
        CDGFileOpened = true;
    }
    else
    {
        CDGFileOpened = false;
    }
    needupdate = true;
    NeedFullUpdate = true;
    return CDGFileOpened;
}

void CDG::FileClose()
{
    fclose(CDGFile);
    CDGFileOpened = false;
}

void CDG::VideoClose()
{
    for(unsigned int i=0; i<CDGVideo.size(); i++)
    {
        if (!SkipFrames.at(i))
            delete(CDGVideo.at(i));
    }
    CDGVideo.clear();
    SkipFrames.clear();
    CurPos = 0;
    CDGFileOpened = false;
    Open = false;
}

unsigned int CDG::GetPosMS()
{
    float fpos = (CurPos / 300.0) * 1000;
    return (int) fpos;
}

bool CDG::SkipFrame(int ms)
{
    bool retval = false;
    unsigned int frame = ms / 40;
    if ((ms % 40 < 0) && (SkipFrames.size() > frame + 1) && (SkipFrames.at(frame + 1)))
        retval = true;
    if ((SkipFrames.at(frame)) && (SkipFrames.size() > frame))
        retval = true;
    return retval;
}

bool CDG::Process(bool clear)
{
    NeedFullUpdate = true;

    needupdate = true;
    CDG_SubCode SubCode;
    static int lastupdate;
    static int frame;
    if (clear)
    {
        lastupdate = 0;
        frame = 0;
    }
    while (CDGFileOpened)
    {
        if (!feof(CDGFile))
        {
            if (fread(&SubCode, sizeof(SubCode), 1, CDGFile) == 1)
            {
                CDG_Read_SubCode_Packet(SubCode);
                CurPos++;
                if (((GetPosMS() % 40) == 0) && (GetPosMS() >= 40))
                {
                    CDG_Frame_Image *img;;
                    if (needupdate)
                    {
                        LastCDGCommandMS = frame * 40;
                        lastupdate = frame;
                        needupdate = false;
                        img = new CDG_Frame_Image();
                        for (unsigned int i=0; i < ChangedRows.size(); i++)
                        {
                            img->ChangedRowAdd(ChangedRows.at(i));
                        }
                        img->NeedFullUpdate = NeedFullUpdate;
                        NeedFullUpdate = false;
                        ChangedRows.clear();
                        memcpy(&img->colors, &colors,sizeof(img->colors));
                        memcpy(&img->CDG_Map, &CDGImage->CDG_Map, sizeof(img->CDG_Map));
                        SkipFrames.push_back(false);
                    }
                    else
                    {
                        img = CDGVideo.at(lastupdate);
                        SkipFrames.push_back(true);
                    }
                    CDGVideo.push_back(img);
                    frame++;
                }
            }
            else
            {
                FileClose();
                Open = true;
                return true;
            }
        }
        else
        {
            FileClose();
            Open = true;
            return true;
        }
    }
    return false;
}


void CDG::CDG_Read_SubCode_Packet(CDG_SubCode &SubCode)
{
    if ((SubCode.command & SC_MASK) == SC_CDG_COMMAND)
    {
        switch (SubCode.instruction & SC_MASK)
        {
        case CDG_MEMORYPRESET:
            needupdate = true;
            NeedFullUpdate = true;
            CMDMemoryPreset(SubCode.data);
            break;
        case CDG_BORDERPRESET:
            needupdate = true;
            NeedFullUpdate = true;
            CMDBorderPreset(SubCode.data);
            break;
        case CDG_TILEBLOCK:
            needupdate = true;
            CMDTileBlock(SubCode.data, false);
            break;
        case CDG_SCROLLPRESET:
            needupdate = true;
            NeedFullUpdate = true;
            CMDScrollPreset(SubCode.data);
            break;
        case CDG_SCROLLCOPY:
            needupdate = true;
            NeedFullUpdate = true;
            CMDScrollCopy(SubCode.data);
            break;
        case CDG_DEFINETRANS:
            needupdate = true;
            NeedFullUpdate = true;
            CMDDefineTrans(SubCode.data);
            break;
        case CDG_COLORSLOW:
            needupdate = true;
            CMDColors(SubCode.data, CDG_COLOR_TABLE_LOW);
            break;
        case CDG_COLORSHIGH:
            needupdate = true;
            CMDColors(SubCode.data, CDG_COLOR_TABLE_HIGH);
            break;
        case CDG_TILEBLOCKXOR:
            needupdate = true;
            CMDTileBlock(SubCode.data, true);
            break;
        }
    }
}

void CDG::CMDBorderPreset(char data[16])
{
    CDG_Border_Preset preset;
    preset.color = (data[0] & 0x0F);
    if (preset.color <= 15)
    {
        // set first 12 rows to preset color
        memset(&CDGImage->CDG_Map, preset.color, 3600);
        // set last 12 rows to preset color
        memset(&CDGImage->CDG_Map + 204, preset.color, 3600);
        for (unsigned int i = 12; i < 204; i++)
        {
            // set first 6 cols in row to preset color
            memset(&CDGImage->CDG_Map[i], preset.color, 6);
            // want to use memset for the end 6 cols as well, but it's choking on it for some reason
            // just going to use direct setting for now, will try to figure it out later.
            CDGImage->CDG_Map[i][294] = preset.color;
            CDGImage->CDG_Map[i][295] = preset.color;
            CDGImage->CDG_Map[i][296] = preset.color;
            CDGImage->CDG_Map[i][297] = preset.color;
            CDGImage->CDG_Map[i][298] = preset.color;
            CDGImage->CDG_Map[i][299] = preset.color;
        }
    }
}

void CDG::CMDColors(char data[16], int Table)
{
    static CDG_Color original[16];
    for (unsigned int i=0; i < 16; i++)
    {
        original[i] = colors[i];
    }
    int i;
    int j;
    char highbyte;
    char lowbyte;
    int red;
    int green;
    int blue;
    i = 0;
    if (Table == CDG_COLOR_TABLE_HIGH) j = 8;
    else j = 0;
    while (i < 15)
    {
        lowbyte = data[i];
        highbyte = data[i + 1];
        red = 0;
        green = 0;
        blue = 0;
        if ((lowbyte & 0x20) > 0)
        {
            red = 8;
        }
        if ((lowbyte & 0x10) > 0)
        {
            red = red + 4;
        }
        if ((lowbyte & 0x08) > 0)
        {
            red = red + 2;
        }
        if ((lowbyte & 0x04) > 0)
        {
            red = red + 1;
        }
        if ((lowbyte & 0x02) > 0)
        {
            green = 8;
        }
        if ((lowbyte & 0x01) > 0)
        {
            green = green + 4;
        }
        if ((highbyte & 0x20) > 0)
        {
            green = green + 2;
        }
        if ((highbyte & 0x10) > 0)
        {
            green = green + 1;
        }
        if ((highbyte & 0x08) > 0)
        {
            blue = 8;
        }
        if ((highbyte & 0x04) > 0)
        {
            blue = blue + 4;
        }
        if ((highbyte & 0x02) > 0)
        {
            blue = blue + 2;
        }
        if ((highbyte & 0x01) > 0)
        {
            blue = blue + 1;
        }
        blue  = blue  * 17;
        red   = red   * 17;
        green = green * 17;
        i++;
        i++;
        colors[j].SetRGB(red, green, blue);
        j++;
    }
    bool change = false;
    for (unsigned int i=0; i < 16; i++)
    {
        if (colors[i] != original[i]) change = true;
    }
    if (change) NeedFullUpdate = true;
}

void CDG::CMDDefineTrans(char data[16])
{
    UNUSED(data);
    // Unused CDG command from red book spec
}

void CDG::CMDMemoryPreset(char data[16])
{
    CDG_Memory_Preset preset;
    preset.color = (data[0] & 0x0F);
    preset.repeat = (data[1] & 0x0F);
    if (preset.color <= 15)
    {
        memset(&CDGImage->CDG_Map, preset.color, sizeof(CDGImage->CDG_Map));
    }
}

void CDG::CMDScrollCopy(char data[16])
{
    UNUSED(data);
}

void CDG::CMDScrollPreset(char data[16])
{
    UNUSED(data);
}

void CDG::CMDTileBlock(char data[16], bool XOR)
{
    CDG_Tile_Block tile;
    int top, left, i, j;
    tile.color0 = (data[0] & 0x0F);
    tile.color1 = (data[1] & 0x0F);
    tile.row = (data[2] & 0x1F);
    tile.column = (data[3] & 0x3F);
    tile.tilePixels[0]  = data[4];
    tile.tilePixels[1]  = data[5];
    tile.tilePixels[2]  = data[6];
    tile.tilePixels[3]  = data[7];
    tile.tilePixels[4]  = data[8];
    tile.tilePixels[5]  = data[9];
    tile.tilePixels[6]  = data[10];
    tile.tilePixels[7]  = data[11];
    tile.tilePixels[8]  = data[12];
    tile.tilePixels[9]  = data[13];
    tile.tilePixels[10] = data[14];
    tile.tilePixels[11] = data[15];
    ChangedRows.push_back(tile.row);
    top  = (tile.row    * 12);
    left = (tile.column * 6);
    for (i = 0; i <= 11; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            if (((top + i) <= 215) && ((left + j) <= 299) && (tile.color0 <= 15) && (tile.color1 <= 15))
            {
                if ((tile.tilePixels[i] & masks[j]) > 0)
                {
                    int coloridx;
                    if (XOR) coloridx = CDGImage->GetCDG_Color(left + j, top + i) ^ tile.color1;
                    else     coloridx = tile.color1;
                    CDGImage->SetColor(left + j, top + i, coloridx);
                }
                else
                {
                    int coloridx;
                    if (XOR) coloridx = CDGImage->GetCDG_Color(left + j, top + i) ^ tile.color0;
                    else     coloridx = tile.color0;
                    CDGImage->SetColor(left + j, top + i, coloridx);
                }
            }
        }
    }
}

CDG::~CDG()
{
    delete CDGImage;
}



unsigned char *CDG::GetImageByTime(unsigned int ms)
{
    unsigned int frameno = ms / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno < CDGVideo.size())
        return CDGVideo.at(frameno)->Get_RGB_Data();
    else
    {
        return CDGVideo.at(CDGVideo.size() -1)->Get_RGB_Data();
    }
}

void CDG::GetImageByTime(unsigned int ms, unsigned char * pRGB)
{
    unsigned int frameno = ms / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno < CDGVideo.size())
        CDGVideo.at(frameno)->Get_RGB_Data(pRGB);
    else
    {
        CDGVideo.at(CDGVideo.size() -1)->Get_RGB_Data(pRGB);
    }
}
unsigned char *CDG::GetCDGRowByTime(unsigned int ms, unsigned int row)
{
    unsigned int frameno = ms / 40;
    if (ms % 40 > 0) frameno++;
    if (frameno < CDGVideo.size())
        return CDGVideo.at(frameno)->Get_Row_Data(row);
    else
    {
        return CDGVideo.at(CDGVideo.size() -1)->Get_Row_Data(row);
    }


}

bool CDG::RowNeedsUpdate(unsigned int ms, int row)
{
    unsigned int frameno = ms / 40;
    if ((ms % 40 > 0) && (frameno + 1 < CDGVideo.size()) && (CDGVideo.at(frameno + 1)->RowChanged(row)))
        return true;
    if ((frameno < CDGVideo.size()) && (CDGVideo.at(frameno)->RowChanged(row)))
        return true;
    return false;
}
bool CDG::AllNeedUpdate(unsigned int ms)
{

    unsigned int frameno = ms / 40;
    if (frameno < CDGVideo.size())
    return CDGVideo.at(frameno)->NeedFullUpdate;
    else return false;
}
