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

#include <FileIO/FaceModelXMLFileHandler.h>
#include <Metric/PhenotypeManager.h>
#include <MaskRegistration.h>
#include <FaceTools.h>
#include <FaceModel.h>
#include <Ethnicities.h>
#include <MiscFunctions.h>
#include <r3dio/IOHelpers.h>
#include <QTemporaryDir>
#include <quazip5/JlCompress.h>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <ctime>
using FaceTools::FileIO::FaceModelXMLFileHandler;
using FaceTools::Metric::PhenotypeManager;
using FaceTools::Metric::Phenotype;
using FaceTools::FM;


void FaceTools::FileIO::exportMetaData( const FM* fm, bool withExtras, PTree& tnode)
{
    PTree& rnode = tnode.add("FaceModel","");

    rnode.put( "Source", fm->source().toStdString());
    rnode.put( "StudyId", fm->studyId().toStdString());
    rnode.put( "SubjectId", fm->subjectId().toStdString());
    rnode.put( "ImageId", fm->imageId().toStdString());
    rnode.put( "DateOfBirth", fm->dateOfBirth().toString().toStdString());
    rnode.put( "Sex", FaceTools::toSexString(fm->sex()).toStdString());
    rnode.put( "MaternalEthnicity", fm->maternalEthnicity());
    rnode.put( "PaternalEthnicity", fm->paternalEthnicity());
    rnode.put( "CaptureDate", fm->captureDate().toString().toStdString());
    if ( fm->hasMask())
    {
        PTree &maskNode = rnode.put( "Mask", "");
        maskNode.put( "Hash", fm->maskHash());
    }   // end if

    PTree& anodes = rnode.put("Assessments", "");
    anodes.put( "DefaultAssessment", fm->currentAssessment()->id());
    const double age = fm->age();
    const IntSet aids = fm->assessmentIds();
    for ( int id : aids)
    {
        FaceAssessment::CPtr ass = fm->assessment( id);

        PTree& anode = anodes.add("Assessment", "");
        anode.put( "AssessmentId", id);
        anode.put( "Assessor", ass->assessor().toStdString());
        anode.put( "Notes", ass->notes().toStdString());
        ass->landmarks().write( anode.put("Landmarks", ""));
        ass->paths().write( anode.put("Paths", ""), withExtras);

        PTree& mgrps = anode.put("MetricGroups", "");
        ass->cmetrics( MID).write( mgrps.put("Frontal", ""), age);
        ass->cmetrics( LEFT).write( mgrps.put("LeftLateral", ""), age);
        ass->cmetrics( RIGHT).write( mgrps.put("RightLateral", ""), age);

        PTree& pnodes = anode.put("HPO_Terms", "");
        const IntSet ptypes = PhenotypeManager::discover( fm, id);
        for ( int hid : ptypes)
        {
            PTree& pnode = pnodes.add( "Term", "");
            const Phenotype::Ptr hpo = PhenotypeManager::phenotype(hid);
            pnode.put( "ID", PhenotypeManager::formattedId(hid).toStdString());
            pnode.put( "Name", hpo->name().toStdString());
            pnode.put( "Metrics", hpo->metricsList().toStdString());
            pnode.put( "AgeMatch", hpo->isAgeMatch( age));
            pnode.put( "SexMatch", hpo->isSexMatch( fm->sex()));
            pnode.put( "MaternalEthnicityMatch", hpo->isEthnicityMatch(fm->maternalEthnicity()));
            pnode.put( "PaternalEthnicityMatch", hpo->isEthnicityMatch(fm->paternalEthnicity()));
        }   // end for
    }   // end for
}   // end exportMetaData


namespace {
PTree& exportXMLHeader( PTree& tree)
{
    PTree& topNode = tree.put( "faces","");
    topNode.put( "<xmlattr>.version", FaceTools::FileIO::XML_VERSION.toStdString());
    std::ostringstream desc;
    desc << FaceTools::FileIO::XML_FILE_DESCRIPTION.toStdString() << ";" << time(nullptr);
    topNode.put( "description", desc.str());

    PTree& records = topNode.put( "FaceModels","");    // Only a single record
    records.put( "<xmlattr>.count", 1);
    return records;
}   // end exportXMLHeader
}   // end namespace


