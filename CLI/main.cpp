// ======================================================================================
// Patchwork / Patchwork++ CLI — PCD input → LAS output
// Usage: patchwork_cli --input <pcd> --output <las> [--algo patchwork|patchworkpp]
// ======================================================================================

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "LASWriter.h"
#include "PCDReader.h"
#include "patchwork/patchwork.h"
#include "patchwork/patchworkpp.h"

static void print_usage() {
    std::cout <<
        "Usage: patchwork_cli --input <pcd> --output <las> [--algo patchwork|patchworkpp]\n"
        "  --input   Input PCD file (ascii or binary)\n"
        "  --output  Output LAS file\n"
        "  --algo    Algorithm: patchwork (classic) or patchworkpp (default)\n"
        "  --sensor_height  Sensor height in meters (default: 1.723)\n";
}

int main(int argc, char* argv[]) {
    std::string input, output, algo = "patchworkpp";
    double sensor_height = 1.723;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        else if (strcmp(argv[i], "--algo") == 0 && i + 1 < argc)
            algo = argv[++i];
        else if (strcmp(argv[i], "--sensor_height") == 0 && i + 1 < argc)
            sensor_height = atof(argv[++i]);
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(); return 0;
        }
    }

    if (input.empty() || output.empty()) { print_usage(); return 1; }

    // 1. Read PCD
    PCDData data;
    if (!read_pcd(input, data)) {
        std::cerr << "Failed to read PCD: " << input << std::endl;
        return 1;
    }
    std::cout << "Loaded " << data.points.size() << " points" << std::endl;

    // 2. Run algorithm
    std::vector<int> ground_idx;
    double time_ms = 0.0;

    if (algo == "patchwork") {
        patchwork::PatchworkParams params;
        params.sensor_height = sensor_height;
        patchwork::PatchWork pw(params);

        auto t0 = std::chrono::high_resolution_clock::now();
        pw.estimateGround(data.cloud);
        auto t1 = std::chrono::high_resolution_clock::now();

        ground_idx = pw.getGroundIndices();
        time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    } else {
        patchwork::Params params;
        params.sensor_height = sensor_height;
        patchwork::PatchWorkpp pw(params);

        auto t0 = std::chrono::high_resolution_clock::now();
        pw.estimateGround(data.cloud);
        auto t1 = std::chrono::high_resolution_clock::now();

        Eigen::VectorXi idx = pw.getGroundIndices();
        ground_idx.assign(idx.data(), idx.data() + idx.size());
        time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    std::cout << "Ground: " << ground_idx.size()
              << " / Non-ground: " << (data.points.size() - ground_idx.size())
              << "  Time: " << time_ms << " ms" << std::endl;

    // 3. Write LAS
    if (!write_las(output, data.points, ground_idx)) {
        std::cerr << "Failed to write LAS: " << output << std::endl;
        return 1;
    }

    // 4. Write runtime.json
    std::string rt_path = output;
    size_t ext = rt_path.rfind(".las");
    if (ext == std::string::npos) ext = rt_path.rfind(".LAS");
    if (ext != std::string::npos)
        rt_path.replace(ext, 4, "_runtime.json");
    else
        rt_path += "_runtime.json";

    std::ofstream rt(rt_path);
    rt << "{\"infer_time_ms\": " << time_ms << "}" << std::endl;
    rt.close();

    std::cout << "Done. Output: " << output << ", " << rt_path << std::endl;
    return 0;
}
