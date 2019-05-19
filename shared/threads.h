#ifndef ILYA_HEADER_THREADS
#define ILYA_HEADER_THREADS

#include <boost/thread.hpp>

namespace threads {
    using boost::mutex;
    
    template<class targ> class mutex_box {
        targ data;
        mutex lock;
        
        public:
        class box {
            friend class mutex_box;
            mutex_box* container;
            box(mutex_box* t_container) : container(t_container) {}
            box& operator=(mutex_box* t_container) {
                if (container) container->lock.unlock();
                container=t_container;
                return *this;
            }
            
            public:
            struct does_not_exist {};
            
            box() : container(0) {}
            box(box& t_box) : container(t_box.container) { t_box.container=0; }
            box& operator=(box& t_box) {
                if (container) container->lock.unlock();
                container=t_box.container;
                t_box.container=0;
                return *this;
            }
            bool exists() const { return container; }
            void clear() {
                if (container) container->lock.unlock();
                container=0;
            }
            targ& open() const {
                if (!container) throw does_not_exist();
                return container->data;
            }
            targ* operator->() const { return &open(); }
            targ& operator*() const { return open(); }
            ~box() {
                if (container) container->lock.unlock();
            }
        };
        
        mutex_box() {}
        mutex_box(const targ& t_data) : data(t_data) {}
        box take() {
            lock.lock();
            box res(this);
            return res;
        }
        box& take(box& t_box) {
            lock.lock();
            t_box=this;
            return t_box;
        }
        box try_take() {
            if (lock.try_lock()) {
                box res(this);
                return res;
            } else {
                box res;
                return res;
            }
        }
        box& try_take(box& t_box) {
            if (lock.try_lock()) {
                t_box=this;
                return t_box;
            } else {
                t_box.clear();
                return t_box;
            }
        }
    };
    
    template<class type> class shared_data {
        struct t_inst {
            type data;
            int rc;
            mutex lock;
            //
            t_inst() : rc(1) {}
            t_inst(const type& targ) : data(targ), rc(1) {}
            t_inst(type& targ) : data(targ), rc(1) {}
            void use() {
                lock.lock();
                ++rc;
                lock.unlock();
            }
            bool unuse() {
                lock.lock();
                int t_rc=--rc;
                lock.unlock();
                return t_rc==0;
            }
        };
        t_inst *inst;
        void destruct() {
            bool should_del=inst->unuse();
            if (should_del) delete inst;
        }
        
        public:
        shared_data() : inst(new t_inst()) {}
        shared_data(type& targ) : inst(new t_inst(targ)) {}
        shared_data(const type& targ) : inst(new t_inst(targ)) {}
        shared_data(const shared_data& targ) : inst(targ.inst) { inst->use(); }
        //
        shared_data& operator=(const shared_data& targ) {
            destruct();
            inst=targ.inst;
            inst->use();
            return *this;
        }
        type& get() { return inst->data; }
        const type& get() const { return inst->data; }
        type* ptr() { return &(get()); }
        const type* ptr() const { return &(get()); }
        type& operator*() { return get(); }
        const type& operator*() const { return get(); }
        type* operator->() { return &(get()); }
        const type* operator->() const { return &(get()); }
        //
        ~shared_data() { destruct(); }
    };
    
    template<class> class resource;
    
    template<class type> class passthrough {
        template<class> friend class resource;
        struct t_inst {
            type data;
            int passes;
            int uses;
            mutex lock;
            mutex alter_lock;
            //
            t_inst(const type& targ, int t_uses, int t_passes) : data(targ), uses(t_uses), passes(t_passes) {}
            t_inst(type& targ, int t_uses, int t_passes) : data(targ), uses(t_uses), passes(t_passes) {}
            void use() {
                lock.lock();
                ++uses;
                lock.unlock();
            }
            void cross_use() {
                alter_lock.lock();
                alter_lock.unlock();
                lock.lock();
                ++uses;
                lock.unlock();
            }
            bool unuse() {
                lock.lock();
                int t_uses=--uses;
                int t_passes=passes;
                lock.unlock();
                return t_uses==0 && t_passes==0;
            }
            void pass() {
                lock.lock();
                ++passes;
                lock.unlock();
            }
            bool unpass() {
                lock.lock();
                int t_uses=uses;
                int t_passes=--passes;
                lock.unlock();
                return t_uses==0 && t_passes==0;
            }
        };
        t_inst *inst;
        bool altering;
        void destruct() {
            if (inst) {
                if (altering) commit();
                bool should_del=inst->unpass();
                if (should_del) delete inst;
            }
        }
        
        public:
        passthrough() : inst(0), altering(0) {}
        passthrough(type& targ) : altering(0) { inst=new t_inst(targ, 0, 1); }
        passthrough(const type& targ) : altering(0) { inst=new t_inst(targ, 0, 1); }
        passthrough(const resource<type>& targ) : inst(targ.inst), altering(0) { if (inst) inst->pass(); }
        passthrough(const passthrough& targ) : inst(targ.inst), altering(0) { if (inst) inst->pass(); }
        //
        passthrough& operator=(const passthrough& targ) {
            destruct();
            inst=targ.inst;
            if (inst) inst->pass();
            return *this;
        }
        passthrough& operator=(const resource<type>& targ) {
            destruct();
            inst=targ.inst;
            if (inst) inst->pass();
            return *this;
        }
        //
        bool is_null() const { return !inst; }
        type* alter() {
            if (altering) return &(inst->data);
            if (!inst) return 0;
            inst->alter_lock.lock();
            if (inst->uses!=0) {
                inst->alter_lock.unlock();
                return 0;
            }
            altering=1;
            return &(inst->data);
        }
        void commit() {
            altering=0;
            inst->alter_lock.unlock();
        }
        //
        ~passthrough() { destruct(); }
    };
    
    template<class type> class resource {
        typedef typename passthrough<type>::t_inst t_inst;
        t_inst *inst;
        bool altering;
        void destruct() {
            if (inst) {
                if (altering) commit();
                bool should_del=inst->unuse();
                if (should_del) delete inst;
            }
        }
        
        public:
        resource() : inst(0), altering(0) {}
        resource(type& targ) : altering(0) { inst=new t_inst(targ, 1, 0); }
        resource(const type& targ) : altering(0) { inst=new t_inst(targ, 1, 0); }
        resource(const passthrough<type>& targ) : inst(targ.inst), altering(0) {
            if (inst) {
                if (targ.altering) inst->use(); else inst->cross_use();
            }
        }
        resource(const resource& targ) : inst(targ.inst), altering(0) { if (inst) inst->use(); }
        //
        resource& operator=(const passthrough<type>& targ) {
            destruct();
            inst=targ.inst;
            if (inst) {
                if (targ.altering) inst->use(); else inst->cross_use();
            }
            return *this;
        }
        resource& operator=(const resource& targ) {
            destruct();
            inst=targ.inst;
            if (inst) inst->use();
            return *this;
        }
        //
        bool is_null() const { return !inst; }
        type* alter() {
            if (altering) return &(inst->data);
            if (!inst) return 0;
            inst->alter_lock.lock();
            if (inst->uses!=1) {
                inst->alter_lock.unlock();
                return 0;
            }
            altering=1;
            return &(inst->data);
        }
        void commit() {
            altering=0;
            inst->alter_lock.unlock();
        }
        const type& get() const { return inst->data; }
        const type* ptr() const { return &(get()); }
        const type& operator*() const { return get(); }
        const type* operator->() const { return &(get()); }
        //
        ~resource() { destruct(); }
    };
}

#endif