// public
bool FaceModelXMLFileHandler::write( const FM* fm, const QString& fname)
{
    assert(fm);
    _err = "";

    try
    {
        QTemporaryDir tdir( QDir::tempPath() + "/" + QFileInfo( fname).baseName());
        if ( !tdir.isValid())
        {
            _err = "Unable to create temporary directory for writing to!";
            return false;
        }   // end if

        std::ofstream ofs;
        ofs.open( tdir.filePath( "meta.xml").toLocal8Bit().toStdString());
        if ( !ofs.is_open())
        {
            _err = "Cannot open output file stream for writing metadata!";
            return false;
        }   // end if

        // Export metadata
        PTree tree;
        PTree& rnode = exportXMLHeader( tree);
        exportMetaData( fm, false/*no extra data*/, rnode);
        boost::property_tree::write_xml( ofs, tree);
        ofs.close();

        // Write out the model geometry itself into .obj format.
        if ( !r3dio::saveAsOBJ( fm->mesh(), tdir.filePath( "mesh.obj").toLocal8Bit().toStdString(), false/*as jpeg*/))
        {
            _err = "Failed to write mesh!";
            return false;
        }   // end if

        // Write out the mask if set
        if ( fm->hasMask() && !r3dio::saveAsPLY( fm->mask(), tdir.filePath( "mask.ply").toLocal8Bit().toStdString()))
        {
            _err = "Failed to write mask!";
            return false;
        }   // end if

        // Export jpeg thumbnail of model.
        const cv::Mat img = makeThumbnail( fm, cv::Size(256,256), 500);   // Generate thumbnail
        cv::imwrite( tdir.filePath("thumb.jpg").toLocal8Bit().toStdString(), img);

        // Finally, zip up the contents of the directory into fname.
        if ( !JlCompress::compressDir( fname, tdir.path(), true/*recursively pack subdirs*/))
            _err = "Unable to compress saved data into archive format!";
    }   // end try
    catch ( const std::exception& e)
    {
        std::cerr << "[EXCEPTION] FaceTools::FileIO::FaceModelXMLFileHandler::write: Failed to write to " << fname.toStdString() << std::endl;
        _err = e.what();
    }   // end catch

    return _err.isEmpty();
}   // end write


namespace {

QDate getDateRecord( const PTree &n, const QString &camelCaseLabel)
{
    QDate date = QDate::currentDate();
    const QString sdate = QString::fromStdString( FaceTools::FileIO::getRecord<std::string>( n, camelCaseLabel));
    if ( !sdate.isEmpty() && QDate::fromString(sdate).isValid())
        date = QDate::fromString( sdate);
    return date;
}   // end getDateRecord


QString getStringRecord( const PTree &n, const QString &camelCaseLabel)
{
    return QString::fromStdString( FaceTools::FileIO::getRecord<std::string>( n, camelCaseLabel));
}   // end getStringRecord


void importModelRecord( FM &fm, const PTree& rnode, QString &meshfname, QString &maskfname)
{
    meshfname = getStringRecord( rnode, "ObjFilename");
    if ( meshfname.isEmpty())
        meshfname = "mesh.obj";

    fm.setSource( getStringRecord( rnode, "Source"));
    fm.setStudyId( getStringRecord( rnode, "StudyId"));
    fm.setSubjectId( getStringRecord( rnode, "SubjectId"));
    fm.setImageId( getStringRecord( rnode, "ImageId"));
    fm.setCaptureDate( getDateRecord( rnode, "CaptureDate"));
    QDate dob = getDateRecord( rnode, "DateOfBirth");
    // If no date of birth given, find it as the capture date minus the age (assumed given)
    if ( dob == QDate::currentDate() && fm.captureDate() != QDate::currentDate())
    {
        const double age = FaceTools::FileIO::getRecord<double>( rnode, "Age");   // Will be zero if not found
        if ( age > 0.0)
            dob = fm.captureDate().addDays(qint64( -age * 365.25));
    }   // end if
    fm.setDateOfBirth( dob);

    fm.setSex( FaceTools::fromSexString( getStringRecord( rnode, "Sex")));

    int methn = FaceTools::FileIO::getRecord<int>( rnode, "MaternalEthnicity");
    int pethn = FaceTools::FileIO::getRecord<int>( rnode, "PaternalEthnicity");
    if ( methn == 0 && pethn == 0)
    {
        const QString seth = getStringRecord( rnode, "Ethnicity");
        if ( !seth.isEmpty())
            pethn = methn = FaceTools::Ethnicities::code( seth.split(" ")[0].trimmed());  // Use just the first word
    }   // end if
    fm.setMaternalEthnicity(methn);
    fm.setPaternalEthnicity(pethn);

    if ( rnode.count("Mask") > 0)
    {
        maskfname = "mask.ply";
        const PTree &maskNode = rnode.get_child("Mask");
        if ( maskNode.count("Filename") > 0)
        {
            maskfname = getStringRecord( maskNode, "Filename"); // Will end in 3DF - needs to end with OBJ
            const QFileInfo finfo( maskfname);
            if ( finfo.suffix().toLower() == "3df")
                maskfname = finfo.baseName() + ".obj";
        }   // end if
        if ( maskNode.count("Hash") > 0)
            fm.setMaskHash( maskNode.get<size_t>("Hash"));
    }   // end if

    /**
     * Older version of this file format save landmarks, paths, and notes in the root node.
     * If the file has these elements in the root node, set them on the current assessment.
     */
    FaceTools::FaceAssessment::Ptr ass = fm.currentAssessment();   // Has ID zero

    // Read in the landmarks
    FaceTools::Landmark::LandmarkSet lmks;
    if ( rnode.count("Landmarks") > 0)
    {
        lmks.read( rnode.get_child("Landmarks"));
        ass->setLandmarks( lmks);
    }   // end if

    // Read in the saved paths
    FaceTools::PathSet paths;
    if ( rnode.count("Paths") > 0)
    {
        paths.read( rnode.get_child("Paths"));
        ass->setPaths( paths);
    }   // end if

    QString notes = getStringRecord( rnode, "Notes");
    if ( notes.isEmpty())
        notes = getStringRecord( rnode, "Description");

    QString assessor = "Unknown";
    ass->setAssessor( assessor);
    if ( !notes.isEmpty())
        ass->setNotes( notes);

    if ( rnode.count("Assessments") > 0)
    {
        fm.eraseAssessment(0);    // Remove the default assessment since defining explicitly

        const PTree& anodes = rnode.get_child("Assessments");
        const int cassid = anodes.count("DefaultAssessment") > 0 ? anodes.get<int>( "DefaultAssessment") : 0;
        int nassid = 0;   // Assessment ids if not defined
        int assid = 0;

        for ( const PTree::value_type& vnode : anodes)
        {
            const PTree& anode = vnode.second;
            assessor = getStringRecord( anode, "Assessor");
            notes = getStringRecord( anode, "Notes");
            lmks = FaceTools::Landmark::LandmarkSet();
            if ( anode.count("Landmarks") > 0)
                lmks.read( anode.get_child("Landmarks"));

            paths.reset();
            if ( anode.count("Paths") > 0)
                paths.read( anode.get_child("Paths"));

            // Skip no content entries
            if ( assessor.isEmpty() && notes.isEmpty() && lmks.empty() && paths.empty())
                continue;

            if ( assessor.isEmpty())
                assessor = "Unknown";

            assid = anode.count("AssessmentId") > 0 ? anode.get<int>("AssessmentId") : nassid;
            ass = FaceTools::FaceAssessment::create( assid);
            if ( ass->id() >= nassid)
                nassid = ass->id() + 1;

            ass->setAssessor( assessor);
            ass->setNotes( notes);
            ass->setLandmarks( lmks);
            ass->setPaths( paths);
            fm.setAssessment( ass);
        }   // end for

        // Get the created assessment ids.
        const IntSet aids = fm.assessmentIds();

        // If there were no assessments actually created, we need to add back in a default empty one!
        if ( aids.empty())
            fm.setAssessment( FaceTools::FaceAssessment::create(0));
        else if ( aids.count(cassid) > 0)
            fm.setCurrentAssessment(cassid);
    }   // end if
}   // end importModelRecord
}   // end namespace


