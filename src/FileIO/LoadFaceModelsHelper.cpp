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

#include <FileIO/LoadFaceModelsHelper.h>
#include <FileIO/FaceModelManager.h>
#include <FaceModel.h>
#include <QMessageBox>
#include <QFile>
using FaceTools::FileIO::LoadFaceModelsHelper;
using FMM = FaceTools::FileIO::FaceModelManager;


LoadFaceModelsHelper::LoadFaceModelsHelper( QWidget* parent) : _parent(parent) {}

int LoadFaceModelsHelper::setFilteredFilenames( const QStringList& fnames)
{
    // Get the supported filenames.
    QStringList notpres;
    QStringList notsupp;
    QStringList areopen;
    _filenames.clear();

    for ( const QString& qfname : fnames)
    {
        if ( !QFile::exists( qfname))
            notpres << qfname;
        else if ( !FMM::canRead( qfname))
            notsupp << qfname;
        else if ( FMM::isOpen( qfname))
            areopen << qfname;
        else
            _filenames << qfname;
    }   // end for

    // Check the load limit (assume that all selected can be loaded)
    if ( _filenames.size() + int(FMM::numOpen()) > int(FMM::loadLimit()))
    {
        const size_t nallowed = FMM::loadLimit() - FMM::numOpen();
        QString msg = QObject::tr("Too many files selected! Only ");
        msg.append( QString::number( FMM::loadLimit())).append( QObject::tr(" models are allowed to be open at once."));
        if ( nallowed == 1)
            msg.append( QObject::tr("Only one more model may be loaded."));
        else
            msg.append( QObject::tr("%1 more models may be loaded.").arg(nallowed));
        QMessageBox::warning( _parent, QObject::tr("Load limit reached!"), msg);
        _filenames.clear();
    }   // end if

    // Show warning for not supported files
    if ( notpres.size() == 1)
        QMessageBox::warning( _parent, QObject::tr("Not existent file!"), QObject::tr( "Can't find file ") + notpres.join(" "));
    else if ( notpres.size() > 1)
        QMessageBox::warning( _parent, QObject::tr("Not existent files!"), QObject::tr( "Can't find files ") + notpres.join(" "));

    if ( notsupp.size() == 1)
        QMessageBox::warning( _parent, QObject::tr("Unsupported file type!"), notsupp.join(" ") + QObject::tr(" has an unsupported format!"));
    else if ( notsupp.size() > 1)
        QMessageBox::warning( _parent, QObject::tr("Unsupported file types!"), notsupp.join(", ") + QObject::tr(" are not supported file types!"));

    if ( areopen.size() == 1)
        QMessageBox::warning( _parent, QObject::tr("File already open!"), areopen.join(" ") + QObject::tr(" is already open!"));
    else if ( areopen.size() > 1)
        QMessageBox::warning( _parent, QObject::tr("Files already open!"), areopen.join(", ") + QObject::tr(" are already open!"));

    return _filenames.size();
}   // end setFilteredFilenames


size_t LoadFaceModelsHelper::loadModels()
{
    _loaded.clear();
    _failnames.clear();
    for ( const QString& fname : _filenames)
    {
        FM* fm = FMM::read( fname);   // Blocks
        if ( fm)
            _loaded.insert( fm);
        else
            _failnames[FMM::error()] << fname;
    }   // end for
    _filenames.clear();
    return _loaded.size();
}   // end loadModels


bool LoadFaceModelsHelper::loadModel( const QString& filename)
{
    QStringList flist;
    flist << filename;
    if ( setFilteredFilenames( flist) < 1)
        return false;
    return loadModels() == 1;
}   // end loadModel


void LoadFaceModelsHelper::showLoadErrors()
{
    // For each error type, display a warning dialog
    for ( auto f : _failnames)
    {
        QString msg = f.first + QObject::tr("\nUnable to load the following:\n");
        msg.append( f.second.join("\n"));
        QMessageBox::warning( _parent, QObject::tr("Unable to load file(s)!"), msg);
    }   // end for
    _failnames.clear();
}   // end showLoadErrors
