// ======================================================================================
// LASWriter - Minimal LAS 1.2 writer
// ======================================================================================

#include "LASWriter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

#pragma pack(push, 1)

struct LASHeader12 {
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

static double clamp_scaled(double val, double scale, double offset) {
    double s = (val - offset) / scale;
    if (s > static_cast<double>(INT32_MAX)) return static_cast<double>(INT32_MAX);
    if (s < static_cast<double>(INT32_MIN)) return static_cast<double>(INT32_MIN);
    return s;
}

static void set_hdr_str(char* dst, size_t n, const std::string& src) {
    std::memset(dst, 0, n);
    std::strncpy(dst, src.c_str(), n - 1);
}

bool write_las(const std::string& fname,
               const std::vector<Point3D>& points,
               const std::vector<int>& ground_idx) {

    std::ofstream out(fname.c_str(), std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "LASWriter: cannot open: " << fname << std::endl;
        return false;
    }
    int n = static_cast<int>(points.size());
    if (n == 0) { std::cerr << "LASWriter: no points" << std::endl; return false; }

    // Classification
    std::vector<uint8_t> cls(n, 1);  // default unclassified
    for (int idx : ground_idx)
        if (idx >= 0 && idx < n) cls[idx] = 2;

    // Bounding box
    double minX = std::numeric_limits<double>::max(), maxX = -minX;
    double minY = minX, maxY = -minY;
    double minZ = minX, maxZ = -minZ;
    for (const auto& p : points) {
        if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
        if (p.z < minZ) minZ = p.z; if (p.z > maxZ) maxZ = p.z;
    }

    double sx = (maxX - minX) / (INT32_MAX - 1.0);
    double sy = (maxY - minY) / (INT32_MAX - 1.0);
    double sz = (maxZ - minZ) / (INT32_MAX - 1.0);
    if (sx < 1e-16) sx = 0.001;
    if (sy < 1e-16) sy = 0.001;
    if (sz < 1e-16) sz = 0.001;

    LASHeader12 hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.fileSignature[0] = 'L'; hdr.fileSignature[1] = 'A';
    hdr.fileSignature[2] = 'S'; hdr.fileSignature[3] = 'F';
    hdr.versionMajor = 1; hdr.versionMinor = 2;
    set_hdr_str(hdr.systemId, sizeof(hdr.systemId), "Patchwork++");
    set_hdr_str(hdr.generatingSoftware, sizeof(hdr.generatingSoftware), "Patchwork++ CLI");
    hdr.creationDayOfYear = 1; hdr.creationYear = 2026;
    hdr.headerSize = sizeof(LASHeader12);
    hdr.offsetToPointData = sizeof(LASHeader12);
    hdr.numVLRs = 0; hdr.pointDataFormat = 0;
    hdr.pointDataRecordLen = sizeof(LASPointRecord0);
    hdr.legacyNumPoints = n;
    hdr.legacyNumPointsByReturn[0] = n;
    hdr.xScale = sx; hdr.yScale = sy; hdr.zScale = sz;
    hdr.xOffset = minX; hdr.yOffset = minY; hdr.zOffset = minZ;
    hdr.maxX = maxX; hdr.minX = minX;
    hdr.maxY = maxY; hdr.minY = minY;
    hdr.maxZ = maxZ; hdr.minZ = minZ;
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    for (int i = 0; i < n; ++i) {
        LASPointRecord0 rec;
        std::memset(&rec, 0, sizeof(rec));
        rec.x = static_cast<int32_t>(std::llround(clamp_scaled(points[i].x, sx, minX)));
        rec.y = static_cast<int32_t>(std::llround(clamp_scaled(points[i].y, sy, minY)));
        rec.z = static_cast<int32_t>(std::llround(clamp_scaled(points[i].z, sz, minZ)));
        rec.returnByte = 0x01;
        rec.classification = cls[i];
        out.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
    }
    out.close();
    std::cout << "LASWriter: wrote " << n << " points to " << fname << std::endl;
    return true;
}
