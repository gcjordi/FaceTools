/************************************************************************
 * Copyright (C) 2020 SIS Research Ltd & Richard Palmer
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

#ifndef FACE_TOOLS_ACTION_RESTORE_LANDMARKS_H
#define FACE_TOOLS_ACTION_RESTORE_LANDMARKS_H

#include "FaceAction.h"
#include <FaceTools/Widget/LandmarksCheckDialog.h>
#include <FaceTools/Interactor/LandmarksHandler.h>

namespace FaceTools { namespace Action {

class FaceTools_EXPORT ActionRestoreLandmarks : public FaceAction
{ Q_OBJECT
public:
    ActionRestoreLandmarks( const QString&, const QIcon&, Interactor::LandmarksHandler::Ptr);
    QString toolTip() const override { return "Restore landmarks to their initially detected positions.";}

    // Returns true iff given model has a mask and the landmarks were transferred.
    // Set ulmks specifies the ids of the landmarks to restore (function does nothing if empty).
    static bool restoreLandmarks( FM*, const IntSet& ulmks);

protected:
    void postInit() override;
    bool isAllowed( Event) override;
    bool doBeforeAction( Event) override;   // Warn if overwriting
    void doAction( Event) override;
    Event doAfterAction( Event) override;

private:
    Widget::LandmarksCheckDialog *_cdialog;
    Interactor::LandmarksHandler::Ptr _handler;
    IntSet _ulmks;
};  // end class

}}   // end namespace

#endif