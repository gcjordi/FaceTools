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

#ifndef FACE_TOOLS_INTERACTOR_MOVEMENT_NOTIFIER_H
#define FACE_TOOLS_INTERACTOR_MOVEMENT_NOTIFIER_H

#include "ViewerNotifier.h"

namespace FaceTools { namespace Interactor {

class FaceTools_EXPORT MovementNotifier : public QObject, public ViewerNotifier
{ Q_OBJECT
signals:
    void onEvent( Action::Event);   // Event::CAMERA_CHANGE or Event::ACTOR_MOVE

private:
    void cameraStart() override;
    void actorStart( const vtkProp3D*) override;
};  // end class

}}   // end namespace

#endif
