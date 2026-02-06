#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>   // for system() to call external programs
#include <filesystem> // for path handling (C++17 and above)
#include <sstream>   // for string stream
#include <vector>

// Namespace alias for simplified path operations
namespace fs = std::filesystem;

/**
 * @brief Check if a file exists
 * @param file_path Path to the file
 * @return true if the file exists, false otherwise
 */
bool FileExists(const std::string& file_path) {
    return fs::exists(file_path) && fs::is_regular_file(file_path);
}

/**
 * @brief Rename a file
 * @param old_path Original file path
 * @param new_path New file path
 * @param step_desc Step description (for logging)
 * @return true on success, false on failure
 */
bool RenameFile(const std::string& old_path, const std::string& new_path, 
                const std::string& step_desc) {
    std::cout << "\n[Step] " << step_desc << std::endl;
    std::cout << "Renaming file: " << old_path << " -> " << new_path << std::endl;
    
    try {
        fs::rename(old_path, new_path);
        std::cout << "[Success] " << step_desc << " completed!" << std::endl;
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[Error] " << step_desc << " failed! Reason: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Execute an external command and check its result
 * @param cmd Command string to execute
 * @param step_desc Step description (for logging)
 * @return true on success, false on failure
 */
bool ExecuteCommand(const std::string& cmd, const std::string& step_desc) {
    std::cout << "\n[Step] " << step_desc << std::endl;
    std::cout << "Executing command: " << cmd << std::endl;

    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "[Error] " << step_desc << " failed! Return code: " << ret << std::endl;
        return false;
    }
    std::cout << "[Success] " << step_desc << " completed!" << std::endl;
    return true;
}

/**
 * @brief Main pipeline: OBJ → PLY → NODE/ELE → TET
 * @param obj_path Input OBJ file path
 * @param max_tet_volume Maximum tetrahedron volume (default: 0.001)
 * @param keep_intermediate Whether to keep intermediate files
 * @return true if the entire pipeline succeeds, false otherwise
 */
bool ObjToTet(const std::string& obj_path, double max_tet_volume = 0.001, 
              bool keep_intermediate = false) {
    // ========== Step 1: Validate input OBJ file ==========
    if (!FileExists(obj_path)) {
        std::cerr << "[Error] Input OBJ file does not exist: " << obj_path << std::endl;
        return false;
    }

    // Extract base path info
    fs::path obj_fs_path(obj_path);
    std::string stem_name = obj_fs_path.stem().string();  // filename without extension
    fs::path parent_dir = obj_fs_path.parent_path();
    if (parent_dir.empty()) {
        parent_dir = ".";
    }
    
    // ========== Step 2: Construct PLY output path (same dir and name as OBJ) ==========
    std::string ply_path = (parent_dir / (stem_name + ".ply")).string();
    
    // Handle paths with spaces: wrap in quotes
    std::string quoted_obj_path = "\"" + obj_path + "\"";
    std::string quoted_ply_path = "\"" + ply_path + "\"";

    // ========== Step 3: Call obj2ply.exe to convert OBJ to PLY ==========
    std::string obj2ply_cmd = "obj2ply " + quoted_obj_path + " " + quoted_ply_path;
    if (!ExecuteCommand(obj2ply_cmd, "OBJ to PLY conversion")) {
        return false;
    }
    if (!FileExists(ply_path)) {
        std::cerr << "[Error] PLY file was not generated after obj2ply: " << ply_path << std::endl;
        return false;
    }

    // ========== Step 4: Call tetgen.exe to generate NODE/ELE files ==========
    std::stringstream tetgen_ss;
    tetgen_ss << "tetgen1.5.1\\" << "tetgen -pqO -a" << max_tet_volume << " " << quoted_ply_path;
    std::string tetgen_cmd = tetgen_ss.str();
    if (!ExecuteCommand(tetgen_cmd, "Generating NODE/ELE from PLY")) {
        return false;
    }

    // ========== Step 5: Rename .1.node/.1.ele/.1.face/.1.edge/.1.smesh ==========
    std::string node_with_1 = (parent_dir / (stem_name + ".1.node")).string();
    std::string ele_with_1 = (parent_dir / (stem_name + ".1.ele")).string();
    std::string face_with_1 = (parent_dir / (stem_name + ".1.face")).string();
    std::string edge_with_1 = (parent_dir / (stem_name + ".1.edge")).string();
    std::string smesh_with_1 = (parent_dir / (stem_name + ".1.smesh")).string();
    std::string node_without_1 = (parent_dir / (stem_name + ".node")).string();
    std::string ele_without_1 = (parent_dir / (stem_name + ".ele")).string();
    std::string face_without_1 = (parent_dir / (stem_name + ".face")).string();
    std::string edge_without_1 = (parent_dir / (stem_name + ".edge")).string();
    std::string smesh_without_1 = (parent_dir / (stem_name + ".smesh")).string();
    
    // Verify that all expected .1.* files exist
    if (!FileExists(node_with_1) || !FileExists(ele_with_1) || !FileExists(face_with_1) || 
        !FileExists(edge_with_1) || !FileExists(smesh_with_1)) {
        std::cerr << "[Error] tetgen did not generate required .1.* files:" << std::endl;
        if (!FileExists(node_with_1)) std::cerr << "  - Missing: " << node_with_1 << std::endl;
        if (!FileExists(ele_with_1)) std::cerr << "  - Missing: " << ele_with_1 << std::endl;
        if (!FileExists(face_with_1)) std::cerr << "  - Missing: " << face_with_1 << std::endl;
        if (!FileExists(edge_with_1)) std::cerr << "  - Missing: " << edge_with_1 << std::endl;
        if (!FileExists(smesh_with_1)) std::cerr << "  - Missing: " << smesh_with_1 << std::endl;
        return false;
    }
    
    // Rename files
    if (!RenameFile(node_with_1, node_without_1, "Renaming .1.node to .node")) {
        return false;
    }
    
    if (!RenameFile(ele_with_1, ele_without_1, "Renaming .1.ele to .ele")) {
        if (FileExists(node_without_1) && !FileExists(node_with_1)) {
            fs::rename(node_without_1, node_with_1);
        }
        return false;
    }

    if (!RenameFile(face_with_1, face_without_1, "Renaming .1.face to .face")) {
        if (FileExists(ele_without_1) && !FileExists(ele_with_1)) {
            fs::rename(ele_without_1, ele_with_1);
        }
        return false;
    }

    if (!RenameFile(edge_with_1, edge_without_1, "Renaming .1.edge to .edge")) {
        if (FileExists(face_without_1) && !FileExists(face_with_1)) {
            fs::rename(face_without_1, face_with_1);
        }
        return false;
    }

    if (!RenameFile(smesh_with_1, smesh_without_1, "Renaming .1.smesh to .smesh")) {
        if (FileExists(edge_without_1) && !FileExists(edge_with_1)) {
            fs::rename(edge_without_1, edge_with_1);
        }
        return false;
    }
    
    // ========== Step 6: Call nodele2tet.exe to generate .tet file ==========
    std::string tet_output_path = (parent_dir / (stem_name + ".tet")).string();
    std::string quoted_tet_output = "\"" + tet_output_path + "\"";
    std::string quoted_stem_name = "\"" + (parent_dir / stem_name).string() + "\"";
    
    std::string nodele2tet_cmd = "nodele2tet -0 " + quoted_stem_name + ".node " + quoted_stem_name + ".ele " + quoted_tet_output;
    if (!ExecuteCommand(nodele2tet_cmd, "Merging NODE/ELE into TET")) {
        return false;
    }
    
    if (!FileExists(tet_output_path)) {
        std::cerr << "[Error] .tet file was not generated after nodele2tet: " << tet_output_path << std::endl;
        return false;
    }

    // ========== Step 7: Clean up intermediate files (optional) ==========
    if (!keep_intermediate) {
        std::cout << "\n[Cleanup] Removing intermediate files..." << std::endl;
        
        try {
            if (FileExists(ply_path)) {
                fs::remove(ply_path);
                std::cout << "  Deleted: " << ply_path << std::endl;
            }
            
            if (FileExists(node_without_1)) {
                fs::remove(node_without_1);
                std::cout << "  Deleted: " << node_without_1 << std::endl;
            }
            
            if (FileExists(ele_without_1)) {
                fs::remove(ele_without_1);
                std::cout << "  Deleted: " << ele_without_1 << std::endl;
            }
            
            std::vector<std::string> other_files = {
                (parent_dir / (stem_name + ".edge")).string(),
                (parent_dir / (stem_name + ".face")).string(),
                (parent_dir / (stem_name + ".neigh")).string()
            };
            
            for (const auto& file : other_files) {
                if (FileExists(file)) {
                    fs::remove(file);
                    std::cout << "  Deleted: " << file << std::endl;
                }
            }
            
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[Warning] Error during cleanup: " << e.what() << std::endl;
        }
    }

    // ========== Step 8: Output final result ==========
    std::cout << "\n==================== Conversion Completed ====================" << std::endl;
    std::cout << "Input OBJ file: " << obj_path << std::endl;
    std::cout << "Output TET file: " << tet_output_path << std::endl;
    std::cout << "Max tetrahedron volume: " << max_tet_volume << std::endl;
    
    if (keep_intermediate) {
        std::cout << "Intermediate files retained:" << std::endl;
        if (FileExists(ply_path)) std::cout << "  - " << ply_path << std::endl;
        if (FileExists(node_without_1)) std::cout << "  - " << node_without_1 << std::endl;
        if (FileExists(ele_without_1)) std::cout << "  - " << ele_without_1 << std::endl;
    } else {
        std::cout << "Intermediate files cleaned up." << std::endl;
    }
    
    std::cout << "==============================================================" << std::endl;

    return true;
}

// Main function: parse command-line arguments
int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: obj2tet <input_OBJ_file> [max_tet_volume (default: 0.001)] [keep_intermediate (0/1, default: 0)]" << std::endl;
        std::cerr << "Example 1 (default volume, no intermediates): obj2tet bunny_SB.obj" << std::endl;
        std::cerr << "Example 2 (custom volume, no intermediates): obj2tet bunny_SB.obj 0.0005" << std::endl;
        std::cerr << "Example 3 (custom volume, keep intermediates): obj2tet bunny_SB.obj 0.0005 1" << std::endl;
        return 1;
    }

    std::string obj_path = argv[1];
    double max_volume = 0.001;
    bool keep_intermediate = false;
    
    if (argc >= 3) {
        try {
            max_volume = std::stod(argv[2]);
            if (max_volume <= 0) {
                std::cerr << "[Error] Max tetrahedron volume must be greater than 0!" << std::endl;
                return 1;
            }
        } catch (...) {
            std::cerr << "[Error] Max tetrahedron volume must be a valid number!" << std::endl;
            return 1;
        }
    }
    
    if (argc == 4) {
        std::string keep_flag = argv[3];
        if (keep_flag == "1" || keep_flag == "true" || keep_flag == "yes") {
            keep_intermediate = true;
        } else if (keep_flag == "0" || keep_flag == "false" || keep_flag == "no") {
            keep_intermediate = false;
        } else {
            std::cerr << "[Warning] Unrecognized keep flag; using default (do not keep)." << std::endl;
        }
    }

    bool success = ObjToTet(obj_path, max_volume, keep_intermediate);
    return success ? 0 : 1;
}