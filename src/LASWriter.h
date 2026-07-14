// ======================================================================================
// LASWriter - Minimal LAS 1.2 writer (Format 0, identity coordinates)
// ======================================================================================

#ifndef LAS_WRITER_H_
#define LAS_WRITER_H_

#include <string>
#include <vector>
#include "PCDReader.h"  // Point3D

/// Write LAS 1.2 with classification: 2=ground, 1=unclassified.
/// @param points      All points in original PCD coordinates
/// @param ground_idx  Indices of ground-classified points
bool write_las(const std::string& fname,
               const std::vector<Point3D>& points,
               const std::vector<int>& ground_idx);

#endif
