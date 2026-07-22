// ======================================================================================
// PCDReader - PCD→PCL, LAS→LASlib (支持 LAS 1.2–1.4 + LAZ, 含 Eigen::MatrixXf 同步)
// ======================================================================================

#include "PCDReader.h"
#include <algorithm>
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include "lasreader.hpp"

// ---------- Eigen mirror ----------
static void build_eigen(PCDData& d) {
    int n = (int)d.points.size();
    d.cloud.resize(n, 3);
    for (int i = 0; i < n; ++i) {
        d.cloud(i,0) = (float)d.points[i].x;
        d.cloud(i,1) = (float)d.points[i].y;
        d.cloud(i,2) = (float)d.points[i].z;
    }
}

// ---------- LAS/LAZ reader (via LASlib, auto-detect compression) ----------
static bool read_las(const std::string& f, PCDData& d) {
    LASreadOpener opener;
    opener.set_file_name(f.c_str());
    LASreader* reader = opener.open();
    if (!reader) {
        std::cerr << "LAS/LAZ: failed to open " << f << std::endl;
        return false;
    }

    I64 npoints = reader->npoints;
    d.points.clear();
    d.points.reserve(static_cast<std::size_t>(npoints));

    while (reader->read_point()) {
        double x = reader->point.get_x();
        double y = reader->point.get_y();
        double z = reader->point.get_z();
        d.points.emplace_back(x, y, z);
    }

    build_eigen(d);
    std::cout << "LAS/LAZ: loaded " << d.points.size()
              << " pts from " << f << std::endl;

    reader->close();
    delete reader;
    return !d.points.empty();
}

// ---------- PCD reader (PCL) ----------
static bool read_pcd_pcl(const std::string& f, PCDData& d) {
    pcl::PointCloud<pcl::PointXYZ> c;
    if (pcl::io::loadPCDFile(f, c) < 0) return false;
    d.points.clear(); d.points.reserve(c.size());
    for (auto& p : c) d.points.emplace_back(p.x, p.y, p.z);
    build_eigen(d);
    std::cout << "PCD: loaded " << d.points.size() << " pts from " << f << std::endl;
    return !d.points.empty();
}

// ---------- public ----------
bool read_pcd(const std::string& f, PCDData& d) {
    auto dot = f.rfind('.'); if (dot == std::string::npos) return false;
    std::string ext; for (size_t i = dot; i < f.size(); ++i) ext += (char)std::tolower((unsigned char)f[i]);
    if (ext == ".las" || ext == ".laz") return read_las(f, d);
    return read_pcd_pcl(f, d);
}
