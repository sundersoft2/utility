namespace geometry { namespace blender {


struct ngon {
    vector<int> verts;
    int material;
};

struct object {
    string name;
    vector3 pos=vector3::Zero();
    quaternion rotation=quaternion::Identity();

    vector<vector3> vert_pos;
    vector<ngon> ngons;

    void separate_ngons() {
        vector<vector3> new_vert_pos;
        for (ngon& f : ngons) {
            for (int& i : f.verts) {
                new_vert_pos.push_back(vert_pos.at(i));
                i=new_vert_pos.size()-1;
            }
        }
        vert_pos=new_vert_pos;
    }

    void set_pos(vector3 t_pos) {
        vector3 delta=pos-t_pos;
        pos=t_pos;

        for (vector3& v : vert_pos) {
            v=v+delta;
        }
    }
};

struct graph_object : public object {
    map<int, int> vert_pos_map;

    void add(graph& c_graph, triangle_ptr n, int m, scalar z_value) {
        assert(m>=0);
        ngons.emplace_back();
        ngon& f=ngons.back();

        f.material=m;
        for (int x=0;x<3;++x) {
            vert_ptr v=c_graph.get_vert(n, x);
            int& i=vert_pos_map.emplace(v.i(), vert_pos.size()).first->second;
            if (i==vert_pos.size()) {
                vert_pos.emplace_back(c_graph.position[v].x(), c_graph.position[v].y(), z_value);
            }
            f.verts.push_back(i);
        }
    }

    template<class func_type, unsigned long long n> void add(func_type func, array<int, n> verts, int m, bool flipped) {
        assert(m>=0);
        ngons.emplace_back();
        ngon& f=ngons.back();

        f.material=m;
        for (int x=0;x<n;++x) {
            int v=verts[x];
            int& i=vert_pos_map.emplace(v, vert_pos.size()).first->second;
            if (i==vert_pos.size()) {
                vert_pos.emplace_back(func(v));
            }
            f.verts.push_back(i);
        }
        if (flipped) {
            reverse(f.verts.begin(), f.verts.end());
        }
    }
};

template<class type> type read(deque<string>& in) {
    assert(!in.empty());
    string c=move(in.front());
    in.pop_front();
    return assert_from_string<type>(c);
}

vector3 read_v3(deque<string>& in) {
    vector3 res;
    for (int x=0;x<3;++x) {
        res[x]=read<scalar>(in);
    }
    return res;
}

vector4 read_v4(deque<string>& in) {
    vector4 res;
    for (int x=0;x<4;++x) {
        res[x]=read<scalar>(in);
    }
    return res;
}

quaternion read_quaternion(deque<string>& in) {
    vector4 res=read_v4(in);
    return quaternion(res.w(), res.x(), res.y(), res.z());
}

void write_v3(ostream& out, vector3 v) {
    out << v[0] << "\n";
    out << v[1] << "\n";
    out << v[2] << "\n";
}

void write_v4(ostream& out, vector4 v) {
    out << v[0] << "\n";
    out << v[1] << "\n";
    out << v[2] << "\n";
    out << v[3] << "\n";
}

void write_quaternion(ostream& out, quaternion v) {
    write_v4(out, vector4(v.x(), v.y(), v.z(), v.w()));
}

vector<object> load(istream& in) {
    //easier to debug. fuck iostream
    deque<string> in_data;
    while (true) {
        string c;
        getline(in, c);
        if (in.good()) {
            in_data.push_back(move(c));
        } else {
            break;
        }
    }

    vector<object> res;
    while (!in_data.empty()) {
        object c;
        c.name=read<string>(in_data);
        c.pos=read_v3(in_data);
        c.rotation=read_quaternion(in_data);

        int num_verts=read<int>(in_data);

        for (int x=0;x<num_verts;++x) {
            c.vert_pos.push_back(read_v3(in_data));
        }

        int num_ngons=read<int>(in_data);

        for (int x=0;x<num_ngons;++x) {
            ngon f;
            f.material=read<int>(in_data);

            int num_ngon_verts=read<int>(in_data);

            for (int y=0;y<num_ngon_verts;++y) {
                f.verts.push_back(read<int>(in_data));
            }

            c.ngons.push_back(f);
        }

        res.push_back(move(c));
    }

    return res;
}

//call multiple times with the same stream to save multiple objects
void save(ostream& out, object& data) {
    out << data.name << "\n";
    write_v3(out, data.pos);
    write_quaternion(out, data.rotation);

    out << data.vert_pos.size() << "\n";
    for (vector3 v : data.vert_pos) {
        write_v3(out, v);
    }

    set<vector<int>> ngon_verts;
    for (ngon& f : data.ngons) {
        vector<int> vs=f.verts;
        sort(vs.begin(), vs.end());
        if (!ngon_verts.insert(vs).second) {
            f.verts.clear();
        }
    }

    out << ngon_verts.size() << "\n";
    for (ngon& f : data.ngons) {
        if (f.verts.empty()) {
            continue;
        }

        out << f.material << "\n";
        out << f.verts.size() << "\n";

        for (int v : f.verts) {
            out << v << "\n";
        }
    }
}


}}