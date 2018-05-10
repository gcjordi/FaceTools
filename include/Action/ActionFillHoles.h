/************************************************************************
 * Copyright (C) 2017 Richard Palmer
 *
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
 ************************************************************************/

#ifndef FACE_TOOLS_ACTION_FILL_HOLES_H
#define FACE_TOOLS_ACTION_FILL_HOLES_H

#include "FaceAction.h"

namespace FaceTools {
namespace Action {

class FaceTools_EXPORT ActionFillHoles : public FaceAction
{ Q_OBJECT
public:
    explicit ActionFillHoles( QProgressBar* pb=NULL);   // Async if pb not NULL

    QString getDisplayName() const override { return "Fill Holes";}
    const QIcon* getIcon() const override { return &_icon;}

protected slots:
    bool testReady( FaceControl* fc) override;
    bool testEnabled() const { return readyCount() == 1;}   // Only enabled for single selections
    bool doAction( FaceControlSet&) override;

private:
    QIcon _icon;
};  // end class

}   // end namespace
}   // end namespace

#endif