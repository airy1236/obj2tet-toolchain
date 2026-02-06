import sys
import trimesh
import os

def obj_to_ply(input_path, output_path):
    """
    Convert OBJ file to PLY file
    :param input_path: Input OBJ file path
    :param output_path: Output PLY file path
    :return: True on success, False on failure
    """
    # 1. Verify input file exists
    if not os.path.exists(input_path):
        print(f"Error: Input file does not exist → {input_path}", file=sys.stderr)
        return False
    
    # 2. Validate input file format (simple extension check)
    if not input_path.lower().endswith('.obj'):
        print(f"Error: Input file is not in OBJ format → {input_path}", file=sys.stderr)
        return False
    
    try:
        # 3. Load OBJ mesh (trimesh automatically handles triangulation/non-manifold issues)
        mesh = trimesh.load(input_path, file_type='obj')
        
        # 4. Export to PLY file (trimesh save as ASCII format, compatible with most software)
        mesh.export(output_path, file_type='ply', encoding='ascii')
        
        print(f"Success: Conversion completed → Input: {input_path} → Output: {output_path}")
        return True
    
    except trimesh.loading.LoadError as e:
        print(f"Error: Failed to load OBJ file → {str(e)}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Error: Conversion failed → {str(e)}", file=sys.stderr)
        return False

def main():
    # 1. Validate command line arguments
    if len(sys.argv) != 3:
        print("Usage: obj2ply <input_OBJ_file_path> <output_PLY_file_path>", file=sys.stderr)
        print("Example: obj2ply bunny_SB.obj output.ply", file=sys.stderr)
        sys.exit(1)
    
    # 2. Extract input/output paths
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    # 3. Execute conversion
    if not obj_to_ply(input_file, output_file):
        sys.exit(1)  # Return non-zero exit code on failure for programmatic detection
    sys.exit(0)

if __name__ == "__main__":
    main()