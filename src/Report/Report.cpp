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

#include <Report/Report.h>
#include <Report/ReportManager.h>
#include <Metric/MetricManager.h>
#include <Metric/PhenotypeManager.h>
#include <Metric/Chart.h>
#include <r3dio/PDFGenerator.h>
#include <FaceModel.h>
#include <FaceTools.h>
#include <Ethnicities.h>
#include <rlib/MathUtil.h>
#include <QtSvg/QSvgGenerator>
#include <QtCharts/QChartView>
#include <QPixmap>
#include <QFile>
#include <QtDebug>
using MM = FaceTools::Metric::MetricManager;
using MC = FaceTools::Metric::Metric;
using FaceTools::Metric::GrowthData;
using FaceTools::Metric::MetricValue;
using FaceTools::Metric::MetricSet;

using FaceTools::Report::Report;
using r3d::CameraParams;
using r3d::Mesh;
using FaceTools::FM;
using FaceTools::Metric::PhenotypeManager;
using FaceTools::Metric::Phenotype;

namespace {
QString sanit( QString s)
{
    // Note here that the order is very important!
    s.replace("\\", "\\textbackslash");
    s.replace("#", "\\#");
    s.replace("$", "\\$");
    s.replace("%", "\\%");
    s.replace("&", "\\&");
    s.replace("^", "\\textasciicircum");
    s.replace("_", "\\_");
    s.replace("{", "\\{");
    s.replace("}", "\\}");
    s.replace("~", "\\~{}");
    s.replace("|", "$\\mid$");
    return s;
}   // end sanit
}   // end namespace


Report::Report( QTemporaryDir& tdir) : _tmpdir(tdir), _os(nullptr), _model(nullptr)
{
    // Create some standardised Lua scripting functions for inserting report elements.
    addCustomLuaFn( "addScanInfo", [this](){ this->_addLatexScanInfo();});
    addCustomLuaFn( "addNotes", [this](){ this->_addLatexNotes();});
    addCustomLuaFn( "addStartColumn", [this](){ this->_addLatexStartMinipage();});
    addCustomLuaFn( "addEndColumn", [this](){ this->_addLatexEndMinipage();});
    addCustomLuaFn( "addLineBreak", [this](){ this->_addLatexLineBreak();});

    _lua.set_function( "getNumPhenotypicTraits",
                        [this](){ return this->_getNumPhenotypicTraits();});

    _lua.set_function( "addPhenotypicTraits",
                       [this](int sidx, int nhids)
                       { return this->_addLatexPhenotypicTraits( sidx, nhids);});

    _lua.set_function( "addFigure",
                       [this]( float widthMM, float heightMM, const std::string& caption)
                        { this->_addLatexFigure( widthMM, heightMM, caption);});

    _lua.set_function( "addGrowthCurvesChart",
                       [this]( int mid, size_t d, int footnotemark)
                        { this->_addLatexGrowthCurvesChart( mid, d, footnotemark);});

    _lua.set_function( "addCustomLatex", [this]( const std::string& s) { this->addCustomLatex( QString::fromStdString(s));});
    _lua.set_function( "addFootnoteSources", [this]( const sol::table &mids){ this->_addFootnoteSources(mids);});

    _lua.set_function( "metric", MM::metric);
    _lua.set_function( "metricSource", [this](int mid){ return this->_metricCurrentSource(mid).toStdString();});
    _lua.set_function( "footnoteIndices", [this]( const sol::table &mids){ return this->_footnoteIndices(mids);});
    _lua.set_function( "round", []( double v, size_t nd){ return rlib::round(v,nd);});

    _lua.new_usertype<MetricSet>( "MetricSet",
                                  "metric", &MetricSet::metric);

    _lua.new_usertype<MetricValue>( "MetricValue",
                                    "ndims", &MetricValue::ndims,
                                    "value", &MetricValue::value,
                                    "zscore", &MetricValue::zscore,
                                    "mean", &MetricValue::mean);

    //_lua.new_usertype<MC>( "MC",
    //                       "currentGrowthData", &MC::growthData);

    _lua.new_usertype<GrowthData>( "GrowthData",
                                   "source", &GrowthData::source,
                                   "note", &GrowthData::note,
                                   "longNote", &GrowthData::longNote);

    _lua.new_enum( "FaceSide",
                   "MID", FaceSide::MID,
                   "LEFT", FaceSide::LEFT,
                   "RIGHT", FaceSide::RIGHT);

    _lua.new_usertype<FM>( "FM",
                           "age", &FM::age,
                           "sex", &FM::sex,
                           "studyId", &FM::studyId,
                           "maternalEthnicity", &FM::maternalEthnicity,
                           "paternalEthnicity", &FM::paternalEthnicity,
                           "captureDate", &FM::captureDate,
                           "dateOfBirth", &FM::dateOfBirth,
                           "currentAssessment", &FM::cassessment);

    _lua.new_usertype<FaceAssessment>( "FaceAssessment",
                           "hasLandmarks", &FaceAssessment::hasLandmarks,
                           "metrics", &FaceAssessment::cmetrics);

    _lua.new_usertype<Phenotype>( "Phenotype",
                                  "name", &Phenotype::name);

    _lua.new_usertype<QDate>( "QDate",
                              "day", QOverload<>::of(&QDate::day),
                              "month", QOverload<>::of(&QDate::month),
                              "year", QOverload<>::of(&QDate::year));

    _lua.new_usertype<QString>( "QString",
                                "toStdString", &QString::toStdString,
                                "isEmpty", &QString::isEmpty);

    _lua.open_libraries( sol::lib::base);
    _lua.open_libraries( sol::lib::math);
}   // end ctor


