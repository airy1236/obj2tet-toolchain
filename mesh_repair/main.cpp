#include <iostream>
#include <vector>

// VCG Core headers for mesh data structures and algorithms
#include <vcg/complex/complex.h>
#include <vcg/complex/algorithms/clean.h>        // Cleaning operations (duplicate removal, etc.)
#include <vcg/complex/algorithms/hole.h>          // Hole detection and filling
#include <vcg/complex/algorithms/update/topology.h> // Topology updates (face‑face adjacency)
#include <vcg/complex/algorithms/update/flag.h>   // Management of mesh element flags
#include <vcg/complex/algorithms/update/normal.h> // Normal computation

// VCG I/O headers for importing OBJ and exporting PLY
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>

using namespace vcg;
using namespace std;

// -------------------------------------------------------------------------
// 1. Definition of the mesh type
//    We define the vertex, edge and face classes and then assemble them
//    into a TriMesh that will hold the data.
// -------------------------------------------------------------------------

// Forward declarations
class MyVertex;
class MyEdge;
class MyFace;

// The UsedTypes struct tells VCG which types are used as vertex/edge/face.
struct MyUsedTypes : public UsedTypes<Use<MyVertex>::AsVertexType,
    Use<MyEdge>::AsEdgeType,
    Use<MyFace>::AsFaceType> {
};

// Vertex: stores 3D coordinates, normal, bit flags and a mark.
class MyVertex : public Vertex<MyUsedTypes,
    vertex::Coord3f,
    vertex::Normal3f,
    vertex::BitFlags,
    vertex::Mark> {
};

// Face: stores references to its three vertices, a normal, flags and
// face‑face adjacency information (FFAdj) needed for topological operations.
class MyFace : public Face<MyUsedTypes,
    face::VertexRef,
    face::Normal3f,
    face::BitFlags,
    face::FFAdj> {
};

// Edge (not heavily used here, but required by the used types).
class MyEdge : public Edge<MyUsedTypes> {};

// The actual mesh type: a container of vertices, faces and edges.
class MyMesh : public tri::TriMesh<vector<MyVertex>,
    vector<MyFace>,
    vector<MyEdge>> {};

// -------------------------------------------------------------------------
// Main program
// -------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " input.obj output.ply" << endl;
        return -1;
    }

    const char* inputPath = argv[1];
    const char* outputPath = argv[2];

    MyMesh m;   // The mesh object we will work on

    // ---------------------------------------------------------------------
    // 2. Load the mesh from an OBJ file
    // ---------------------------------------------------------------------
    if (tri::io::Importer<MyMesh>::Open(m, inputPath) != 0) {
        cerr << "Error: Failed to open file " << inputPath << endl;
        return -1;
    }
    cout << "Loaded mesh: " << m.VN() << " vertices, " << m.FN() << " faces." << endl;

    // ---------------------------------------------------------------------
    // 3. Basic cleaning operations required before hole filling
    // ---------------------------------------------------------------------
    // Remove duplicate vertices (distance tolerance = 0 means exact duplicates)
    int v_dup = tri::Clean<MyMesh>::RemoveDuplicateVertex(m);
    // Remove vertices that are not referenced by any face
    int v_unref = tri::Clean<MyMesh>::RemoveUnreferencedVertex(m);
    // Remove faces that are exactly identical
    int f_dup = tri::Clean<MyMesh>::RemoveDuplicateFace(m);
    // Remove degenerate faces (area zero or two equal vertices)
    int f_deg = tri::Clean<MyMesh>::RemoveDegenerateFace(m);

    cout << "Cleaned: " << v_dup << " dup verts, " << v_unref << " unref verts, "
        << f_dup << " dup faces, " << f_deg << " deg faces." << endl;

    // ---------------------------------------------------------------------
    // 4. Topology pre‑processing
    //    Build face‑face adjacency information. This is required for many
    //    subsequent algorithms (hole detection, normal orientation, etc.).
    // ---------------------------------------------------------------------
    tri::UpdateTopology<MyMesh>::FaceFace(m);

    // ---------------------------------------------------------------------
    // 5. Remove non‑manifold faces and handle self‑intersections
    //    TetGen requires a watertight manifold mesh, so we eliminate
    //    faces that cause non‑manifold edges (edges with >2 incident faces).
    // ---------------------------------------------------------------------
    int f_nm = tri::Clean<MyMesh>::RemoveNonManifoldFace(m);
    // After removal, we must update the topology again
    tri::UpdateTopology<MyMesh>::FaceFace(m);
    cout << "Removed " << f_nm << " non-manifold faces." << endl;

    // ---------------------------------------------------------------------
    // 6. Automatic hole filling
    //    We use the intersection‑aware ear cutting algorithm that also checks
    //    that newly added triangles do not intersect existing ones.
    //    The template parameter selects the ear type (SelfIntersectionEar)
    //    which performs self‑intersection tests during filling.
    //    Parameters: max hole size (10000 edges), selectedOnly (false),
    //    and a callback pointer (nullptr = no progress feedback).
    // ---------------------------------------------------------------------
    // Mark border edges first (required by the hole filling algorithm)
    tri::UpdateFlags<MyMesh>::FaceBorderFromFF(m);
    int holesFilled = tri::Hole<MyMesh>::EarCuttingIntersectionFill<
        tri::SelfIntersectionEar<MyMesh>
    >(m, 10000, false, nullptr);
    cout << "Filled " << holesFilled << " holes." << endl;

    // After filling, we must rebuild topology again
    tri::UpdateTopology<MyMesh>::FaceFace(m);

    // ---------------------------------------------------------------------
    // 7. Consistent orientation of the whole mesh
    //    This function flips faces if necessary so that all normals point
    //    either outward or inward consistently. It returns two booleans:
    //    isOriented   – whether the mesh is now consistently oriented
    //    isOrientable – whether the mesh is topologically orientable
    //                   (a Möbius strip would be non‑orientable)
    // ---------------------------------------------------------------------
    bool isOriented, isOrientable;
    tri::Clean<MyMesh>::OrientCoherentlyMesh(m, isOriented, isOrientable);
    if (!isOrientable) {
        cerr << "Warning: Mesh is non-orientable (e.g., Mobius-like)!" << endl;
    }
    if (isOriented) {
        cout << "Mesh successfully oriented consistently." << endl;
    }
    else {
        cerr << "Warning: Orientation may still be inconsistent." << endl;
    }

    // Compute per‑face and per‑vertex normals (useful for visualization,
    // though not strictly required by TetGen).
    tri::UpdateNormal<MyMesh>::PerFaceNormalized(m);
    tri::UpdateNormal<MyMesh>::PerVertexNormalized(m);

    // ---------------------------------------------------------------------
    // 8. Export the repaired mesh to a PLY file
    //    The exporter automatically chooses format based on file extension.
    //    For TetGen we usually want binary PLY, but the exporter will
    //    handle that if the filename ends with ".ply".
    // ---------------------------------------------------------------------
    if (tri::io::Exporter<MyMesh>::Save(m, outputPath) != 0) {
        cerr << "Error: Failed to save to " << outputPath << endl;
        return -1;
    }

    cout << "Successfully saved watertight mesh to: " << outputPath << endl;
    return 0;
}