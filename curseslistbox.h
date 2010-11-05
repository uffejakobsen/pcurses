/* *************************************************************************
 *  Copyright 2010 Jakob Gruber                                            *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ************************************************************************* */

#ifndef CURSESLISTBOX_H
#define CURSESLISTBOX_H

#include <vector>

#include "cursesframe.h"
#include "package.h"

using std::vector;

class CursesListBox : public CursesFrame
{
public:
    CursesListBox(int w, int h, int x, int y, bool hasborder);

    void SetList(vector<Package*> *l);
    void Move(int step);
    void MoveAbs(int pos);
    int FocusedIndex() const;
    virtual void Refresh();

protected:

    bool IsInBounds(int pos) const;
    void UpdateFocus();

    std::vector<Package*>
            *list;
    int
            windowpos,
            cursorpos;
};

#endif // CURSESLISTBOX_H