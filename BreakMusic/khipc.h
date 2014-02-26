/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
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

#ifndef KHIPC_H
#define KHIPC_H

class KhIPC
{
public:
    KhIPC();
    enum{CMD_FADE_OUT=0,CMD_FADE_IN,CMD_PAUSE,CMD_STOP,CMD_PLAY,CMD_NOOP};

    int getCommand() const;
    void setCommand(int value);

private:
    int command;
};

#endif // KHIPC_H
