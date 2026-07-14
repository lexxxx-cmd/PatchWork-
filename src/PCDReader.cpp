// ======================================================================================
// PCDReader - Lightweight ASCII + binary PCD file reader
// ======================================================================================

#include "PCDReader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

static int field_offset(int field_idx,
                        const std::vector<int>& sizes,
                        const std::vector<int>& counts) {
    int off = 0;
    for (int i = 0; i < field_idx; ++i) {
        int cnt = (i < static_cast<int>(counts.size())) ? counts[i] : 1;
        off += sizes[i] * cnt;
    }
    return off;
}

static float read_float32(const char* buf, int off) {
    float val;
    std::memcpy(&val, buf + off, sizeof(float));
    return val;
}

static double read_float64(const char* buf, int off) {
    double val;
    std::memcpy(&val, buf + off, sizeof(double));
    return val;
}

static std::int32_t read_int32(const char* buf, int off) {
    std::int32_t val;
    std::memcpy(&val, buf + off, sizeof(std::int32_t));
    return val;
}

static std::uint32_t read_uint32(const char* buf, int off) {
    std::uint32_t val;
    std::memcpy(&val, buf + off, sizeof(std::uint32_t));
    return val;
}

struct PCDHeader {
    std::string version;
    std::vector<std::string> fields;
    std::vector<int> sizes;
    std::vector<char> types;
    std::vector<int> counts;
    int width = 0, height = 0, points = 0;
    std::string data_type;
};

static bool parse_pcd_header(std::ifstream& fin, PCDHeader& hdr) {
    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        std::istringstream iss(trimmed);
        std::string keyword;
        iss >> keyword;
        if (keyword == "VERSION")      iss >> hdr.version;
        else if (keyword == "FIELDS")  { std::string f; while (iss >> f) hdr.fields.push_back(f); }
        else if (keyword == "SIZE")    { int s; while (iss >> s) hdr.sizes.push_back(s); }
        else if (keyword == "TYPE")    { char t; while (iss >> t) hdr.types.push_back(t); }
        else if (keyword == "COUNT")   { int c; while (iss >> c) hdr.counts.push_back(c); }
        else if (keyword == "WIDTH")   iss >> hdr.width;
        else if (keyword == "HEIGHT")  iss >> hdr.height;
        else if (keyword == "POINTS")  iss >> hdr.points;
        else if (keyword == "DATA")    { iss >> hdr.data_type; return true; }
    }
    return false;
}

