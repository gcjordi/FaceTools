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

#ifndef FACE_TOOLS_MODEL_VIEWER_INTERACTOR_H
#define FACE_TOOLS_MODEL_VIEWER_INTERACTOR_H

/**
 * Provides an interface to a model viewer via VTK events on the underlying viewer.
 * Multiple ModelViewerInteractor instances can be active on a single viewer at once,
 * but a single ModelViewerInteractor can only be attached to one viewer.
 * This interface attaches itself to a viewer using ModelViewer::attachInterface( &MVI)
 * and detaches itself on destruction with ModelViewer::detachInterface( &MVI).
 */

#include "FaceTools_Export.h"
#include <VtkViewerInteractor.h>    // QTools::VVI
#include <QPoint>

namespace FaceTools {
class ModelViewer;

namespace Interactor {

class FaceTools_EXPORT ModelViewerInteractor : public QTools::VVI
{ Q_OBJECT
public:
    explicit ModelViewerInteractor( ModelViewer *v=NULL);
    virtual ~ModelViewerInteractor();       // Calls setViewer(NULL) to detach.

    void setViewer( ModelViewer *v=NULL);  // Attach to given viewer or detach from current (NULL).

    ModelViewer* viewer() const { return _viewer;} // Get attached viewer.

protected:
    // Called immediately after attaching self. Can be used by derived types to add
    // other (composite) interactions for example (see RadialSelectInteractor).
    virtual void onAttached(){}

    // Called immediately after detaching self. Can be used by derived
    // types to finish interactions (e.g. emit final signals).
    virtual void onDetached(){}

private:
    ModelViewer *_viewer;
};  // end class

typedef ModelViewerInteractor MVI;

}   // end namespace
}   // end namespace

#endif