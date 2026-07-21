// ======================================================================================
// PCDReader — 基于 PCL 的 PCD/LAS 读取器实现
// ======================================================================================

#include "PCDReader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>

// ======================================================================
// 内建 LAS 1.2 读取器
// ======================================================================

#pragma pack(push, 1)
struct LAS12Header {
    char     fileSignature[4];
    uint16_t fileSourceId;
    uint16_t globalEncoding;
    uint32_t projectIdGuid1;
    uint16_t projectIdGuid2;
    uint16_t projectIdGuid3;
    char     projectIdGuid4[8];
    uint8_t  versionMajor;
    uint8_t  versionMinor;
    char     systemId[32];
    char     generatingSoftware[32];
    uint16_t creationDayOfYear;
    uint16_t creationYear;
    uint16_t headerSize;
    uint32_t offsetToPointData;
    uint32_t numVLRs;
    uint8_t  pointDataFormat;
    uint16_t pointDataRecordLen;
    uint32_t legacyNumPoints;
    uint32_t legacyNumPointsByReturn[5];
    double   xScale, yScale, zScale;
    double   xOffset, yOffset, zOffset;
    double   maxX, minX, maxY, minY, maxZ, minZ;
};

struct LASPointRecord0 {
    int32_t  x, y, z;
    uint16_t intensity;
    uint8_t  returnByte;
    uint8_t  classification;
    int8_t   scanAngleRank;
    uint8_t  userData;
    uint16_t pointSourceId;
};
#pragma pack(pop)

static bool read_las(const std::string& fname, PCDData& data) {
    std::ifstream in(fname, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "LASReader: cannot open " << fname << std::endl;
        return false;
    }

    LAS12Header hdr;
    in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (in.gcount() < static_cast<std::streamsize>(sizeof(hdr)) ||
        std::strncmp(hdr.fileSignature, "LASF", 4) != 0) {
        std::cerr << "LASReader: invalid or unsupported LAS file" << std::endl;
        return false;
    }

    int n = static_cast<int>(hdr.legacyNumPoints);
    if (n <= 0) {
        std::cerr << "LASReader: zero points in header" << std::endl;
        return false;
    }

    in.seekg(hdr.offsetToPointData);
    int recordLen = (hdr.pointDataRecordLen > 0) ? hdr.pointDataRecordLen : 20;

    data.points.clear();
    data.points.reserve(static_cast<std::size_t>(n));

    std::vector<char> buf(static_cast<std::size_t>(recordLen));
    for (int i = 0; i < n; ++i) {
        in.read(buf.data(), recordLen);
        if (in.gcount() < recordLen) break;

        const LASPointRecord0* rec =
            reinterpret_cast<const LASPointRecord0*>(buf.data());
        double x = static_cast<double>(rec->x) * hdr.xScale + hdr.xOffset;
        double y = static_cast<double>(rec->y) * hdr.yScale + hdr.yOffset;
        double z = static_cast<double>(rec->z) * hdr.zScale + hdr.zOffset;
        data.points.emplace_back(x, y, z);
    }

    // 同步构建 Eigen 矩阵
    std::size_t m = data.points.size();
    data.cloud.resize(static_cast<int>(m), 3);
    for (std::size_t i = 0; i < m; ++i) {
        data.cloud(static_cast<int>(i), 0) = static_cast<float>(data.points[i].x);
        data.cloud(static_cast<int>(i), 1) = static_cast<float>(data.points[i].y);
        data.cloud(static_cast<int>(i), 2) = static_cast<float>(data.points[i].z);
    }

    std::cout << "LASReader: loaded " << m << " points from " << fname << std::endl;
    return !data.points.empty();
}

// ======================================================================
// PCD 读取器（基于 PCL）
// ======================================================================

static bool read_pcd_pcl(const std::string& fname, PCDData& data) {
    pcl::PointCloud<pcl::PointXYZ> cloud;
    if (pcl::io::loadPCDFile(fname, cloud) < 0) {
        std::cerr << "PCDReader: failed to load " << fname << std::endl;
        return false;
    }

    std::size_t n = cloud.size();
    data.points.clear();
    data.points.reserve(n);
    for (const auto& pt : cloud) {
        data.points.emplace_back(static_cast<double>(pt.x),
                                 static_cast<double>(pt.y),
                                 static_cast<double>(pt.z));
    }

    // 同步构建 Eigen 矩阵 (Patchwork 算法需要)
    data.cloud.resize(static_cast<int>(n), 3);
    for (std::size_t i = 0; i < n; ++i) {
        data.cloud(static_cast<int>(i), 0) = static_cast<float>(data.points[i].x);
        data.cloud(static_cast<int>(i), 1) = static_cast<float>(data.points[i].y);
        data.cloud(static_cast<int>(i), 2) = static_cast<float>(data.points[i].z);
    }

    std::cout << "PCDReader(PCL): loaded " << n << " points from " << fname << std::endl;
    return !data.points.empty();
}

// ======================================================================
// 公共入口 — 自动识别扩展名
// ======================================================================

bool read_pcd(const std::string& fname, PCDData& data) {
    std::size_t dot = fname.rfind('.');
    if (dot == std::string::npos) {
        std::cerr << "PCDReader: no extension in filename: " << fname << std::endl;
        return false;
    }
    std::string ext;
    for (std::size_t i = dot; i < fname.size(); ++i)
        ext.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(fname[i]))));

    if (ext == ".las" || ext == ".laz")
        return read_las(fname, data);
    else
        return read_pcd_pcl(fname, data);
}
