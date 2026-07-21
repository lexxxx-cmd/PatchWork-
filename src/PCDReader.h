// ======================================================================================
// PCDReader — 基于 PCL 的 PCD/LAS 读取器
//   支持 .pcd (通过 PCL) 和 .las/.laz (通过内建解析器)
// ======================================================================================

#ifndef PCD_READER_H_
#define PCD_READER_H_

#include <string>
#include <vector>
#include <Eigen/Dense>

struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

struct PCDData {
    std::vector<Point3D> points;
    Eigen::MatrixXf cloud;   // N×3 matrix (for Patchwork/Patchwork++ algorithms)
};

/// 读取点云文件（自动识别扩展名: .pcd → PCL, .las/.laz → 内建解析器）
/// 返回 true 成功, false 失败
bool read_pcd(const std::string& fname, PCDData& data);

#endif
