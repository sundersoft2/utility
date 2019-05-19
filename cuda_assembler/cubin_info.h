const int params_initial_offset=0x140;

struct nv_info_entry {
    int id=-1; //2 bytes
    int size=-1; //2 bytes
    string symbol_name; //first 4 bytes of data if present
    string data;

    void append(string& res, elf_file& elf) {
        if (!symbol_name.empty()) {
            assert(data.size()>=4);
            uint32& i=*(uint32*)&(data[0]);
            assert(i==0);
            i=elf.symbols_by_name.at(symbol_name);
        }

        assert(((uint16)id)==id);
        assert(((uint16)size)==size);

        append_string(res, (uint16)id);
        append_string(res, (uint16)size);
        res+=data;
    }

    void assign_null(uint16 t_id) {
        id=t_id;
        size=0;
        symbol_name.clear();
        data.clear();
    }

    void assign_no_data(uint16 t_id, uint16 t_size) {
        id=t_id;
        size=t_size;
        symbol_name.clear();
        data.clear();
    }

    void assign_index_value(uint16 t_id, string t_symbol_name, uint32 t_value) {
        id=t_id;
        size=8;
        symbol_name=t_symbol_name;
        data.clear();
        append_string(data, (uint32)0);
        append_string(data, (uint32)t_value);
    }

    void assign_value(uint16 t_id, uint32 t_value) {
        id=t_id;
        size=4;
        symbol_name.clear();
        data.clear();
        append_string(data, (uint32)t_value);
    }

    void assign_array(uint16 t_id, vector<uint32> t_value) {
        id=t_id;
        size=t_value.size()*4;
        symbol_name.clear();
        data.clear();
        for (uint32 v : t_value) {
            append_string(data, (uint32)v);
        }
    }

    //.nv.info:

    nv_info_entry& max_stack_size(string t_symbol_name, uint32 value) {
        assign_index_value(0x2304, t_symbol_name, value); //EIATTR_MAX_STACK_SIZE
        return *this;
    }

    nv_info_entry& min_stack_size(string t_symbol_name, uint32 value) {
        assign_index_value(0x1204, t_symbol_name, value); //EIATTR_MIN_STACK_SIZE
        return *this;
    }

    nv_info_entry& frame_size(string t_symbol_name, uint32 value) {
        assign_index_value(0x1104, t_symbol_name, value); //EIATTR_FRAME_SIZE
        return *this;
    }

    //.nv.info.[kernel]:

    nv_info_entry& header() {
        assign_null(0x2a01); //EIATTR_SW1850030_WAR
        return *this;
    }

    nv_info_entry& param_cbank(string t_symbol_name, uint32 params_size) {
        assert(params_size<=0x1000);
        uint32 params_offset=params_initial_offset;
        assign_index_value(0x0a04, t_symbol_name, params_offset | (params_size<<16)); //EIATTR_PARAM_CBANK
        return *this;
    }

    nv_info_entry& param_cbank_size(uint32 params_size) {
        assert(params_size<=0x1000);
        assign_no_data(0x1903, params_size); //EIATTR_CBANK_PARAM_SIZE
        return *this;
    }

    nv_info_entry& param_info(uint32 t_ordinal, uint32 t_offset, uint32 t_size) {
        uint32 index=0; //no idea what this does
        uint32 magic=0x1f000; //pointee's logAlignment + space + cbank. cbank is 0x1f and the rest are 0s. no idea what these do

        assert(t_ordinal<=0xffff);
        assert(t_offset<=0xffff);
        assert(t_size<=0x3fff);

        id=0x1704; //EIATTR_KPARAM_INFO
        size=12;
        symbol_name.clear();
        data.clear();
        append_string(data, (uint32)index);
        append_string(data, (uint32)(t_ordinal | (t_offset<<16)));
        append_string(data, (uint32)(magic | (t_size<<18)));

        return *this;
    }

    nv_info_entry& max_reg_count(uint32 value) {
        assert(value<=0xff);
        assign_no_data(0x1b03, value); //EIATTR_MAXREG_COUNT
        return *this;
    }

    nv_info_entry& shfl_offsets(vector<uint32> t_value) {
        assign_array(0x2804, t_value); //EIATTR_COOP_GROUP_INSTR_OFFSETS
        return *this;
    }

    nv_info_entry& exit_offsets(vector<uint32> t_value) {
        assign_array(0x1c04, t_value); //EIATTR_EXIT_INSTR_OFFSETS
        return *this;
    }

    nv_info_entry& s2r_ctaid_offsets(vector<uint32> t_value) {
        assign_array(0x1d04, t_value); //EIATTR_S2RCTAID_INSTR_OFFSETS
        return *this;
    }

    nv_info_entry& required_num_tids(uint32 value) {
        assign_value(0x1004, value); //EIATTR_REQNTID
        return *this;
    }

    nv_info_entry& max_threads(uint32 value) {
        assign_value(0x0504, value); //EIATTR_MAX_THREADS
        return *this;
    }

    nv_info_entry& crs_stack_size(uint32 value) {
        assign_value(0x1e04, value); //EIATTR_CRS_STACK_SIZE
        return *this;
    }

    nv_info_entry& ctaid_z_used() {
        assign_null(0x0401); //EIATTR_CTAIDZ_USED
        return *this;
    }

    nv_info_entry& externs(vector<uint32> t_value) {
        assign_array(0x0f04, t_value); //EIATTR_EXTERNS
        return *this;
    }
};