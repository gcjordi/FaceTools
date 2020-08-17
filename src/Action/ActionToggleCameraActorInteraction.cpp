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

#include <Action/ActionToggleCameraActorInteraction.h>
#include <FaceModelViewer.h>
#include <FaceModel.h>
using FaceTools::Action::ActionToggleCameraActorInteraction;
using FaceTools::Interactor::ActorMoveNotifier;
using FaceTools::Action::FaceAction;
using FaceTools::Action::Event;
using FaceTools::FaceModelViewer;
using FaceTools::ModelViewer;
using FaceTools::FM;
using FaceTools::FVS;
using FaceTools::Vis::FV;
using MS = FaceTools::Action::ModelSelector;


ActionToggleCameraActorInteraction::ActionToggleCameraActorInteraction( const QString& dn, const QIcon& ico, const QKeySequence& ks)
    : FaceAction( dn, ico, ks)
{
    connect( &_moveNotifier, &ActorMoveNotifier::onActorStart,
            this, &ActionToggleCameraActorInteraction::_doOnActorStart);
    connect( &_moveNotifier, &ActorMoveNotifier::onActorStop,
            this, &ActionToggleCameraActorInteraction::_doOnActorStop);
    connect( &_moveNotifier, &ActorMoveNotifier::onCameraStop,
            this, &ActionToggleCameraActorInteraction::_doOnCameraStop);
    setCheckable( true, false);
}   // end ctor


QString ActionToggleCameraActorInteraction::toolTip() const
{
    return "Toggle on to reposition models by clicking and dragging on them.";
}   // end toolTip


QString ActionToggleCameraActorInteraction::whatsThis() const
{
    QStringList ht;
    ht << "If toggled on, clicking and dragging on a model repositions it.";
    ht << "Click and drag with the left mouse button to rotate the model in place.";
    ht << "Click and drag with the right mouse button (or hold SHIFT while left clicking and dragging)";
    ht << "to shift the model laterally. Click and drag with the middle mouse button (or hold CTRL while";
    ht << "left or right clicking and dragging) to move the model towards or away from you.";
    ht << "Clicking and dragging outside a model switches back to camera interaction mode.";
    return tr( ht.join(" ").toStdString().c_str());
}   // end whatsThis


bool ActionToggleCameraActorInteraction::checkState( Event)
{
    return MS::interactionMode() == IMode::ACTOR_INTERACTION;
}   // end checkState


bool ActionToggleCameraActorInteraction::isAllowed( Event)
{
    const FM* fm = MS::selectedModel();
    return fm || isChecked();
}   // end isAllowedd


void ActionToggleCameraActorInteraction::doAction( Event)
{
    if ( isChecked())
    {
        MS::showStatus( "Model interaction ACTIVE");
        MS::setInteractionMode( IMode::ACTOR_INTERACTION, true);
    }   // end if
    else
    {
        MS::showStatus( "Camera interaction ACTIVE", 5000);
        MS::setInteractionMode( IMode::CAMERA_INTERACTION);
    }   // end else
}   // end doAction


Event ActionToggleCameraActorInteraction::doAfterAction( Event)
{
    return Event::NONE;
}   // end doAfterAction


void ActionToggleCameraActorInteraction::_doOnActorStart()
{
    storeUndo( this, Event::AFFINE_CHANGE | Event::CAMERA_CHANGE);
}   // end _doOnActorStart


void ActionToggleCameraActorInteraction::_doOnActorStop()
{
    emit onEvent( Event::AFFINE_CHANGE | Event::CAMERA_CHANGE);
}   // end _doOnActorStop


void ActionToggleCameraActorInteraction::_doOnCameraStop()
{
    setChecked(false);
}   // end _doOnCameraStop