Report::~Report() {}


void Report::addCustomLuaFn( const QString& fnName, const std::function<void()>& fn)
{
    _lua.set_function( fnName.toStdString(), fn);
}   // end addCustomLuaFn


void Report::addCustomLatex( const QString &s)
{
    assert(_os);
    QTextStream& os = *_os;
    os << s << Qt::endl;
}   // end addCustomLatex


bool Report::isAvailable(const FM *fm) const
{
    bool available = false;
    try
    {
        sol::function_result result = _available( fm);
        if ( result.valid())
            available = result;
    }   // end try
    catch (const sol::error& e)
    {
        qWarning() << "Lua Error!:" << e.what();
        available = false;
    }   // end catch
    return available;
}   // end isAvailable


Report::Ptr Report::load( const QString& fname, QTemporaryDir& tdir)
{
    bool loadedOk = false;
    Ptr report( new Report( tdir), []( Report* d){ delete d;});

    try
    {
        QFile file(fname);
        if ( file.open(QIODevice::ReadOnly | QIODevice::Text))  // Open for read only text
        {
            QTextStream in(&file);
            const QString code = in.readAll();
            report->_lua.script( code.toStdString());
            loadedOk = true;
        }   // end if
    }   // end try
    catch ( const sol::error& e)
    {
        qWarning() << "Unable to load and execute file '" << fname << "'!" << Qt::endl << "\t" << e.what();
    }   // end catch

    if ( !loadedOk)
        return nullptr;

    auto table = report->_lua["report"];
    if ( !table.valid())
    {
        qWarning() << "Missing table 'report'!";
        return nullptr;
    }   // end if

    if ( sol::optional<std::string> v = table["name"])
        report->_name = v.value().c_str();
    else
    {
        qWarning() << "Missing report name!";
        return nullptr;
    }   // end else

    // Reject the TEST report if this is the release build
    if ( report->name().toUpper() == "TEST")
    {
#ifndef NDEBUG
        std::cerr << "[INFO] FaceTools::Report::Report::load: Loading test report for debug version." << std::endl;
#else
        return nullptr;
#endif
    }   // end if

    if ( sol::optional<std::string> v = table["title"])
        report->_title = v.value().c_str();
    else
    {
        qWarning() << "Missing report title!";
        return nullptr;
    }   // end else

    if ( sol::optional<sol::function> v = table["isAvailable"])
        report->_available = v.value();
    else
    {
        qWarning() << "Missing 'available' function!";
        return nullptr;
    }   // end else

    if ( sol::optional<sol::function> v = table["addContent"])
        report->_addContent = v.value();
    else
    {
        qWarning() << "Missing 'addContent' function!";
        return nullptr;
    }   // end else

    return report;
}   // end load


