namespace geometry {


//there is an edge case where 3 triangles are arrenged so that there is a hole in the center of them, and a 4th triangle contains
// all of these triangle
//in this case, the hole should be filled but it will be empty if all of the triangles are treated as a single polygon
//note: polygons cannot have holes (don't work in error mode) and cannot intersect themselves (can trigger the above edge case)
struct material_map {
    vector<pair<int, int>> face_material_pairs;

    void reset() {
        face_material_pairs.clear();
    }

    void add_face(int face, int material) {
        assert(face_material_pairs.size()==face);
        face_material_pairs.emplace_back(face_material_pairs.size(), material);
    }

    void sort_faces() {
        sort(face_material_pairs.begin(), face_material_pairs.end(), [&](pair<int, int> a, pair<int, int> b) -> bool {
            return a.second<b.second;
        });
    }

    int face_at_index(int index) {
        return face_material_pairs.at(index).first;
    }

    int face_material_at_index(int index) {
        return face_material_pairs.at(index).second;
    }
};

template<class source_type> struct graph_bitmap {
    property<source_type, bool> mask;
    vector<handle<source_type>> mask_true;

    void init(int size) {
        if (mask.size()==size) {
            reset();
        } else {
            mask.clear();
            mask_true.clear();

            mask.expand(size, false);
        }
    }

    bool get(handle<source_type> pos) const {
        return mask[pos];
    }

    handle<source_type> select_true() const {
        assert(!mask_true.empty());
        return mask_true[0];
    }

    void set(handle<source_type> pos) {
        if (!mask[pos]) {
            mask[pos]=true;
            mask_true.push_back(pos);
        }
    }

    void reset() {
        for (handle<source_type> i : mask_true) {
            assert(mask[i]);
            mask[i]=false;
        }
        mask_true.clear();
    }
};

struct polygon_search {
    graph* c_graph;

    graph_bitmap<edge_tag> boundary_edges;
    graph_bitmap<triangle_tag> flagged_triangles;
    graph_bitmap<triangle_tag> flagged_triangles_inverse;
    vector<edge_ptr> pending_edges;

    void init(graph& t_graph) {
        c_graph=&t_graph;

        boundary_edges.init(c_graph->num_edges());
        flagged_triangles.init(c_graph->num_triangles());
        flagged_triangles_inverse.init(c_graph->num_triangles());
    }

    bool flood_fill(bool is_error) {
        while (!pending_edges.empty()) {
            edge_ptr c=pending_edges.back();
            pending_edges.pop_back();

            triangle_ptr n=c_graph->inside[!c];
            if (n.null()) {
                if (!is_error) {
                    return false;
                }
                continue;
            }

            if (flagged_triangles.get(n)) {
                continue;
            }
            flagged_triangles.set(n);

            for (int x=0;x<3;++x) {
                edge_ptr e=c_graph->loop[n][x];
                if (e!=c && !boundary_edges.get(e)) {
                    pending_edges.push_back(e);
                }
            }
        }

        return true;
    }

    const graph_bitmap<triangle_tag>& inside(const vector<edge_ptr>& edges) {
        boundary_edges.reset();
        flagged_triangles.reset();
        pending_edges.clear();

        if (edges.empty()) {
            return flagged_triangles;
        }

        for (edge_ptr e : edges) {
            boundary_edges.set(e);
            pending_edges.push_back(!e);
        }

        bool error=!flood_fill(false);

        if (error) {
            static bool printed_message=false;
            if (!printed_message) {
                print("Warning: Polygon search error mode used (slow)");
                printed_message=true;
            }

            flagged_triangles.reset();
            flagged_triangles_inverse.reset();
            pending_edges.clear();

            triangle_ptr n=c_graph->get_boundary_triangle();
            flagged_triangles.set(n);
            for (int x=0;x<3;++x) {
                pending_edges.push_back(c_graph->loop[n][x]);
            }

            flood_fill(true);

            for (int x=0;x<c_graph->num_triangles();++x) {
                triangle_ptr i(x);
                if (!flagged_triangles.get(i)) {
                    flagged_triangles_inverse.set(i);
                }
            }
        }

        return error? flagged_triangles_inverse : flagged_triangles;
    }
};

//input can have holes and be non-connected
//output edges are guaranteed not to have any duplicates
void polygon_boundary_search(
    graph& c_graph,
    const graph_bitmap<triangle_tag>& c_triangles,
    vector<edge_ptr>& out_edges
) {
    for (triangle_ptr i : c_triangles.mask_true) {
        for (int x=0;x<3;++x) {
            //each edge is processed at most twice
            //an edge is included if its inside is in the set and its outside isn't
            //if an edge is included, it will only be processed once so there will be no duplicates
            edge_ptr e=c_graph.loop[i][x];
            triangle_ptr n=c_graph.inside[!e];
            assert(!n.null());

            if (!c_triangles.get(n)) {
                out_edges.push_back(e);
            }
        }
    }
}


}