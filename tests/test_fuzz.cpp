#include <gtest/gtest.h>

#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "mesh.h"
#include "shapes.h"

namespace{
    //This namespace contains the operations that the fuzzer draws from
    //tesselate() and subdivide() will be added after test_mesh.cpp passes 

    enum class Op{
        Extrude, 
        TriangulateFace, 
        RefineEdge, 
        ClipCorner, 
        BevelEdge, 
        Count
    }; 

    HalfEdgeMesh Cube(){
        HalfEdgeMesh mesh;
        mesh.load(CUBE_VERTICES, CUBE_FACES);
        return mesh;
    }

    std::string ApplyRandomOp(HalfEdgeMesh& mesh, std::mt19937& rng){
        const uint faceCount = mesh.FaceCount(); 
        const uint edgeCount = mesh.EdgeCount();

        if (faceCount == 0 || edgeCount == 0)
            return "skip(empty mesh)"; 
        
        std::uniform_int_distribution<uint> faceDist(0, faceCount - 1);
        std::uniform_int_distribution<uint> edgeDist(0, edgeCount - 1);
        std::uniform_int_distribution<uint> opDist(0, static_cast<uint>(Op::Count) - 1);
        
        std::ostringstream what; 

        switch (static_cast<Op>(opDist(rng))){
            case Op::Extrude: {
                const uint f = faceDist(rng);
                what << "Extrude(" << f << ")";
                mesh.Extrude(f); 
                break; 
            }
            case Op::TriangulateFace: {
                const uint f = faceDist(rng);
                what << "TriangulateFace(" << f << ")";
                mesh.TriangulateFace(f);
                break;
            }
            case Op::RefineEdge: {
                const uint e = edgeDist(rng);
                what << "refineEdge(" << e << ")";
                mesh.refineEdge(e);
                break;
            }
            case Op::ClipCorner: {
                uint e = edgeDist(rng);
                uint inner = MAX_UINT, outer = MAX_UINT, corner = MAX_UINT; 
                what << "ClipCorner(" << e << ")";
                mesh.ClipCorner(e, inner, outer, corner);
                break;
            }
            case Op::BevelEdge: {
                const uint e = edgeDist(rng);
                what << "bevel(" << e << ")";
                
                HalfEdgeMeshBeveler beveler(mesh);
                beveler.StartEdgeBevel(std::vector<uint>{e});
                beveler.AdjustBevel(0.5f);
                beveler.EndBevel(); 
                break;
            }
            default: 
                break;
        }

        return what.str();
    }
    
    std::string RunSequence(uint32_t seed, int steps){
        std::mt19937 rng(seed);
        HalfEdgeMesh mesh = Cube(); 

        std::vector<std::string> history; 
        history.reserve(steps); 

        std::string err; 
        if (!mesh.Validate(err))
            return "the starting cube cube was already invalid: " + err; 
        
        for (int i = 0; i < steps; ++i){
            history.push_back(ApplyRandomOp(mesh, rng)); 

            if (!mesh.Validate(err)){
                std::ostringstream report; 
                report << "\nseed " << seed << " failed at step " << i << " (" << history.back() << ")\n"
                       << "  " << err << "\n"
                       << "  mesh: " << mesh.VertexCount() << " verts, "
                       << mesh.FaceCount() << " faces, "
                       << mesh.EdgeCount() << " edges\n"
                       << "  replay:\n";

                for (size_t h = 0; h < history.size(); ++h)
                    report << "    " << h << ": " << history[h] << "\n";

                return report.str();
            }
        }

        return ""; 
    }
}


//After limiting the iterations in TriangulateFace(), we get this issue: 
//   cube                       V=  8 F=  6 E= 24  valid=yes
//   refineEdge(0)              V=  9 F=  6 E= 26  valid=yes   <- creates degree-2 midpoint v8 (vN where N denotes the index within the vertex array)
//   edge 0 (the index) before bevel: orig=0 dest=8 next=24 face=0
//   beveling edge 24 (orig=8 dest=4)
//   bevel(midpoint edge)       V= 10 F=  7 E= 30  valid=NO   edge 3: next/prev disagree
//   edge 0 after  bevel: orig=0 dest=8 next=29 face=6        <- edge 0 stolen into the new bevel face
TEST(MeshFuzz, RandomOpSequencesKeepCubeValid){
    constexpr uint32_t kSeeds = 32; 
    constexpr int kSteps = 200; 
    
    for (uint32_t seed = 0; seed < kSeeds; ++seed){
        std::string failure = RunSequence(seed, kSteps); 
        ASSERT_TRUE(failure.empty()) << failure; 
    }
}