static bool find_xyz_indices(const std::vector<std::string>& fields,
                              int& x_idx, int& y_idx, int& z_idx) {
    x_idx = y_idx = z_idx = -1;
    for (size_t i = 0; i < fields.size(); ++i) {
        std::string f_lower = fields[i];
        std::transform(f_lower.begin(), f_lower.end(), f_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (f_lower == "x")      x_idx = static_cast<int>(i);
        else if (f_lower == "y") y_idx = static_cast<int>(i);
        else if (f_lower == "z") z_idx = static_cast<int>(i);
    }
    if (x_idx < 0 || y_idx < 0 || z_idx < 0) {
        std::cerr << "PCDReader: PCD must contain x, y, z fields." << std::endl;
        return false;
    }
    return true;
}

// --- ASCII reader ---

static bool read_pcd_ascii(std::ifstream& fin, const PCDHeader& hdr,
                            int total, int xi, int yi, int zi,
                            std::vector<Point3D>& pts) {
    int nf = static_cast<int>(hdr.fields.size());
    pts.clear(); pts.reserve(total);
    std::string line;
    int cnt = 0;
    while (cnt < total && std::getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;
        std::istringstream iss(trimmed);
        std::vector<std::string> tokens;
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        if (static_cast<int>(tokens.size()) < nf) continue;
        pts.emplace_back(atof(tokens[xi].c_str()),
                         atof(tokens[yi].c_str()),
                         atof(tokens[zi].c_str()));
        ++cnt;
    }
    return true;
}

// --- Binary reader ---

static bool read_pcd_binary(std::ifstream& fin, const PCDHeader& hdr,
                             int total, int xi, int yi, int zi,
                             std::vector<Point3D>& pts) {
    pts.clear(); pts.reserve(total);
    int stride = 0;
    for (size_t i = 0; i < hdr.fields.size(); ++i) {
        int cnt = (i < hdr.counts.size()) ? hdr.counts[i] : 1;
        stride += hdr.sizes[i] * cnt;
    }
    std::vector<char> buf(stride);
    int x_off = field_offset(xi, hdr.sizes, hdr.counts);
    int y_off = field_offset(yi, hdr.sizes, hdr.counts);
    int z_off = field_offset(zi, hdr.sizes, hdr.counts);
    char xt = hdr.types[xi], yt = hdr.types[yi], zt = hdr.types[zi];
    int xs = hdr.sizes[xi], ys = hdr.sizes[yi], zs = hdr.sizes[zi];

    auto read_val = [](char type, int size, const char* ptr) -> double {
        switch (type) {
        case 'F':
            if (size == 4) return static_cast<double>(read_float32(ptr, 0));
            if (size == 8) return read_float64(ptr, 0);
            break;
        case 'I':
            if (size == 1) return static_cast<double>(static_cast<std::int8_t>(ptr[0]));
            if (size == 2) { std::int16_t v; std::memcpy(&v, ptr, 2); return static_cast<double>(v); }
            if (size == 4) return static_cast<double>(read_int32(ptr, 0));
            if (size == 8) { std::int64_t v; std::memcpy(&v, ptr, 8); return static_cast<double>(v); }
            break;
        case 'U':
            if (size == 1) return static_cast<double>(static_cast<std::uint8_t>(ptr[0]));
            if (size == 2) { std::uint16_t v; std::memcpy(&v, ptr, 2); return static_cast<double>(v); }
            if (size == 4) return static_cast<double>(read_uint32(ptr, 0));
            if (size == 8) { std::uint64_t v; std::memcpy(&v, ptr, 8); return static_cast<double>(v); }
            break;
        default: break;
        }
        return 0.0;
    };

    for (int i = 0; i < total; ++i) {
        fin.read(buf.data(), stride);
        if (fin.gcount() < stride) {
            std::cerr << "PCDReader: unexpected EOF at point " << i << std::endl;
            break;
        }
        pts.emplace_back(read_val(xt, xs, buf.data() + x_off),
                         read_val(yt, ys, buf.data() + y_off),
                         read_val(zt, zs, buf.data() + z_off));
    }
    return true;
}

// --- Public entry point ---

bool read_pcd(const std::string& fname, PCDData& data) {
    std::ifstream fin(fname.c_str(), std::ios::in | std::ios::binary);
    if (!fin.is_open()) {
        std::cerr << "PCDReader: cannot open: " << fname << std::endl;
        return false;
    }
    PCDHeader hdr;
    if (!parse_pcd_header(fin, hdr)) {
        std::cerr << "PCDReader: incomplete header" << std::endl;
        return false;
    }
    if (hdr.data_type != "ascii" && hdr.data_type != "binary") {
        std::cerr << "PCDReader: unsupported DATA '" << hdr.data_type << "'" << std::endl;
        return false;
    }
    int total = hdr.points;
    if (total <= 0) total = hdr.width * hdr.height;
    if (total <= 0) {
        std::cerr << "PCDReader: cannot determine point count" << std::endl;
        return false;
    }
    int xi, yi, zi;
    if (!find_xyz_indices(hdr.fields, xi, yi, zi)) return false;

    bool ok;
    if (hdr.data_type == "ascii")
        ok = read_pcd_ascii(fin, hdr, total, xi, yi, zi, data.points);
    else
        ok = read_pcd_binary(fin, hdr, total, xi, yi, zi, data.points);

    if (ok) {
        int n = static_cast<int>(data.points.size());
        data.cloud.resize(n, 3);
        for (int i = 0; i < n; ++i) {
            data.cloud(i, 0) = static_cast<float>(data.points[i].x);
            data.cloud(i, 1) = static_cast<float>(data.points[i].y);
            data.cloud(i, 2) = static_cast<float>(data.points[i].z);
        }
        std::cout << "PCDReader: loaded " << n << " points (" << hdr.data_type << ")" << std::endl;
    }
    return ok;
}
