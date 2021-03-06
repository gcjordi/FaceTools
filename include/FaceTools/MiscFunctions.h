/************************************************************************
 * Copyright (C) 2018 SIS Research Ltd & Richard Palmer
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

#ifndef FACE_TOOLS_MISC_FUNCTIONS_H
#define FACE_TOOLS_MISC_FUNCTIONS_H

#include "FaceTypes.h"
#include <r3d/Mesh.h>   // r3d
#include <QTemporaryFile>
#include <vtkIdList.h>

namespace FaceTools {

// Returns the string context of the given file or an empty string on failure.
FaceTools_EXPORT QString loadTextFromFile( const QString& fname);

// Write the contents of a resource file to a temporary file which is returned.
// Only necessary if files are not going to be read using QFile (which integrates
// with Qt's resource system). The client should destroy the returned object when
// done with the file's contents.
FaceTools_EXPORT QTemporaryFile* writeToTempFile( const QString& rfile);

FaceTools_EXPORT std::string getDateTimeDigits( const std::string&); // Get the first 10 digits as a string from given string.

// Construct and return a string of the form "P pos[0] X | pos[1] Y | pos[2] Z" where fw is the field width
// that each value takes up. Floating point representation with precision of 2 d.p. is used.
FaceTools_EXPORT QString posString( const QString prefix, const Vec3f& pos, int fw=8);

FaceTools_EXPORT long getDateTimeSecs( const std::string&);

FaceTools_EXPORT cv::Point calcCentre( const cv::Rect&, float scaleFactor=1.0f);

FaceTools_EXPORT cv::Point2f calcCentre( const cv::Rect_<float>&, float scaleFactor=1.0f);
FaceTools_EXPORT float calcDistance( const cv::Point&, const cv::Point&);
FaceTools_EXPORT float calcDistance( const cv::Point2f&, const cv::Point2f&);
FaceTools_EXPORT cv::Point calcMid( const cv::Point&, const cv::Point&);
FaceTools_EXPORT cv::Point2f calcMid( const cv::Point2f&, const cv::Point2f&);

FaceTools_EXPORT Vec3f calcSum( const std::vector<Vec3f>&);
FaceTools_EXPORT Vec3f calcMean( const std::vector<Vec3f>&);

// Calculates the sum over the points specified. A single point (or multiple same
// points) returns a length of zero. Empty vectors return a length of zero.
FaceTools_EXPORT float calcLength( const std::vector<Vec3f>&);

// Calculate and return length of list of vertices in given order. Vertices don't have to be connected!
FaceTools_EXPORT float calcLength( const r3d::Mesh*, const std::vector<int>& vertices);

// Get N equidistant points into q from among vertices in path p starting at entry j. Returns the length of the path p.
// NB path p does not need to be a chain of sequentially connected vertices.
FaceTools_EXPORT float getEquidistant( const r3d::Mesh*, const std::vector<int>& p, int j, int N, std::vector<int>& q);

FaceTools_EXPORT cv::Point2i scale( const cv::Point2i&, float scaleFactor);
FaceTools_EXPORT cv::Point2f scale( const cv::Point2f&, float scaleFactor);
FaceTools_EXPORT cv::Rect scale( const cv::Rect&, float scaleFactor);

// Given fixed integer sizes, convert to and from point and rectangle proportions.
FaceTools_EXPORT cv::Point2i fromProportion( const cv::Point2f&, const cv::Size2i&);
FaceTools_EXPORT cv::Size2i fromProportion( const cv::Size2f&, const cv::Size2i&);
FaceTools_EXPORT cv::Rect_<int> fromProportion( const cv::Rect_<float>&, const cv::Size2i&);
FaceTools_EXPORT cv::RotatedRect fromProportion( const cv::RotatedRect&, const cv::Size2i&);

FaceTools_EXPORT cv::Point2f toProportion( const cv::Point2i&, const cv::Size2i&);
FaceTools_EXPORT cv::Size2f toProportion( const cv::Size2i&, const cv::Size2i&);
FaceTools_EXPORT cv::Rect_<float> toProportion( const cv::Rect_<int>&, const cv::Size2i&);
FaceTools_EXPORT cv::RotatedRect toProportion( const cv::RotatedRect&, const cv::Size2i&);

// Get the vertices/normals from the unique vertex index ids specified.
FaceTools_EXPORT void getVertices( const r3d::Mesh*, const std::vector<int>& uvids, std::vector<Vec3f>& path);

// Replace any black in m with the mean value of m not counting the black (0) pixels
FaceTools_EXPORT cv::Mat_<byte> removeBlackBackground( const cv::Mat_<byte>& m);

FaceTools_EXPORT cv::Rect getUnion( const std::list<cv::Rect>&);
FaceTools_EXPORT cv::Rect getIntersection( const std::list<cv::Rect>&);
FaceTools_EXPORT cv::Rect getMean( const std::list<cv::Rect>&);

// Given two points, use the angle between them and the point equidistant from them
// to define and return a rotated rectangle. The size is defined separately.
FaceTools_EXPORT cv::RotatedRect createRotatedRect( const cv::Point& p0, const cv::Point& p1, const cv::Size& sz);

FaceTools_EXPORT float calcAngleDegs( const cv::Point2f& p0, const cv::Point2f& p1);
FaceTools_EXPORT cv::Mat rotateUpright( const cv::Mat& img, const cv::RotatedRect& rr);

// Find the entry in uvidxs (giving unique vertex indices into omodel) that
// is roughly equidistant from the endpoints of the path. Entries in uvidxs
// do not have to form a connected path over the surface of the model.
FaceTools_EXPORT int findMidway( const r3d::Mesh&, const std::vector<int>& uvidxs);

// Gets the shortest path over the surface of the model between v0 and v1, setting the resulting
// unique vertex IDs in uvidxs, and returning the location of the vector approximately midway along
// this path between the two endpoints v0 and v1.
FaceTools_EXPORT Vec3f getShortestPath( const r3d::Mesh&, int v0, int v1, std::vector<int>& uvidxs);

// Given a 3D triangle identifed by the given three corner points (with base given by v1 - v0)
// derive the direction vector whose magnitude also gives the triangle's height.
FaceTools_EXPORT Vec3f calcDirectionVectorFromBase( const Vec3f& v0, const Vec3f& v1, const Vec3f& apex);

// Find, and return the index of the vertex maximally distant from svidx in the direction of growVec.
FaceTools_EXPORT int growOut( const r3d::Mesh&, const Vec3f& growVec, int svidx);

// Copy contents of vtkIdList (assumed to be integers) to given std::vector.
// Returns the number of elements copied in. Does not clear contents of vector first.
FaceTools_EXPORT int toVector( const vtkSmartPointer<vtkIdList>&, std::vector<int>&);

// Return contents of stream as a front/rear trimmed QString optionally in lowercase.
FaceTools_EXPORT QString getRmLine( std::istringstream&, bool lower=false);
}   // end namespace

#endif
