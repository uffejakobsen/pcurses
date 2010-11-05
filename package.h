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

#ifndef PACKAGE_H
#define PACKAGE_H

#include <alpm.h>
#include <string>
#include <ctime>

typedef struct __pmpkg_t pmpkg_t;

using std::string;

enum InstallReasonEnum {
    IRE_EXPLICIT,
    IRE_ASDEPS,
    IRE_NOTINSTALLED
};

enum OperationEnum {
    OE_INSTALL_EXPLICIT,
    OE_INSTALL_ASDEPS,
    OE_UPGRADE,
    OE_REMOVE
};

class Package
{
public:
    Package(pmpkg_t *pkg);

    string name() const;
    string url() const;
    string packager() const;
    string desc() const;
    string version() const;
    string dbname() const;
    InstallReasonEnum reason() const;
    string reasonstring() const;
    string builddate() const;
    bool installed() const;
    bool needsupdate() const;

    void setop(OperationEnum oe);
    OperationEnum getop() const;

    static bool cmp(const Package *lhs, const Package *rhs);
    static bool matches(const Package *a, const std::string needle, const std::string op);

private:

    pmpkg_t
            *_pkg,
            *_localpkg;

    OperationEnum
            op;
};

#endif // PACKAGE_H