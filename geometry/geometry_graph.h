namespace geometry {


scalar cross_2d(vector2 a, vector2 b) {
    //(a x b)[z]=ax*by-ay*bx
    return a.x()*b.y()-a.y()*b.x();
}

vector2 rotate_2d(vector2 a) {
    return vector2(-a.y(), a.x());
}

bool point_in_half_space(vector2 s, vector2 e, vector2 p) {
    vector2 t=rotate_2d(e-s);
    return t.dot(p-s)>=0;
}

scalar triangle_signed_area(vector2 v0, vector2 v1, vector2 v2) {
    return scalar(0.5)*cross_2d(v1-v0, v2-v0);
}

template<class source_tag> class handle {
    int d_i=INT_MAX;

    public:
    handle() {}
    explicit handle(int t_i, bool t_flipped=false) {
        assert((t_i&0x80000000)==0);
        d_i=t_i | (t_flipped? 0x80000000 : 0);
    }

    int i() const {
        return d_i&(0x7FFFFFFF);
    }

    bool is_flipped() const {
        return d_i&0x80000000;
    }

    bool null() const {
        return d_i==INT_MAX;
    }

    handle operator!() const {
        return handle(i(), !is_flipped());
    }

    //note: ignores is_flipped flag
    bool operator==(handle t) const {
        return i()==t.i();
    }

    bool operator!=(handle t) const {
        return i()!=t.i();
    }

    bool operator<(handle t) const {
        return i()<t.i();
    }
};

template<class source_tag, class type> class property {
    vector<type> data;

    public:
    typename vector<type>::reference operator[](handle<source_tag> h) {
        return data.at(h.i());
    }

    typename vector<type>::const_reference operator[](handle<source_tag> h) const {
        return data.at(h.i());
    }

    void expand(int size, type value) {
        assert(data.size()<size);
        data.resize(size, value);
    }

    void clear() {
        data.clear();
    }

    int size() const {
        return data.size();
    }
};

template<class source_tag, class type> class property_pair {
    vector<array<type, 2>> data;

    public:
    typename vector<type>::reference operator[](handle<source_tag> h) {
        return data.at(h.i())[h.is_flipped()];
    }

    typename vector<type>::const_reference operator[](handle<source_tag> h) const {
        return data.at(h.i())[h.is_flipped()];
    }

    void expand(int size, type value, type value_flipped) {
        assert(data.size()<size);
        data.resize(size, {value, value_flipped});
    }

    void clear() {
        data.clear();
    }

    int size() const {
        return data.size();
    }
};

struct vert_tag {};
struct edge_tag {};
struct triangle_tag {};

typedef handle<vert_tag> vert_ptr;
typedef handle<edge_tag> edge_ptr;
typedef handle<triangle_tag> triangle_ptr;

struct graph {
    property<vert_tag, vector2> position;
    property<vert_tag, edge_ptr> contains; //start == this vert
    property<vert_tag, array<vert_ptr, 2>> vert_source;

    property_pair<edge_tag, vert_ptr> end;
    property_pair<edge_tag, triangle_ptr> inside;
    property<edge_tag, bool> is_fixed;

    property<triangle_tag, array<edge_ptr, 3>> loop; //ccw

    scalar merge_distance;

    //
    //

    void expand_verts(int size) {
        position.expand(size, vector2(NAN, NAN));
        contains.expand(size, edge_ptr());
        vert_source.expand(size, {vert_ptr(), vert_ptr()});
    }

    void expand_edges(int size) {
        end.expand(size, vert_ptr(), vert_ptr());
        inside.expand(size, triangle_ptr(), triangle_ptr());
        is_fixed.expand(size, false);
    }

    void expand_triangles(int size) {
        loop.expand(size, {edge_ptr(), edge_ptr(), edge_ptr()});
    }

    int num_verts() {
        return position.size();
    }

    int num_edges() {
        return end.size();
    }

    int num_triangles() {
        return loop.size();
    }

    scalar signed_area(triangle_ptr i) {
        return triangle_signed_area(
            position[get_vert(i, 0)],
            position[get_vert(i, 1)],
            position[get_vert(i, 2)]
        );
    }

    //ccw
    void init(vector2 a, vector2 b, vector2 c, scalar t_merge_distance) {
        merge_distance=t_merge_distance;

        position.clear();
        contains.clear();
        vert_source.clear();

        end.clear();
        inside.clear();
        is_fixed.clear();

        loop.clear();

        expand_verts(3);
        expand_edges(3);
        expand_triangles(1);

        position[vert_ptr(0)]=a;
        position[vert_ptr(1)]=b;
        position[vert_ptr(2)]=c;

        for (int x=0;x<3;++x) {
            end[edge_ptr(x, true)]=vert_ptr((x+2)%3);
            end[edge_ptr(x, false)]=vert_ptr(x);

            contains[vert_ptr(x)]=edge_ptr(x, true);

            inside[edge_ptr(x)]=triangle_ptr(0);

            loop[triangle_ptr(0)][x]=edge_ptr(x, false);
        }
    }

    void check_consistency() {
        if (num_triangles()==0) {
            assert(num_verts()==0);
            assert(num_edges()==0);
            return;
        }

        for (int x=0;x<num_verts();++x) {
            vert_ptr i(x);
            assert(end[!contains[i]]==i);
        }

        for (int x=0;x<num_edges();++x) {
            edge_ptr i(x, false);
            edge_ptr j(x, true);

            assert(!inside[i].null() || !inside[j].null());
            assert(inside[i]!=inside[j]);
            assert(end[i]!=end[j]);

            if (inside[i].null() || inside[j].null()) {
                assert(end[i].i()<=2);
                assert(end[j].i()<=2);
            }

            for (int n=0;n<2;++n) {
                if (inside[n? i : j].null()) {
                    continue;
                }

                bool found_edge=false;
                for (int y=0;y<3;++y) {
                    if (loop[inside[n? i : j]][y]==i) {
                        assert(!found_edge);
                        found_edge=true;
                    }
                }
                assert(found_edge);
            }
        }

        for (int x=0;x<num_triangles();++x) {
            triangle_ptr i(x);

            for (int y=0;y<3;++y) {
                edge_ptr e1=loop[i][(y+2)%3];
                edge_ptr e2=loop[i][y];
                edge_ptr e3=loop[i][(y+1)%3];

                assert(end[e1]==end[!e2]);
                assert(inside[e1]==i);
                assert(inside[!e1]!=i);
                assert(
                    (inside[!e1].null() && inside[!e2].null() && inside[!e3].null()) ||
                    (inside[!e1]!=inside[!e2] && inside[!e2]!=inside[!e3] && inside[!e1]!=inside[!e3])
                );
            }

            scalar c_signed_area=signed_area(i);
            assert(c_signed_area>=-1e-5f);
        }
    }

    vert_ptr get_vert(triangle_ptr n, int x) {
        return end[loop[n][x]];
    }

    vert_ptr vert_opposite_edge(triangle_ptr n, edge_ptr e, int* out_pos=nullptr) {
        int v_pos=-1;
        for (int x=0;x<3;++x) {
            vert_ptr v=get_vert(n, x);
            if (v!=end[e] && v!=end[!e]) {
                assert(v_pos==-1);
                v_pos=x;
            }
        }
        assert(v_pos!=-1);

        if (out_pos!=nullptr) {
            *out_pos=v_pos;
        }
        return get_vert(n, v_pos);
    }

    edge_ptr edge_opposite_vert(triangle_ptr n, vert_ptr v, int* out_pos=nullptr) {
        int e_pos=-1;
        for (int x=0;x<3;++x) {
            edge_ptr e=loop[n][x];
            if (v!=end[e] && v!=end[!e]) {
                assert(e_pos==-1);
                e_pos=x;
            }
        }
        assert(e_pos!=-1);

        if (out_pos!=nullptr) {
            *out_pos=e_pos;
        }
        return loop[n][e_pos];
    }

    void change_edge_inside(edge_ptr e, triangle_ptr old_triangle, triangle_ptr new_triangle) {
        assert(inside[e]==old_triangle);
        inside[e]=new_triangle;
    }

    template<class func_type> void iterate_vert(vert_ptr v, func_type func) {
        edge_ptr e_start=contains[v];
        edge_ptr e=e_start;

        int iter=0;

        bool first=true;
        while (true) {
            assert(end[!e]==v);

            int e_pos=-1;
            edge_ptr next_e;
            for (int x=0;x<3;++x) {
                if (loop[inside[e]][x]==e) {
                    next_e=loop[inside[e]][(x+2)%3];
                    e_pos=x;
                    break;
                }
            }
            assert(e_pos!=-1);

            bool last=(next_e==e_start);

            if (!func(e, inside[e], e_pos, first, last)) {
                break;
            }

            if (last) {
                break;
            }

            first=false;

            e=!next_e;

            ++iter;
            assert(iter<=num_triangles());
        }
    }

    bool is_inside(edge_ptr e, vector2 pos) {
        return point_in_half_space(position[end[!e]], position[end[e]], pos);
    }

    triangle_ptr search(vector2 pos, triangle_ptr i=triangle_ptr()) {
        if (i.null()) {
            i=triangle_ptr(num_triangles()-1);
        }

        edge_ptr previous_edge;

        int iter=0;
        while (true) {
            bool found_edge=false;
            for (int x=0;x<3;++x) {
                edge_ptr e=loop[i][x];
                if (!previous_edge.null() && e==previous_edge) {
                    continue;
                }

                if (is_inside(loop[i][x], pos)) {
                    continue;
                }

                i=inside[!e];
                assert(!i.null());

                previous_edge=e;
                found_edge=true;
                break;
            }

            if (!found_edge) {
                break;
            }

            ++iter;
            assert(iter<=num_triangles());
        }

        return i;
    }

    void grid_triangles(grid_2d<triangle_ptr>& t_grid) {
        triangle_ptr last_i;
        t_grid.iterate([&](vector2 p, triangle_ptr& res) -> void {
            res=search(p, last_i);
            last_i=res;
        });
    }

    edge_ptr first_colliding_edge(vert_ptr t_start, vert_ptr t_end) {
        edge_ptr res;

        vector2 p=position[t_end];
        bool was_inside=false;
        bool first_inside=false;

        iterate_vert(t_start, [&](edge_ptr e, triangle_ptr n, int n_pos, bool first, bool last) -> bool {
            assert(end[!e]==t_start);
            if (end[e]==t_end) {
                res=e;
                return false;
            }

            if (first) {
                first_inside=is_inside(e, p);
            }

            bool previous_inside=(first)? first_inside : !was_inside;
            bool next_inside=(last)? !first_inside : is_inside(loop[n][(n_pos+2)%3], p);

            if (previous_inside && next_inside) {
                res=loop[n][(n_pos+1)%3];
                return false;
            }

            was_inside=next_inside;
            return true;
        });

        assert(!res.null());
        return res;
    }

    edge_ptr next_colliding_edge(edge_ptr c, vert_ptr t_start, vert_ptr t_end) {
        assert(end[c]!=t_end);

        triangle_ptr n=inside[!c];
        assert(!n.null());

        int v_pos;
        vert_ptr v=vert_opposite_edge(n, c, &v_pos);

        edge_ptr res;
        if (v==t_end) {
            res=loop[n][v_pos];
        } else {
            int e_pos=(v_pos+(point_in_half_space(position[t_start], position[t_end], position[v])? 0 : 1))%3;
            res=loop[n][e_pos];
        }

        assert(res!=c);

        return res;
    }

    vert_ptr add_vert(triangle_ptr bounds, vector2 pos) {
        expand_verts(num_verts()+1);
        vert_ptr new_vert(num_verts()-1);

        expand_triangles(num_triangles()+2);
        array<triangle_ptr, 3> new_triangles={
            bounds,
            triangle_ptr(num_triangles()-2),
            triangle_ptr(num_triangles()-1)
        };

        expand_edges(num_edges()+3);
        array<edge_ptr, 3> new_edges={
            edge_ptr(num_edges()-3),
            edge_ptr(num_edges()-2),
            edge_ptr(num_edges()-1)
        };

        position[new_vert]=pos;
        contains[new_vert]=!new_edges[0];

        array<edge_ptr, 3> old_triangle_loop=loop[bounds];
        for (int x=0;x<3;++x) {
            loop[new_triangles[x]][0]=old_triangle_loop[x];
            loop[new_triangles[x]][1]=new_edges[x];
            loop[new_triangles[x]][2]=!new_edges[(x+2)%3];

            end[!new_edges[x]]=end[old_triangle_loop[x]];
            end[new_edges[x]]=new_vert;
            inside[new_edges[x]]=new_triangles[x];
            inside[!new_edges[x]]=new_triangles[(x+1)%3];

            change_edge_inside(old_triangle_loop[x], bounds, new_triangles[x]);
        }

        return new_vert;
    }

    //line does not end at endpoints
    bool edge_intersects_line(edge_ptr e, vert_ptr line_start, vert_ptr line_end) {
        if (
            end[!e]==line_start || end[e]==line_start ||
            end[!e]==line_end || end[e]==line_end
        ) {
            return false;
        }

        bool p0=point_in_half_space(position[line_start], position[line_end], position[end[!e]]);
        bool p1=point_in_half_space(position[line_start], position[line_end], position[end[e]]);
        return p0!=p1;
    }

    //assumes edge_intersects_line returned true
    vector2 edge_line_intersection(edge_ptr e, vector2 line_start, vector2 line_end) {
        vector2 n=rotate_2d(line_end - line_start);
        scalar k=n.dot(line_start);
        vector2 s=position[end[!e]];
        vector2 d=position[end[e]] - s;

        /*
        p=s+td
        p.n=k
        p.n=k=s.n+td.n
        t=(k-s.n)/(d.n)
        t>=0 ; t<=1
        */

        scalar t=(k-s.dot(n))/(d.dot(n));

        if (!(t>=0)) {
            return s;
        } else
        if (!(t<=1)) {
            return position[end[e]];
        } else {
            return s+t*d;
        }
    }

    //assumes the edge is an auto edge and that both nodes have the same state
    //will not change the node states and will reset the edge state
    bool flip_edge(edge_ptr c_edge, bool force) {
        assert(!is_fixed[c_edge]);

        array<triangle_ptr, 2> new_triangles={inside[c_edge], inside[!c_edge]};

        array<vert_ptr, 2> old_verts={end[!c_edge], end[c_edge]};

        array<int, 2> new_verts_pos;
        array<vert_ptr, 2> new_verts;
        array<array<edge_ptr, 2>, 2> new_edges;

        for (int x=0;x<2;++x) {
            new_verts[x]=vert_opposite_edge(new_triangles[x], c_edge, &new_verts_pos[x]);
            new_edges[x][0]=loop[new_triangles[x]][new_verts_pos[x]];
            new_edges[x][1]=loop[new_triangles[x]][(new_verts_pos[x]+1)%3];
        }

        if (!force && !edge_intersects_line(c_edge, new_verts[0], new_verts[1])) {
            //both of the old verts are on the same side as the half space formed by the new verts
            return false;
        }

        for (int x=0;x<2;++x) {
            if (contains[old_verts[x]]!=c_edge) {
                continue;
            }

            edge_ptr t_contains;
            iterate_vert(old_verts[x], [&](edge_ptr e, triangle_ptr n, int n_pos, bool first, bool last) -> bool {
                if (e!=c_edge) {
                    t_contains=e;
                    return false;
                } else {
                    return true;
                }
            });

            assert(!t_contains.null());
            contains[old_verts[x]]=t_contains;
        }

        //inside and outside are unchanged
        end[!c_edge]=new_verts[0];
        end[c_edge]=new_verts[1];

        for (int x=0;x<2;++x) {
            triangle_ptr n=new_triangles[x];

            if (x==0) {
                loop[n][0]=c_edge;
                loop[n][1]=new_edges[1][1];
                loop[n][2]=new_edges[0][0];
                change_edge_inside(loop[n][1], new_triangles[1], n);
            } else {
                loop[n][0]=!c_edge;
                loop[n][1]=new_edges[0][1];
                loop[n][2]=new_edges[1][0];
                change_edge_inside(loop[n][1], new_triangles[0], n);
            }
        }

        return true;
    }

    //a, b, c are ccw
    //the line a-b needs to partition c and p into opposite half spaces
    bool point_in_circumcircle(vector2 a, vector2 b, vector2 c, vector2 p) {
        /*
        x13 = x1 - x3, etc
        1:a, 2:b, 3:c

        cos a=x13*x23 + y13*y23 = V13.V23
        cos b=x2p*x1p + y2p*y1p = V2P.V1P
        sin ab=(x13*y23 - y13*x23)cos b + (x2p*y1p - y2p*x1p)cos a
        =(V13 x V23)cos b + (V2P x V1P)cos a
        (a x b)[z]=ax*by-ay*bx
        */

        scalar cos_a=(a-c).dot(b-c); //cos of angle from CA to CB
        scalar cos_b=(a-p).dot(b-p); //cos of angle from PB to PA

        if (cos_a>=0 && cos_b>=0) {
            return false;
        }

        if (cos_a<0 && cos_b<0) {
            return true;
        }

        //cross product terms are: sin of angle from CA to CB; sin of angle from PB to PA
        //sin ab = sin a * cos b + sin b * cos a
        //sin ab = sin (a+b)
        scalar sin_ab=cross_2d(a-c, b-c)*cos_b + cross_2d(b-p, a-p)*cos_a;

        return sin_ab<0;
    }

    //e is an edge of n which has n on one side of its half space and v on the other side
    bool point_in_circumcircle(triangle_ptr n, edge_ptr e, vert_ptr v) {
        vector2 a=position[end[!e]];
        vector2 b=position[end[e]];
        vector2 c=position[vert_opposite_edge(n, e)];

        vector2 p=position[v];

        const scalar eps=1e-5f;
        assert(0.5*cross_2d(b-a, c-a)>=-eps);
        vector2 t=rotate_2d(b-a).normalized();
        scalar ct=t.dot(c)-t.dot(a);
        scalar pt=t.dot(p)-t.dot(a);
        assert(
            (ct<eps && pt>-eps) ||
            (ct>-eps && pt<eps)
        );

        return point_in_circumcircle(a, b, c, p);
    }

    vert_ptr add_vert(vector2 pos) {
        triangle_ptr bounds=search(pos);
        for (int x=0;x<3;++x) {
            if ((position[get_vert(bounds, x)] - pos).squaredNorm()<=square(merge_distance)) {
                return get_vert(bounds, x);
            }
        }

        vert_ptr res=add_vert(bounds, pos);
        delaunay_vert(res);
        return res;
    }

    vector<edge_ptr> delaunay_vert_edges;
    void delaunay_vert(vert_ptr t_vert) {
        vector<edge_ptr>& c_edges=delaunay_vert_edges;
        c_edges.clear();

        iterate_vert(t_vert, [&](edge_ptr e, triangle_ptr n, int n_pos, bool first, bool last) -> bool {
            c_edges.push_back(loop[n][(n_pos+1)%3]);
            return true;
        });

        int iter=0;
        while (!c_edges.empty()) {
            edge_ptr e=c_edges.back();
            c_edges.pop_back();

            triangle_ptr n=inside[!e];

            if (
                !inside[e].null() && !inside[!e].null() &&
                !is_fixed[e] && point_in_circumcircle(n, !e, t_vert)
            ) {
                flip_edge(e, true);

                c_edges.push_back(edge_opposite_vert(inside[e], t_vert));
                c_edges.push_back(edge_opposite_vert(inside[!e], t_vert));
            }

            ++iter;
            assert(iter<=num_edges());
        }
    }

    pair<vert_ptr, bool> subdivide_edge(edge_ptr old_edge, vector2 pos, bool add_vert_to_outside=false) {
        assert(!inside[old_edge].null() && !inside[!old_edge].null());
        assert(is_fixed[old_edge]);

        //print("SUBDIVIDE", position[end[!old_edge]], position[end[old_edge]], pos);

        if ((position[end[!old_edge]] - pos).squaredNorm()<=square(merge_distance)) {
            return make_pair(end[!old_edge], false);
        } else
        if ((position[end[old_edge]] - pos).squaredNorm()<=square(merge_distance)) {
            return make_pair(end[old_edge], false);
        }

        vert_ptr old_start=end[!old_edge];
        vert_ptr old_end=end[old_edge];
        array<vert_ptr, 2> start_source=vert_source[end[!old_edge]];
        array<vert_ptr, 2> end_source=vert_source[end[old_edge]];

        vert_ptr res=add_vert((add_vert_to_outside)? inside[!old_edge] : inside[old_edge], pos);

        triangle_ptr n=(add_vert_to_outside)? inside[!old_edge] : inside[old_edge];

        for (int x=0;x<3;++x) {
            is_fixed[loop[n][x]]=true;
        }

        is_fixed[old_edge]=false;
        flip_edge(old_edge, true);

        //if an existing constraint edge is subdivided, the new vert must have its sources set to the edge's original endpoints
        //if it is subdivided multiple times, this applies to each new vert that is added
        array<vert_ptr, 2> res_source={old_start, old_end};

        assert(start_source[0].null() == start_source[1].null());
        assert(end_source[0].null() == end_source[1].null());
        assert(start_source[0].null() || start_source[0]!=start_source[1]);
        assert(end_source[0].null() || end_source[0]!=end_source[1]);
        assert(start_source[0]!=old_start && start_source[1]!=old_start);
        assert(end_source[0]!=old_end && end_source[1]!=old_end);

        bool assigned_res_source=false;
        if (!start_source[0].null() && !end_source[0].null()) {
            if (
                start_source==end_source ||
                (start_source[1]==end_source[0] && start_source[0]==end_source[1])
            ) {
                assert(!assigned_res_source);
                res_source=start_source;
                assigned_res_source=true;
            }
        }

        if (!start_source[0].null()) {
            if (start_source[0]==old_end) {
                assert(!assigned_res_source);
                res_source[0]=start_source[1];
                assigned_res_source=true;
            }

            if (start_source[1]==old_end) {
                assert(!assigned_res_source);
                res_source[0]=start_source[0];
                assigned_res_source=true;
            }
        }

        if (!end_source[0].null()) {
            if (end_source[0]==old_start) {
                assert(!assigned_res_source);
                res_source[1]=end_source[1];
                assigned_res_source=true;
            }

            if (end_source[1]==old_start) {
                assert(!assigned_res_source);
                res_source[1]=end_source[0];
                assigned_res_source=true;
            }
        }

        vert_source[res]=res_source;

        /*
        if (!start_source[0].null()) {
            print(" start source", position[start_source[0]], position[start_source[1]]);
        }
        if (!end_source[0].null()) {
            print(" end source", position[end_source[0]], position[end_source[1]]);
        }
        if (!res_source[0].null()) {
            print(" res source", position[res_source[0]], position[res_source[1]]);
        }*/

        return make_pair(res, true);
    }

    vector<edge_ptr> add_edge_impl_stack_initial;
    vector<edge_ptr> add_edge_impl_stack;
    vector<edge_ptr> add_edge_impl_stack_2;
    vert_ptr add_fixed_edge_impl(vert_ptr t_start, vert_ptr t_end) {
        assert(t_start!=t_end);

        vector<edge_ptr>& c_stack=add_edge_impl_stack;
        c_stack.clear();

        vector2 tangent=position[t_end] - position[t_start];
        vector2 normal=rotate_2d(tangent);
        scalar normal_squared_norm=normal.squaredNorm(); //also tangent squared norm
        scalar plane_constant=normal.dot(position[t_start]);
        auto should_merge=[&](vector2 p) -> bool {
            /*
            d=(n/|n|).(p-s)
            d<=K
            (n/|n|).(p-s)<=K
            n.(p-s)/|n|<=K
            n.(p-s)<=K*|n|
            (n.(p-s))^2 <= K^2 * |n|^2
            */

            scalar d=normal.dot(p)-plane_constant;
            if (square(d)>square(merge_distance)*normal_squared_norm) {
                return false;
            }

            /*
            t=(e-s).(p-s)
            t>=0 && t<=(e-s).(e-s)=|n|^2
            */

            scalar t=tangent.dot(p - position[t_start]);
            return t>=0 && t<=normal_squared_norm;
        };

        bool run_delaunay=false;
        {
            edge_ptr c_edge=first_colliding_edge(t_start, t_end);

            int iter=0;
            while (true) {
                if (end[c_edge]==t_end) {
                    break;
                }

                if (is_fixed[c_edge]) {
                    pair<vert_ptr, bool> r=subdivide_edge(c_edge, edge_line_intersection(c_edge, position[t_start], position[t_end]));
                    t_end=r.first;
                    run_delaunay=r.second;
                    break;
                }

                if (should_merge(position[end[!c_edge]])) {
                    t_end=end[!c_edge];
                    break;
                }

                if (should_merge(position[end[c_edge]])) {
                    t_end=end[c_edge];
                    break;
                }

                c_stack.push_back(c_edge);
                c_edge=next_colliding_edge(c_edge, t_start, t_end);

                ++iter;
                assert(iter<=num_edges());
            }
        }

        vector<edge_ptr>& initial_stack=add_edge_impl_stack_initial;
        initial_stack=c_stack;

        {
            vector<edge_ptr>& next_stack=add_edge_impl_stack_2;
            next_stack.clear();

            int iter=0;
            while (true) {
                while (!c_stack.empty()) {
                    edge_ptr e=c_stack.back();
                    c_stack.pop_back();

                    //note: this can sometimes fail to converge if the edge is within a couple machine epsilon of a vertex
                    //should use the merge_distance to avoid this
                    if (
                        !flip_edge(e, false) ||
                        edge_intersects_line(e, t_start, t_end)
                    ) {
                        next_stack.push_back(e);
                    }

                    ++iter;
                    assert(iter<=num_edges());
                }

                swap(next_stack, c_stack);
                next_stack.clear();
                if (c_stack.empty()) {
                    break;
                }
            }
        }

        bool found_edge=false;
        iterate_vert(t_start, [&](edge_ptr e, triangle_ptr n, int e_pos, bool first, bool last) -> bool {
            if (end[e]==t_end) {
                assert(end[!e]==t_start);
                is_fixed[e]=true;
                found_edge=true;

                return false;
            } else {
                return true;
            }
        });
        assert(found_edge);

        {
            int iter=0;
            while (true) {
                bool did_flip=false;
                for (edge_ptr e : initial_stack) {
                    if (
                        (end[!e]==t_start && end[e]==t_end) ||
                        (end[e]==t_start && end[!e]==t_end)
                    ) {
                        continue;
                    }

                    if (
                        point_in_circumcircle(inside[e], e, vert_opposite_edge(inside[!e], e)) ||
                        point_in_circumcircle(inside[!e], !e, vert_opposite_edge(inside[e], e))
                    ) {
                        flip_edge(e, true);
                        did_flip=true;

                        ++iter;
                        assert(iter<=num_edges());
                    }
                }

                if (!did_flip) {
                    break;
                }
            }
        }

        if (run_delaunay) {
            delaunay_vert(t_end);
        }

        return t_end;
    }

    //t_start is not added to out_verts; t_end is
    void add_fixed_edge(vert_ptr t_start, vert_ptr t_end, vector<vert_ptr>& out_verts) {
        if (t_start==t_end) {
            return;
        }

        int max_iter=num_edges();

        int iter=0;
        vert_ptr c_start=t_start;
        while (c_start!=t_end) {
            vert_ptr c_end=add_fixed_edge_impl(c_start, t_end);
            out_verts.push_back(c_end);

            c_start=c_end;

            ++iter;
            assert(iter<=max_iter);
        }
    }
	
    triangle_ptr get_boundary_triangle() {
        edge_ptr e=contains[vert_ptr(0)];
        assert(inside[e].null() || inside[!e].null());
        return (inside[e].null())? inside[!e] : inside[e];
    }

    //two input verts must be adjacent in the array returned by add_fixed_edge
    //(t_end can also be the first entry in the array if t_start is the t_start passed to add_fixed_edge)
    void iterate_fixed_edge(vert_ptr t_start, vert_ptr t_end, vector<edge_ptr>& out_edges) {
        //print("==== ITERATE", position[t_start], position[t_end]);

        vert_ptr c_start=t_start;
        int iter=0;
        vert_ptr prev_start;
        while (c_start!=t_end) {
            edge_ptr next_edge;
            iterate_vert(c_start, [&](edge_ptr e, triangle_ptr n, int e_pos, bool first, bool last) -> bool {
                /*print(position[end[e]]);
                if (!vert_source[end[e]][0].null()) {
                    print(" source ", position[vert_source[end[e]][0]], position[vert_source[end[e]][1]]);
                }*/
                if (end[e]==t_end) {
                    assert(next_edge.null());
                    next_edge=e;
                } else
                if (end[e]!=prev_start) {
                    array<vert_ptr, 2> source=vert_source[end[e]];
                    if (
                        (source[0]==t_start && source[1]==t_end) ||
                        (source[1]==t_start && source[0]==t_end)
                    ) {
                        assert(next_edge.null());
                        next_edge=e;
                    }
                }
                return true;
            });
            assert(!next_edge.null());

            out_edges.push_back(next_edge);
            prev_start=c_start;
            c_start=end[next_edge];
            //print("<< C_START = ", position[c_start]);

            ++iter;
            assert(iter<=num_edges());
        }
    }
};

struct graph_parameters {
    vector2 min_pos=vector2(INFINITY, INFINITY);
    vector2 max_pos=vector2(-INFINITY, -INFINITY);
    scalar merge_distance;

    graph_parameters(scalar t_merge_distance) : merge_distance(t_merge_distance) {}

    void init_graph(graph& t_graph) {
        /*
        right triangle containing the bounding rect
        the rect is tiled 4 times (left, right, bottom, top)
        the triangle origin is in the bottom left
        the other triangle verts are in the top left and bottom right
        the diagonal will contain the top right vert of the bottom left rect
        */

        const scalar padding_factor=1;
        const scalar padding_min=1;

        vector2 s=min_pos;
        vector2 d=max_pos-min_pos;
        if (!isfinite(s.x() + s.y() + d.x() + d.y())) {
            s=vector2::Zero();
            d=vector2::Zero();
        }
        d.x()=max(d.x(), padding_min);
        d.y()=max(d.y(), padding_min);

        s=s-d*padding_factor;
        d=d*(padding_factor*3);

        t_graph.init(
            s,
            s+vector2(d.x()*2, 0),
            s+vector2(0, d.y()*2),
            merge_distance
        );
    }

    void add(vector2 pos) {
        for (int i=0;i<2;++i) {
            min_pos[i]=min(min_pos[i], pos[i]);
            max_pos[i]=max(max_pos[i], pos[i]);
        }
    }
};

struct bin_sort {
    vector<pair<vector2, int>> verts;

    vector<vert_ptr> added_verts;

    void reset() {
        verts.clear();
        added_verts.clear();
    }

    //if update_parameters is false, pos should be between min_pos and max_pos
    void add_vert(int id, vector2 pos, bool allow_duplicate=false) {
        while (verts.size()<=id) {
            verts.emplace_back(vector2(NAN, NAN), verts.size());
        }

        pair<vector2, int>& c=verts.at(id);
        assert(allow_duplicate || isnan(c.first.x() + c.first.y()));
        assert(c.second==id);

        c.first=pos;

        assert(isfinite(c.first.x() + c.first.y()));
    }

    void make_graph(graph& c_graph, graph_parameters c_parameters) {
        {
            vector<pair<vector2, int>>::iterator verts_end=partition(
                verts.begin(), verts.end(), [&](pair<vector2, int> c) -> bool {
                    return !isnan(c.first.x() + c.first.y());
                }
            );
            verts.resize(verts_end-verts.begin());
        }

        sort(verts.begin(), verts.end(), [&](pair<vector2, int> a, pair<vector2, int> b) -> bool {
            return a.first.y()<b.first.y();
        });

        int stride=int(ceil(pow(verts.size(), 1.0/4.0)));
        if (stride<1) {
            stride=1;
        }

        int start=0;
        bool reverse_next=false;
        while (start<verts.size()) {
            int end=start+stride;
            if (end>verts.size()) {
                end=verts.size();
            }

            sort(verts.begin()+start, verts.begin()+end, [&](pair<vector2, int> a, pair<vector2, int> b) -> bool {
                return a.first.x()<b.first.x();
            });

            if (reverse_next) {
                reverse(verts.begin()+start, verts.begin()+end);
            }

            reverse_next=!reverse_next;
            start=end;
        }

        c_parameters.init_graph(c_graph);

        for (int x=0;x<verts.size();++x) {
            pair<vector2, int> c=verts[x];

            while (added_verts.size()<=c.second) {
                added_verts.emplace_back();
            }

            assert(isfinite(c.first.x() + c.first.y()));
            assert(added_verts.at(c.second).null());
            added_verts.at(c.second)=c_graph.add_vert(c.first);
        }
    }

    vert_ptr get_vert(int id) {
        return added_verts.at(id);
    }
};

struct graph_edges {
    vector<vert_ptr> verts;
    vector<int> edges_start;
    vector<int> edge_groups_start;

    void reset() {
        verts.clear();
        edges_start.clear();
        edge_groups_start.clear();
    }

    void add_group(graph& c_graph, const vector<pair<vert_ptr, vert_ptr>>& t_edges) {
        edge_groups_start.push_back(edges_start.size());
        for (pair<vert_ptr, vert_ptr> c : t_edges) {
            edges_start.push_back(verts.size());
            verts.push_back(c.first);
            c_graph.add_fixed_edge(c.first, c.second, verts);
        }
    }

    int group_size(int group_index) {
        int group_start=edge_groups_start.at(group_index);
        int group_end=(group_index==edge_groups_start.size()-1)? edges_start.size() : edge_groups_start.at(group_index+1);
        int group_size=group_end-group_start;
        return group_size;
    }

    void get_edge(graph& c_graph, int index, vector<edge_ptr>& res) {
        int start=edges_start.at(index);
        int end=(index==edges_start.size()-1)? verts.size() : edges_start.at(index+1);
        int size=end-start;

        for (int x=1;x<size;++x) {
            c_graph.iterate_fixed_edge(verts[start+x-1], verts[start+x], res);
        }
    }

    void get_group(graph& c_graph, int group_index, vector<edge_ptr>& res) {
        int group_start=edge_groups_start.at(group_index);
        int size=group_size(group_index);
        for (int x=0;x<size;++x) {
            get_edge(c_graph, group_start+x, res);
        }
    }
};


}