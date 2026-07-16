// ======================================================================================
// Patchwork / Patchwork++ CLI — PCD input → LAS output
// Usage: patchwork_cli --input <pcd> --output <las> [--config <config.yaml>]
// ======================================================================================

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "LASWriter.h"
#include "PCDReader.h"
#include "patchwork/patchwork.h"
#include "patchwork/patchworkpp.h"

// Helper: fill patchwork::Params from a YAML::Node
static void fill_pp_params(patchwork::Params& p, const YAML::Node& node) {
    if (node["verbose"])                     p.verbose                     = node["verbose"].as<bool>();
    if (node["enable_RNR"])                  p.enable_RNR                  = node["enable_RNR"].as<bool>();
    if (node["enable_RVPF"])                 p.enable_RVPF                 = node["enable_RVPF"].as<bool>();
    if (node["enable_TGR"])                  p.enable_TGR                  = node["enable_TGR"].as<bool>();
    if (node["num_iter"])                    p.num_iter                    = node["num_iter"].as<int>();
    if (node["num_lpr"])                     p.num_lpr                     = node["num_lpr"].as<int>();
    if (node["num_min_pts"])                 p.num_min_pts                 = node["num_min_pts"].as<int>();
    if (node["num_zones"])                   p.num_zones                   = node["num_zones"].as<int>();
    if (node["num_rings_of_interest"])       p.num_rings_of_interest       = node["num_rings_of_interest"].as<int>();
    if (node["RNR_ver_angle_thr"])           p.RNR_ver_angle_thr           = node["RNR_ver_angle_thr"].as<double>();
    if (node["RNR_intensity_thr"])           p.RNR_intensity_thr           = node["RNR_intensity_thr"].as<double>();
    if (node["sensor_height"])               p.sensor_height               = node["sensor_height"].as<double>();
    if (node["th_seeds"])                    p.th_seeds                    = node["th_seeds"].as<double>();
    if (node["th_dist"])                     p.th_dist                     = node["th_dist"].as<double>();
    if (node["th_seeds_v"])                  p.th_seeds_v                  = node["th_seeds_v"].as<double>();
    if (node["th_dist_v"])                   p.th_dist_v                   = node["th_dist_v"].as<double>();
    if (node["max_range"])                   p.max_range                   = node["max_range"].as<double>();
    if (node["min_range"])                   p.min_range                   = node["min_range"].as<double>();
    if (node["uprightness_thr"])             p.uprightness_thr             = node["uprightness_thr"].as<double>();
    if (node["adaptive_seed_selection_margin"]) p.adaptive_seed_selection_margin = node["adaptive_seed_selection_margin"].as<double>();
    if (node["intensity_thr"])               p.intensity_thr               = node["intensity_thr"].as<double>();
    if (node["num_sectors_each_zone"])       p.num_sectors_each_zone       = node["num_sectors_each_zone"].as<std::vector<int>>();
    if (node["num_rings_each_zone"])         p.num_rings_each_zone         = node["num_rings_each_zone"].as<std::vector<int>>();
    if (node["max_flatness_storage"])        p.max_flatness_storage        = node["max_flatness_storage"].as<int>();
    if (node["max_elevation_storage"])       p.max_elevation_storage       = node["max_elevation_storage"].as<int>();
    if (node["elevation_thr"])               p.elevation_thr               = node["elevation_thr"].as<std::vector<double>>();
    if (node["flatness_thr"])                p.flatness_thr                = node["flatness_thr"].as<std::vector<double>>();
}

