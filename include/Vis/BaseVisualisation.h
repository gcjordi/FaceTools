/************************************************************************
 * Copyright (C) 2018 Spatial Information Systems Research Limited
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

#ifndef FACE_TOOLS_VIS_BASE_VISUALISATION_H
#define FACE_TOOLS_VIS_BASE_VISUALISATION_H

#include "VisualisationInterface.h"
#include <vtkMatrix4x4.h>
#include <vtkProp.h>
#include <cassert>

namespace FaceTools {
namespace Vis {

// Derived classes should inherit BaseVisualisation instead of VisualisationInterface.
class FaceTools_EXPORT BaseVisualisation : public VisualisationInterface
{ Q_OBJECT
public:
    BaseVisualisation( const QString& displayName, const QIcon&, const QKeySequence&);
    BaseVisualisation( const QString& displayName, const QIcon&);
    explicit BaseVisualisation( const QString& displayName);
    ~BaseVisualisation() override;

    QString getDisplayName() const override { return _dname;}

    const QIcon* getIcon() const override { return _icon;}
    const QKeySequence* getShortcut() const override { return _keys;}

    bool isAvailable( const FM*) const override { return true;}
    bool isAvailable( const FV*, const QPoint*) const override;

    // Visualisations that are toggled (default) can be turned on and off as layers over the base view.
    // A non-toggled visualisation is mutually exclusive wrt any other non-toggled visualisation.
    bool isToggled() const override { return true;} // DEPRECATE

    // If a visualisation is non-toggled, then it is exclusive by default. Visualisations
    // that toggle on/off can also enforce exclusivity when toggling on. This is the case
    // with the TextureVisualisation (declared in this file).
    virtual bool isExclusive() const { return false;}// DEPRECATE

    // Should this visualisation be available in the UI?
    virtual bool isUIVisible() const { return true;}    // DEPRECATE

    // By default, visualisations are applied to all FaceViews in the selected FaceView(s) viewer(s).
    // This behaviour can be changed by overriding applyToAllInViewer to return false, in which case
    // the visualisation is applied only to the selected FaceView(s). If applyToSelectedModel is
    // overridden to return true, the selected set of FaceViews is first expanded to be the superset
    // of all FaceViews owned by every selected FaceModel.
    virtual bool applyToAllInViewer() const { return true;}     // DEPRECATE
    virtual bool applyToSelectedModel() const { return false;}  // DEPRECATE

    // Return true if the given FaceModel satisfies this visualisation's conditions for
    // switching this visualisation on immediately after loading the given FaceModel.
    // (Some visualisations will have certain unmet data dependencies upon load).
    virtual bool allowShowOnLoad( const FM*) const { return true;}  // DEPRECATE

    // Specifies if application of this visualisation should be automatically
    // synchronised with a FV's ready (selected) status.
    virtual bool applyOnReady() const { return false;}  // DEPRECATE


    // Return true iff the given prop relating to the given FV belongs to
    // this visualisation. Typically, visualisations do not apply extra actors
    // so the default implementation defaults to returning false.
    virtual bool belongs( const vtkProp*, const FV*) const { return false;}

    // Poke the matrix transform for this visualisation's actors (if any) into a temporary state.
    virtual void pokeTransform( const FV*, const vtkMatrix4x4*){}

    // Fix the current transformation matrix of this visualisation's actors (if any)
    // and reset them back to the untransformed (Identity) matrix.
    virtual void fixTransform( const FV*){}

    // Copy the visualisation from src to dst. The underlying FaceModel for the two
    // views must be the same! Copying is only necessary if the visualisation can't
    // obtain its complete state from calling apply on the destination view, so this
    // function should only be reimplemented if the default forwarding of this call
    // to apply(dst) leaves the visualisation in an incomplete state.
    void copy( FV* dst, const FV*) override { this->apply(dst);}

    // Apply the visualisation for the given FaceView (add associated actors to viewer).
    // void apply( FV*, const QPoint* p=nullptr) override;
    // Remove the visualisation for the given FaceView. Do not delete associated actors
    // until purge is called.
    // void clear( FV*) override;

    // Destroy any cached data associated with the given view.
    virtual void purge( FV*){}

    // Destroy any cached data associated with the given model.
    virtual void purge( const FM*){}

private:
    QString _dname;                 // Display name
    const QIcon *_icon;             // Display icon
    const QKeySequence *_keys;      // Key shortcut

    BaseVisualisation( const BaseVisualisation&) = delete;
    void operator=( const BaseVisualisation&) = delete;
};  // end class


class FaceTools_EXPORT TextureVisualisation : public BaseVisualisation
{ Q_OBJECT
public:
    TextureVisualisation( const QString& displayName="Texture Map On/Off",
                          const QIcon &icon=QIcon(":/icons/TEXTURE"),
                          const QKeySequence &keys=QKeySequence(Qt::Key_T))
        : BaseVisualisation( displayName, icon, keys) {}

    bool isExclusive() const override { return true;}

    void apply( FV*, const QPoint* p=nullptr) override;   // Apply texture map.
    void clear( FV*) override;  // Remove texture map.

    // Only available to check on if the model has a single material (must merge materials).
    bool isAvailable( const FM*) const override;

    bool belongs( const vtkProp*, const FV*) const override; // Returns true iff fc->view()->isFace(prop)
};  // end class


class FaceTools_EXPORT WireframeVisualisation : public BaseVisualisation
{ Q_OBJECT
public:
    WireframeVisualisation( const QString& displayName="Wireframe On/Off",
                            const QIcon &icon=QIcon(":/icons/WIREFRAME_VIS"),
                            const QKeySequence &keys=QKeySequence(Qt::Key_W))
        : BaseVisualisation( displayName, icon, keys) {}

    void apply( FV*, const QPoint* p=nullptr) override;   // Add wireframe.
    void clear( FV*) override;  // Remove wireframe.

    bool belongs( const vtkProp*, const FV*) const override; // Returns true iff fc->view()->isFace(prop)
};  // end class

}   // end namespace
}   // end namespace

#endif