void Report::setModelFile( const QString &pathToU3DModel)
{
    _u3dfile = QDir( _tmpdir.path()).relativeFilePath( pathToU3DModel);
}   // end setModelFile


bool Report::generate( const FM* fm, const QString& pdffile)
{
    QFile pfile(pdffile);
    if ( pfile.exists() && !pfile.remove())
    {
        qWarning() << "Cannot overwrite" << pdffile << "!";
        emit onFinishedGenerate( nullptr, pdffile);
        return false;
    }   // end if

    // Create the filestream to the raw LaTeX file
    QFile texfile( _tmpdir.filePath("report.tex"));
    texfile.open( QIODevice::ReadWrite | QIODevice::Text);
    if ( !texfile.isOpen())
    {
        qWarning() << "Unable to open '" << texfile.fileName() << "' for writing!";
        emit onFinishedGenerate( nullptr, pdffile);
        return false;
    }   // end if

    _model = fm;

    _os = new QTextStream( &texfile);
    const bool writtenLatexOk = _writeLatex( *_os);
    texfile.close();
    delete _os;

    if ( !writtenLatexOk)
    {
        qWarning() << "Unable to write latex to '" << texfile.fileName() << "'";
        emit onFinishedGenerate( nullptr, pdffile);
        return false;
    }   // end if

    //qInfo( "Generating PDF from LaTeX...");

    // Need to generate from within the directory.
    r3dio::PDFGenerator pdfgen(true);
    const bool genOk = pdfgen( QFileInfo(texfile).absoluteFilePath().toLocal8Bit().toStdString(), false);
    if ( !genOk)
    {
        qWarning() << "Failed to generate PDF!";
        emit onFinishedGenerate( nullptr, pdffile);
        return false;
    }   // end if

    // Copy the created pdf to the requested location.
    QFile::copy( _tmpdir.filePath( "report.pdf"), pdffile);

    emit onFinishedGenerate( fm, pdffile);

    return true;
}   // end generate


bool Report::_useSVG() const
{
    return !_inkscape.isEmpty() && QFile::exists(_inkscape);
}   // end _useSVG


