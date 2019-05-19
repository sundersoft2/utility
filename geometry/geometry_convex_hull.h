namespace geometry {


//the half space intersection algorithm works in arbitrary dimensions
//have an origin point and a bunch of half-spaces which all contain the origin
//will calculate the intersection of all of the half-spaces
//
//each half space has a dual point
//the half space has a point which is closest to the origin
//the vector between the point and the origin is colinear with the half space normal
//to calculate the dual point, the nearest point on the plane is scaled by the reciprocal of the distance between it and the origin
//take the convex hull of all of the dual points
//each facet in the convex hull can be mapped to an original point
//to do so, take the closest point on the facet's plane to the origin and scale it by the reciprocal of its distance
//
//to output triangles in 3d:
//take each edge of the convex hull and consider one of its vertices along with the two adjacent facets
//each adjacent facet is mapped to a point
//the vertex is mapped to an output face, which has a midpoint (some arbitrary point on the face)
//the two output points plus the output face midpoint form a triangle
//the midpoint for each output facet is selected to be one of its output vertices
//i.e. in the convex hull, each vertex is mapped to one of its adjacent faces and that face's dual vertex is used as the midpoint
//
//for 3d, recommend using the quickhull algorithm. there is an implementation in the shared folder

struct convex_hull {
    scalar merge_distance;
    bin_sort c_bin_sort;
    graph c_graph;
    property<triangle_tag, bool> outer_triangles;
    property<edge_tag, bool> boundary_edges;
    property<vert_tag, bool> boundary_verts;

    vector<vector2> points_buffer;

    convex_hull(scalar t_merge_distance) : merge_distance(t_merge_distance) {}

    vector2 map_plane_to_point(pair<vector2, scalar> s) {
        vector2 n=s.first;
        scalar k=s.second;

        /*
        p.n=k ; |n|=1
        p=n*k
        q=p/(|p|)^2

        q=n*k/(|n|*k)^2
        q=n*k/(k^2)
        q=n/k

        if n and k are both doubled, then q will be unchanged, so n does not have to be normalized
        */

        return n/k;
    }

    template<bool use_origin> pair<vector2, scalar> get_plane(graph& t_graph, edge_ptr t_edge, vector2 origin) {
        vector2 s=t_graph.position[t_graph.end[!t_edge]];
        vector2 e=t_graph.position[t_graph.end[t_edge]];

        vector2 n=rotate_2d(e-s);
        scalar k=n.dot((use_origin)? s-origin : s);

        return make_pair(n, k);
    }

    template<class func_type> void hull(const vector<vector2>& points, func_type func) {
        graph_parameters c_parameters(merge_distance);

        c_bin_sort.reset();
        for (int x=0;x<points.size();++x) {
            vector2 p=points[x];
            c_parameters.add(p);
            c_bin_sort.add_vert(x, p);
        }
        c_bin_sort.make_graph(c_graph, c_parameters);

        outer_triangles.clear();
        outer_triangles.expand(c_graph.num_triangles(), false);

        boundary_edges.clear();
        boundary_edges.expand(c_graph.num_edges(), false);

        boundary_verts.clear();
        boundary_verts.expand(c_graph.num_verts(), false);

        for (int t_id=0;t_id<c_graph.num_triangles();++t_id) {
            triangle_ptr t(t_id);

            bool outside=false;
            for (int x=0;x<3;++x) {
                outside|=(c_graph.get_vert(t, x).i()<3);
            }

            outer_triangles[t]=outside;
        }

        for (int e_id=0;e_id<c_graph.num_edges();++e_id) {
            edge_ptr e(e_id);
            triangle_ptr a=c_graph.inside[e];
            triangle_ptr b=c_graph.inside[!e];

            bool a_outer=(a.null() || outer_triangles[a]);
            bool b_outer=(b.null() || outer_triangles[b]);

            if (a_outer!=b_outer) {
                assert(!a.null() && !b.null());
                boundary_edges[e]=true;
                boundary_verts[c_graph.end[!e]]=true;
                boundary_verts[c_graph.end[e]]=true;
            }
        }

        for (int v_id=0;v_id<c_graph.num_verts();++v_id) {
            vert_ptr v(v_id);
            if (!boundary_verts[v]) {
                continue;
            }

            array<pair<vector2, scalar>, 2> planes;
            int next_plane=0;
            c_graph.iterate_vert(v, [&](edge_ptr e, triangle_ptr n, int e_pos, bool first, bool last) -> bool {
                if (!boundary_edges[e]) {
                    return true;
                }

                assert(next_plane<2);
                planes[next_plane]=get_plane<false>(c_graph, e, vector2::Zero());
                ++next_plane;

                return true;
            });

            assert(next_plane==2);
            func(planes[0], planes[1]);
        }
    }

    void reset() {
        points_buffer.clear();
    }

    void add_edge(graph& t_graph, vector2 origin, edge_ptr& e) {
        points_buffer.push_back(map_plane_to_point(get_plane<true>(t_graph, e, origin)));
    }
};


}