// Helper: fill patchwork::PatchworkParams from a YAML::Node
static void fill_classic_params(patchwork::PatchworkParams& p, const YAML::Node& node) {
    if (node["verbose"])                     p.verbose                     = node["verbose"].as<bool>();
    if (node["max_range"])                   p.max_range                   = node["max_range"].as<double>();
    if (node["min_range"])                   p.min_range                   = node["min_range"].as<double>();
    if (node["num_zones"])                   p.num_zones                   = node["num_zones"].as<int>();
    if (node["num_sectors_each_zone"])       p.num_sectors_each_zone       = node["num_sectors_each_zone"].as<std::vector<int>>();
    if (node["num_rings_each_zone"])         p.num_rings_each_zone         = node["num_rings_each_zone"].as<std::vector<int>>();
    if (node["min_ranges"])                  p.min_ranges                  = node["min_ranges"].as<std::vector<double>>();
    if (node["num_iter"])                    p.num_iter                    = node["num_iter"].as<int>();
    if (node["num_lpr"])                     p.num_lpr                     = node["num_lpr"].as<int>();
    if (node["num_min_pts"])                 p.num_min_pts                 = node["num_min_pts"].as<int>();
    if (node["th_seeds"])                    p.th_seeds                    = node["th_seeds"].as<double>();
    if (node["th_dist"])                     p.th_dist                     = node["th_dist"].as<double>();
    if (node["uprightness_thr"])             p.uprightness_thr             = node["uprightness_thr"].as<double>();
    if (node["elevation_thr"])               p.elevation_thr               = node["elevation_thr"].as<std::vector<double>>();
    if (node["flatness_thr"])                p.flatness_thr                = node["flatness_thr"].as<std::vector<double>>();
    if (node["adaptive_seed_selection_margin"]) p.adaptive_seed_selection_margin = node["adaptive_seed_selection_margin"].as<double>();
    if (node["using_global_thr"])            p.using_global_thr            = node["using_global_thr"].as<bool>();
    if (node["global_elevation_thr"])        p.global_elevation_thr        = node["global_elevation_thr"].as<double>();
    if (node["ATAT_ON"])                     p.ATAT_ON                     = node["ATAT_ON"].as<bool>();
    if (node["max_h_for_ATAT"])              p.max_h_for_ATAT              = node["max_h_for_ATAT"].as<double>();
    if (node["num_sectors_for_ATAT"])        p.num_sectors_for_ATAT        = node["num_sectors_for_ATAT"].as<int>();
    if (node["noise_bound"])                 p.noise_bound                 = node["noise_bound"].as<double>();
}

static void print_usage() {
    std::cout <<
        "Usage: patchwork_cli --input <pcd> --output <las> [--config <config.yaml>]\n"
        "  --input   Input PCD file (ascii or binary)\n"
        "  --output  Output LAS file\n"
        "  --config  YAML config file (default: built-in defaults)\n";
}

int main(int argc, char* argv[]) {
    std::string input, output, config_path;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            config_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(); return 0;
        }
    }

    if (input.empty() || output.empty()) { print_usage(); return 1; }

    // Determine algorithm variant & load params from YAML if available
    std::string algo = "patchworkpp";
    if (!config_path.empty()) {
        try {
            YAML::Node cfg = YAML::LoadFile(config_path);
            if (cfg["algo"])
                algo = cfg["algo"].as<std::string>();
        } catch (const YAML::Exception& e) {
            std::cerr << "Error: failed to parse YAML config (" << config_path
                      << "): " << e.what() << std::endl;
            return 1;
        }
    }

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
        if (!config_path.empty()) {
            try {
                YAML::Node cfg = YAML::LoadFile(config_path);
                // Common params (top-level)
                if (cfg["sensor_height"])
                    params.sensor_height = cfg["sensor_height"].as<double>();
                // Variant-specific params
                if (cfg["patchwork"])
                    fill_classic_params(params, cfg["patchwork"]);
                std::cout << "Patchwork classic params loaded from: " << config_path << std::endl;
            } catch (const YAML::Exception& e) {
                std::cerr << "Error: YAML parse failed: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cout << "Using default Patchwork classic parameters." << std::endl;
        }

        patchwork::PatchWork pw(params);

        auto t0 = std::chrono::high_resolution_clock::now();
        pw.estimateGround(data.cloud);
        auto t1 = std::chrono::high_resolution_clock::now();

        ground_idx = pw.getGroundIndices();
        time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    } else {
        patchwork::Params params;
        if (!config_path.empty()) {
            try {
                YAML::Node cfg = YAML::LoadFile(config_path);
                // Common params (top-level)
                if (cfg["sensor_height"])
                    params.sensor_height = cfg["sensor_height"].as<double>();
                // Variant-specific params
                if (cfg["patchworkpp"])
                    fill_pp_params(params, cfg["patchworkpp"]);
                std::cout << "Patchwork++ params loaded from: " << config_path << std::endl;
            } catch (const YAML::Exception& e) {
                std::cerr << "Error: YAML parse failed: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cout << "Using default Patchwork++ parameters." << std::endl;
        }

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
