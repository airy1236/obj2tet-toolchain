#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <stdexcept>

using Vec3d = std::vector<double>;

bool ConvertTetgenToCustomTet(const std::string& node_path, 
                              const std::string& ele_path, 
                              const std::string& tet_path,
                              bool zero_based = true) {
    std::vector<Vec3d> vertices;
    std::ifstream node_file(node_path);
    if (!node_file.is_open()) {
        throw std::runtime_error("Failed to open .node file: " + node_path);
        return false;
    }

    int num_nodes, dim, num_bdry_markers, num_attrs;
    node_file >> num_nodes >> dim >> num_bdry_markers >> num_attrs;
    if (dim != 3) {
        throw std::runtime_error(".node file dimension error, only 3D mesh (dim=3) is supported");
        return false;
    }

    for (int i = 0; i < num_nodes; ++i) {
        int node_id;
        double x, y, z;
        node_file >> node_id >> x >> y >> z;
        vertices.push_back({x, y, z});
        if (num_bdry_markers > 0) {
            int bdry_marker;
            node_file >> bdry_marker;
        }
        for (int a = 0; a < num_attrs; ++a) {
            double attr;
            node_file >> attr;
        }
    }
    node_file.close();
    std::cout << "Parsed .node file: total " << vertices.size() << " vertices" << std::endl;

    std::vector<std::vector<int>> tetrahedrons;
    std::ifstream ele_file(ele_path);
    if (!ele_file.is_open()) {
        throw std::runtime_error("Failed to open .ele file: " + ele_path);
        return false;
    }

    int num_tets, num_vtx_per_tet, tet_attrs;
    ele_file >> num_tets >> num_vtx_per_tet >> tet_attrs;
    if (num_vtx_per_tet != 4) {
        throw std::runtime_error(".ele file error, each tetrahedron must have 4 vertices");
        return false;
    }

    int index_offset = zero_based ? 0 : 1;
    for (int i = 0; i < num_tets; ++i) {
        int tet_id, v1, v2, v3, v4;
        ele_file >> tet_id >> v1 >> v2 >> v3 >> v4;
        tetrahedrons.push_back({
            v1 - index_offset,
            v2 - index_offset,
            v3 - index_offset,
            v4 - index_offset
        });
        for (int a = 0; a < tet_attrs; ++a) {
            double attr;
            ele_file >> attr;
        }
    }
    ele_file.close();
    std::cout << "Parsed .ele file: total " << tetrahedrons.size() << " tetrahedrons" << std::endl;
    std::cout << "Indexing mode: " << (zero_based ? "0-based" : "1-based") << std::endl;

    std::ofstream tet_file(tet_path);
    if (!tet_file.is_open()) {
        throw std::runtime_error("Failed to create .tet file: " + tet_path);
        return false;
    }

    tet_file << std::fixed << std::setprecision(6);

    for (const auto& v : vertices) {
        tet_file << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
    }

    for (const auto& t : tetrahedrons) {
        tet_file << "t " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << std::endl;
    }

    tet_file.close();
    std::cout << "Conversion completed! .tet file generated at " << tet_path << std::endl;

    return true;
}

int main(int argc, char* argv[]) {
    bool zero_based = true;
    int arg_offset = 0;

    if (argc == 5) {
        std::string index_option = argv[1];
        if (index_option == "-0") {
            zero_based = true;
            arg_offset = 1;
        } else if (index_option == "-1") {
            zero_based = false;
            arg_offset = 1;
        } else {
            std::cerr << "Invalid index option: " << index_option << std::endl;
            std::cerr << "Supported options: -0 (0-based index), -1 (1-based index)" << std::endl;
            return 1;
        }
    } else if (argc != 4) {
        std::cerr << "Invalid usage!" << std::endl;
        std::cerr << "Format 1 (default 0-based): " << argv[0] << " input.node input.ele output.tet" << std::endl;
        std::cerr << "Format 2 (custom index): " << argv[0] << " [-0|-1] input.node input.ele output.tet" << std::endl;
        return 1;
    }

    try {
        std::string node_path = argv[1 + arg_offset];
        std::string ele_path = argv[2 + arg_offset];
        std::string tet_path = argv[3 + arg_offset];
        ConvertTetgenToCustomTet(node_path, ele_path, tet_path, zero_based);
    } catch (const std::exception& e) {
        std::cerr << "Conversion failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}