bool FaceTools::FileIO::importMetaData( FM &fm, const PTree& tree, double &fvers)
{
    QString notused0, notused1;
    return importMetaData( fm, tree, fvers, notused0, notused1);
}   // end importMetaData


bool FaceTools::FileIO::importMetaData( FM &fm, const PTree& tree, double &fvers, QString& meshfname, QString& maskfname)
{
    static const std::string msghd( " FaceTools::FileIO::importMetaData: ");

    boost::optional<std::string> vstr;
    const PTree* fnode = nullptr; // Will point to the FaceModel record

    if ( tree.count("faces") > 0)
    {
        const PTree& faces = tree.get_child( "faces");
        vstr = faces.get_optional<std::string>( "<xmlattr>.version");

        /*
        std::string filedesc;
        if ( faces.count("description") > 0)
            filedesc = faces.get<std::string>( "description");
        else if ( faces.count("Description") > 0)
            filedesc = faces.get<std::string>( "Description");

        if ( !filedesc.empty())
            std::cerr << "[INFO]" << msghd << "\"" << filedesc << "\"" << std::endl;
        */

        if ( faces.count("FaceModels") > 0)
        {
            const PTree& fms = faces.get_child( "FaceModels");
            if ( fms.count("FaceModel") > 0)
                fnode = &fms.get_child("FaceModel");
        }   // end if
        else if ( faces.count("FaceModel") > 0)
            fnode = &faces.get_child("FaceModel");
    }   // end if
    else if ( tree.count("FaceModel") > 0)
        fnode = &tree.get_child("FaceModel");

    fvers = 0.0;
    if ( fnode)
    {
        if ( !vstr)
            vstr = fnode->get_optional<std::string>( "<xmlattr>.version");
    }   // end if
    else
    {
        std::cerr << "[WARNING]" << msghd << "FaceModel record not found!" << std::endl;
        return false;
    }   // end else

    static const double VERSION = XML_VERSION.toDouble();

    fvers = VERSION;
    if ( vstr)
    {
        fvers = QString::fromStdString(*vstr).toDouble();
        //std::cerr << "Read in XML data from version " << *vstr << " file" << std::endl;
    }   // end if

    // If the file version is higher, then compatibility may not be possible (still try to read in).
    if ( fvers > VERSION)
        std::cerr << "[WARNING]" << msghd << "Higher version " << fvers << " of cannot be read into version " << VERSION << " library!" << std::endl;

    importModelRecord( fm, *fnode, meshfname, maskfname);
    return true;
}   // end importMetaData