bool Report::_writeLatex( QTextStream& os) const
{
    os << "\\documentclass[a4paper]{article}" << Qt::endl
       << "\\listfiles" << Qt::endl   // Do this to see in the .log file which packages are used
       << "\\usepackage[textwidth=20cm,textheight=25cm]{geometry}" << Qt::endl
       << "\\usepackage{graphicx}" << Qt::endl
       << "\\usepackage{verbatim}" << Qt::endl
       << "\\usepackage{xcolor}" << Qt::endl;

    if ( _useSVG())
    {
        os << "\\usepackage{svg}" << Qt::endl   // graphicx also included by svg
           << "\\setsvg{inkscape={\"" << _inkscape << "\"}}" << Qt::endl
           << "\\usepackage{relsize}" << Qt::endl;
    }   // end if

    os << "\\usepackage{float}" << Qt::endl
       << "\\usepackage[justification=centering]{caption}" << Qt::endl
       << "\\usepackage{media9}" << Qt::endl
       << "\\usepackage{amsmath}" << Qt::endl
       << "\\usepackage[parfill]{parskip}" << Qt::endl
       << "\\usepackage[colorlinks=true,urlcolor=blue]{hyperref}" << Qt::endl
       << "\\DeclareGraphicsExtensions{.png,.jpg,.pdf,.eps}" << Qt::endl;

    // Header
    os << Qt::endl
       << "\\usepackage{fancyhdr}" << Qt::endl
       << "\\pagestyle{fancy}" << Qt::endl
       //<< "\\renewcommand{\\headrulewidth}{0pt}" << Qt::endl
       //<< "\\renewcommand{\\footrulewidth}{0pt}" << Qt::endl
       //<< "\\setlength\\headheight{30mm}" << Qt::endl
       << "\\setlength\\headheight{23mm}" << Qt::endl
       << "\\rhead{\\raisebox{0.05\\height}{\\includegraphics[width=60mm]{logo.pdf}} \\\\" << Qt::endl
       << "\\footnotesize " << sanit(ReportManager::versionString()) << "}" << Qt::endl
       << "\\lhead{" << Qt::endl
       << "\\Large \\textbf{" << sanit(_title) << "} \\\\" << Qt::endl;

    os << "\\vspace{2mm} \\normalsize" << Qt::endl; // Small gap below title

    // Source and Study reference
    const QString src = sanit(_model->source().isEmpty() ? "N/A" : _model->source());
    const QString studyId = sanit(_model->studyId().isEmpty() ? "N/A" : _model->studyId());
    os << "\\textbf{Source:} " << src << "\\hspace{3mm}\\textbf{Study Ref:} " << studyId << " \\\\" << Qt::endl;

    // Subject and Image reference
    const QString subRef = sanit(_model->subjectId().isEmpty() ? "N/A" : _model->subjectId());
    const QString imgRef = sanit(_model->imageId().isEmpty() ? "N/A" : _model->imageId());
    os << "\\textbf{Subject Ref:} " << subRef << "\\hspace{3mm}\\textbf{Image Ref:} " << imgRef << " \\\\" << Qt::endl;

    os << "\\textbf{Report Date:} " << QDate::currentDate().toString("dd MMMM yyyy") << "\\\\" << Qt::endl;

    os << "}" << Qt::endl;

    // Document
    os << Qt::endl
       << "\\begin{document}" << Qt::endl
       << "\\pagenumbering{gobble}" << Qt::endl
       << "\\thispagestyle{fancy}" << Qt::endl << Qt::endl;

    bool valid = true;
    try
    {
        _addContent( _model);   // Call out to Lua function to add the report elements
    }   // end try
    catch (const sol::error& e)
    {
        qWarning() << "Lua Error: " << e.what();
        valid = false;
    }   // end catch

    os << "\\end{document}" << Qt::endl;
    os.flush();
    return valid;
}   // end _writeLaTeX


void Report::_addLatexFigure( float wmm, float hmm, const std::string& scaption)
{
    assert(_os);
    QTextStream &os = *_os;
    const QString caption = QString::fromStdString(scaption);

    static int labelID = 0;
    const QString label = QString("label%1").arg(labelID++);

    os.setRealNumberNotation( QTextStream::FixedNotation);
    os.setRealNumberPrecision( 3);

    os << R"(\begin{figure}[H]
            \centering
            \includemedia[
            label=)" << label << "," << Qt::endl
       << "width=" << wmm << "mm," << Qt::endl
       << "height=" << hmm << "mm," << Qt::endl
       << R"(add3Djscript=3Dspintool.js,   % let scene rotate about z-axis
             keepaspectratio,
             activate=pageopen,
             playbutton=plain,    % plain | fancy (default) | none
             3Dbg=1 1 1,
             3Dmenu,
             3Dviews=views.vws,
             ]{}{)" << _u3dfile << R"(}\\)" << Qt::endl;

    /* THESE DON'T WORK (and also aren't formatted well).
    os << "\\mediabutton[3Dgotoview=" << label << ":1]{\\fbox{RIGHT}}" << Qt::endl
       << "\\mediabutton[3Dgotoview=" << label << ":0]{\\fbox{FRONT}}" << Qt::endl
       << "\\mediabutton[3Dgotoview=" << label << ":2]{\\fbox{LEFT}}" << Qt::endl;
    */

    if ( !caption.isEmpty())
        os << "\\caption*{" << caption << "}" << Qt::endl;

    os << "\\end{figure}" << Qt::endl;
}   // end _addLatexFigure


