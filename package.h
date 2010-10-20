/***************************************************************************

    Copyright 2010 Jakob Gruber

    This file is part of dungeons.

    dungeons is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    dungeons is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with dungeons.  If not, see <http://www.gnu.org/licenses/>.

 ***************************************************************************/

#ifndef PACKAGE_H
#define PACKAGE_H

#include <alpm.h>
#include <string>
#include <ctime>

typedef struct __pmpkg_t pmpkg_t;

using std::string;

class Package
{
public:
    Package(pmpkg_t *pkg);

    string name() const;
    string desc() const;
    string version() const;
    string dbname() const;
    _pmpkgreason_t reason() const;
    string builddate() const;
    bool installed() const;
    bool needsupdate() const;

private:

    pmpkg_t
            *_pkg,
            *_localpkg;
};

#endif // PACKAGE_H