QString FaceTools::FileIO::readMeta( const QString &fname, QTemporaryDir &tdir, PTree &tree)
{
    QString err;

    try
    {
        if ( !tdir.isValid())
            return "Unable to open temporary directory for reading from!";

        QStringList fnames = JlCompress::extractDir( fname, tdir.path());
        if ( fnames.isEmpty())
            return "Unable to extract files from archive!";

        QStringList xmlList = QDir( tdir.path()).entryList( {"*.xml"});
        QString xmlfile;
        if ( xmlList.size() == 1)
            xmlfile = tdir.filePath( xmlList.first());

        if ( xmlfile.isEmpty() || !QFileInfo(xmlfile).isFile())
            return "Cannot find metadata in archive!";

        std::ifstream ifs;
        ifs.open( xmlfile.toLocal8Bit().toStdString());
        if ( !ifs.is_open())
            return "Cannot open metadata file for reading!";

        boost::property_tree::read_xml( ifs, tree);
    }   // end try
    catch ( const boost::property_tree::ptree_bad_path&)
    {
        err = "XML bad path error encountered reading in stream data!";
    }   // end catch
    catch ( const boost::property_tree::xml_parser_error&)
    {
        err = "XML parse error encountered reading in stream data!";
    }   // end catch
    catch ( const std::exception&)
    {
        err = "Unable to read in stream data!";
    }   // end catch

    return err;
}   // end readMeta


QString FaceTools::FileIO::loadData( FM &fm, const QTemporaryDir &tdir, const QString &meshfname, const QString &maskfname)
{
    QString err;
    try
    {
        // Raw model - no post process undertaken!
        r3d::Mesh::Ptr mesh = r3dio::loadMesh( tdir.filePath( meshfname).toLocal8Bit().toStdString());
        if ( mesh)
        {
            fm.update( mesh, true, false/*don't resettle landmarks (or update paths) just read in*/);
            for ( int aid : fm.assessmentIds()) // Do want to update paths over the mesh though
                fm.assessment(aid)->paths().update( &fm);
        }   // end if
        else
            return QString("Couldn't load main mesh from '%1'").arg( meshfname);

        // Also load the mask if present
        if ( !maskfname.isEmpty())
        {
            r3d::Mesh::Ptr mask = r3dio::loadMesh( tdir.filePath( maskfname).toLocal8Bit().toStdString());
            if ( mask)
            {
                assert( fm.maskHash() != 0);
                // Always ensure that the model is loaded aligned if mask available (fixed for 5.0.2).
                r3d::Mat4f T = MaskRegistration::calcMaskAlignment( *mask);
                fm.addTransformMatrix( T.inverse());
                fm.fixTransformMatrix();
            }   // end if
            else
                std::cout << "Mask not loaded - setting null!" << std::endl;
            fm.setMask( mask);
        }   // end if
    }   // end try
    catch ( const std::exception& e)
    {
        err = "Unable to read in stream data!";
        std::cerr << e.what() << std::endl;
    }   // end catch

    return err;
}   // end loadData


FM* FaceModelXMLFileHandler::read( const QString& fname)
{
    _err = "";

    QTemporaryDir tdir;
    if ( !tdir.isValid())
    {
        _err = "Unable to create temporary directory for file extraction!";
        return nullptr;
    }   // end if

    FM *fm = new FM;
    PTree tree;
    _err = readMeta( fname, tdir, tree);
    if ( _err.isEmpty())
    {
        _fversion = 0.0;
        QString meshfname, maskfname;
        if ( !importMetaData( *fm, tree, _fversion, meshfname, maskfname))
            _err = QObject::tr("No FaceModel objects recorded in file!");
        else
        {
            if ( _fversion > XML_VERSION.toDouble())
                _err = QObject::tr("File version is more recent than this library allows!");
            else
                _err = loadData( *fm, tdir, meshfname, maskfname);
        }   // end else
    }   // end if

    if ( !_err.isEmpty())
    {
        delete fm;
        fm = nullptr;
    }   // end if

    return fm;
}   // end read