void Report::_addLatexGrowthCurvesChart( int mid, size_t d, int footnotemark)
{
    MC::CPtr mc = MM::metric(mid);
    assert(mc);
    if ( !mc)
        return;

    // Ensure the chart name is unique (client may want more than one in a report).
    static int chartid = 0;
    const QString imname = QString("chart_%1").arg(abs(chartid));   // abs (LOL)
    chartid++;

    // Define the resolution and aspect ratio of the chart
    const int spx = 587;
    const int spy = 505;
    static const QRectF crect( 0,0, spx, spy);

    const GrowthData *gd = mc->growthData().current();
    Metric::Chart *chart = new Metric::Chart( gd, d, _model);
    chart->resize( crect.size());
    QtCharts::QChartView cview( chart); // Chart deleted when cview is destroyed

    cview.setRenderHint( QPainter::Antialiasing);
    cview.setSceneRect( crect);

    QPaintDevice *pdev = nullptr;
    QString imgpath;

    const bool usingSVG = _useSVG();

    if ( usingSVG)
    {
        imgpath = _tmpdir.filePath(QString("%1.svg").arg(imname));
        QSvgGenerator* simg = new QSvgGenerator;
        simg->setFileName(imgpath);
        simg->setSize( QSize( spx, spx));
        simg->setViewBox( crect);
        pdev = simg;
    }   // end if
    else
    {
        imgpath = _tmpdir.filePath(QString("%1.png").arg(imname));
        QPixmap* pimg = new QPixmap( QSize( spx, spy));
        pimg->fill( Qt::transparent);
        pdev = pimg;
    }   // end else

    QPainter painter;
    painter.begin( pdev);
    painter.setRenderHint( QPainter::Antialiasing);
    cview.render( &painter, QRectF(0, 0, spx, spy), cview.viewport()->rect());
    painter.end();

    if ( !usingSVG && !static_cast<QPixmap*>(pdev)->save( imgpath))
    {
        qWarning( "Unable to save PNG!");
        return;
    }   // end if

    QString qcaption = chart->makeLatexTitleString( footnotemark);
    assert(_os);
    QTextStream& os = *_os;
    os << R"(\begin{figure}[H]
             \centering)" << Qt::endl;
    if ( !qcaption.isEmpty())
        os << "\\caption*{" << qcaption << "}" << Qt::endl;

    if ( usingSVG)
    {
        os << "\\includesvg[width=93.00mm]{" << imname << "}" << Qt::endl;
        delete static_cast<QSvgGenerator*>(pdev);
    }   // end if
    else
    {
        os << "\\includegraphics[width=\\linewidth]{" << imname << "}" << Qt::endl;
        delete static_cast<QPixmap*>(pdev);
    }   // end else

    os << "\\end{figure}" << Qt::endl;
}   // end _addLatexGrowthCurvesChart


void Report::_addLatexScanInfo()
{
    const QString cdate = _model->captureDate().toString("dd MMMM yyyy");
    const QString dob = _model->dateOfBirth().toString("dd MMMM yyyy");
    const double age = _model->age();
    const int yrs = int(age);
    const int mths = int((age - double(yrs)) * 12);
    const QString sage = QString("%1 yrs. %2 mons.").arg(yrs).arg(mths);

    const QString sexs = sanit(FaceTools::toLongSexString( _model->sex()));
    QString eths = sanit(Ethnicities::name(_model->maternalEthnicity()));
    if ( _model->maternalEthnicity() != _model->paternalEthnicity())
        eths += sanit(" (M) & " + Ethnicities::name(_model->paternalEthnicity()) + " (P)");
    if ( eths.isEmpty())
        eths = "N/A";

    QTextStream& os = *_os;
    os << "\\textbf{Image Captured:} " << cdate << " \\\\" << Qt::endl;
    os << "\\textbf{DOB:} " << dob << "\\hspace{3mm} \\textbf{Age:} " << sage << " \\\\" << Qt::endl;
    os << "\\textbf{Ethnicity:} " << eths << " \\\\" << Qt::endl;
    os << "\\textbf{Sex:} " << sexs << " \\\\" << Qt::endl;
}   // end _addLatexScanInfo


void Report::_addLatexNotes()
{
    QTextStream& os = *_os;
    FaceAssessment::CPtr ass = _model->currentAssessment();
    os << " \\normalsize{\\textbf{" << tr("Assessor") << ":} " << sanit(ass->assessor()) << R"(} \\)" << Qt::endl;
    os << R"( \small{)" << sanit( ass->hasNotes() ? ass->notes() : tr("Nothing recorded.")) << R"(} \\)" << Qt::endl;
}   // end _addLatexNotes


int Report::_getNumPhenotypicTraits() const
{
    const IntSet pids = PhenotypeManager::discover(_model, -1); // Use the current assessment id
    return int(pids.size());
}   // end _getNumPhenotypicTraits


bool Report::_addLatexPhenotypicTraits( int sidx, int nhids)
{
    sidx = std::max( 0, sidx);
    const IntSet pids = PhenotypeManager::discover(_model, -1); // Use the current assessment id
    bool moreToGo = false;

    QTextStream& os = *_os;

    if ( pids.empty())
        os << tr("No notable morphology found.") << Qt::endl;
    else
    {
        if ( int(pids.size()) <= sidx)
            return false;

        const int topidx = std::min<int>( int(pids.size()), sidx + nhids);
        if ( int(pids.size()) > topidx)
            moreToGo = true;

        std::list<int> lids( pids.begin(), pids.end());
        lids.sort();
        std::vector<int> vids( lids.begin(), lids.end());
        const QString title = tr("Notable morphology");
        os << "\\textbf{" << title << ":}" << Qt::endl
           << "\\begin{itemize}" << Qt::endl;
        for ( int i = sidx; i < topidx; ++i)
            os << "\t\\item " << PhenotypeManager::latexLinkString(vids[i]) << Qt::endl;
        os << "\\end{itemize}" << Qt::endl;
    }   // end else

    return moreToGo;
}   // end _addLatexPhenotypicTraits


void Report::_addLatexStartMinipage() { *_os << R"(\begin{minipage}[t]{.5\textwidth})" << Qt::endl;}
void Report::_addLatexEndMinipage() { *_os << R"(\end{minipage})" << Qt::endl;}

void Report::_addLatexLineBreak() { *_os << R"(\hfill \break)" << Qt::endl;}


QString Report::_metricCurrentSource( int mid) const
{
    const GrowthData *gd = nullptr;
    if ( MM::metric(mid))
        gd = MM::metric(mid)->growthData().current();
    if ( !gd)
        return "";
    QString src = gd->source();
    if ( !gd->note().isEmpty())
        src += " " + gd->note();
    return src;
}   // end _metricCurrentSource


std::unordered_map<int, int> Report::_footnoteIndices( const sol::table& mids) const
{
    QMap<QString, int> refs;
    std::unordered_map<int, int> idxs;
    int idx = 1;
    for ( size_t i = 0; i < mids.size(); ++i)
    {
        const int mid = mids[i];
        const QString src = _metricCurrentSource(mid);
        if ( src.isEmpty())
            continue;

        if ( refs.count(src) > 0)
            idxs[mid] = refs.value(src);
        else
        {
            refs[src] = mid;
            idxs[mid] = idx++;
        }   // end else
    }   // end for
    return idxs;
}   // end _footnoteIndices


void Report::_addFootnoteSources( const sol::table& mids)
{
    assert(_os);
    QTextStream& os = *_os;

    QSet<QString> refs;
    int idx = 1;
    for ( size_t i = 0; i < mids.size(); ++i)
    {
        const int mid = mids[i];
        const QString src = _metricCurrentSource(mid);
        if ( !src.isEmpty() && !refs.contains(src))
        {
            refs.insert(src);
            os << "\\footnotetext[" << idx << "]{" << src << "}" << Qt::endl;
            idx++;
        }   // end if
    }   // end for
}   // end _addFootnoteSources
