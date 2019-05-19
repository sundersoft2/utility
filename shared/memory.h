#ifndef ILYA_SHARED_HEADER_MEMORY
#define ILYA_SHARED_HEADER_MEMORY

//#include <deque>
//#include <vector>
//#include <list>
#include <memory>
//#include <limits>
//#include <cstring>
//#include <boost/thread.hpp>
//#include <boost/interprocess/sync/scoped_lock.hpp>

//desired file structure:
//memory_base.h -> clone_ptr
//memory_bulk.h -> bulk allocator
//memory.h -> includes all memory headers

namespace memory {
    using std::move;
    using std::unique_ptr;
    //using std::deque;
    //using std::vector;
    //using std::list;
    //using std::size_t;
    //using std::ptrdiff_t;
    //using std::allocator;
    //using boost::mutex;
    //using boost::interprocess::scoped_lock;
    
    template<class type> class clone_ptr {
        unique_ptr<type> ptr;
        unique_ptr<type> do_cloning() const { return ptr? ptr->clone() : 0; }
        
        public:
        clone_ptr() {}
        clone_ptr(const type& targ) : ptr(targ.clone()) {}
        clone_ptr(const clone_ptr& targ) : ptr(targ.do_cloning()) {}
        clone_ptr(clone_ptr&& targ) : ptr(move(targ.ptr)) { targ.ptr.reset(); }
        //
        clone_ptr& operator=(const clone_ptr& targ) {
            if (this!=&targ) {
                ptr=targ.do_cloning();
            }
            return *this;
        }
        clone_ptr& operator=(clone_ptr&& targ) {
            if (this!=&targ) {
                ptr=move(targ.ptr);
                targ.ptr.reset();
            }
            return *this;
        }
        //
        type& operator*() { return *ptr; }
        const type& operator*() const { return *ptr; }
        type* operator->() { return &*ptr; }
        const type* operator->() const { return &*ptr; }
        bool is_null() const { return !ptr; }
        //
        bool operator==(const clone_ptr& targ) const {
            if (is_null()) return targ.is_null();
            if (targ.is_null()) return 0;
            return **this==*targ;
        }
        bool operator!=(const clone_ptr& targ) const { return !(*this==targ); }
        bool operator<(const clone_ptr& targ) const {
            if (is_null()) return !targ.is_null();
            if (targ.is_null()) return 0;
            return **this<*targ;
        }
        bool operator<=(const clone_ptr& targ) const {
            if (is_null()) return 1;
            if (targ.is_null()) return 0;
            return **this<=*targ;
        }
        bool operator>(const clone_ptr& targ) const { return !(*this<=targ); }
        bool operator>=(const clone_ptr& targ) const { return !(*this<targ); }
        //
        ~clone_ptr() { delete ptr; }
    };
    
    /*template<class> class const_ref_count;
    
    template<class type> class ref_count { //replace with boost::intrusive_ptr
        template<class> friend class const_ref_count;
        struct ref_base {
            int num;
            type targ;
            ref_base() : num(1), targ() {}
            ref_base(type& t_targ) : num(1), targ(t_targ) {}
            ref_base(const type& t_targ) : num(1), targ(t_targ) {}
            //
            void use() {
                ++num;
            }
            bool unuse() {
                --num;
                return num==0;
            }
        };
        ref_base *ref;
        
        public:
        ref_count() { ref=new ref_base; }
        ref_count(type& targ) { ref=new ref_base(targ); }
        ref_count(const type& targ) { ref=new ref_base(targ); }
        ref_count(const ref_count& targ) {
            ref=targ.ref;
            ref->use();
        }
        ref_count& operator=(const ref_count& targ) {
            if (targ.ref==ref) return *this;
            if (ref->unuse()) delete ref;
            ref=targ.ref;
            ref->use();
            return *this;
        }
        //
        type& operator*() const { return ref->targ; }
        type* operator->() const { return &(ref->targ); }
        //
        ~ref_count() {
            if (ref->unuse()) delete ref;
        }
    };
    
    template<class type> class const_ref_count {
        typedef typename ref_count<type>::ref_base ref_base;
        ref_base *ref;
        
        public:
        const_ref_count() { ref=new ref_base; }
        const_ref_count(type& targ) { ref=new ref_base(targ); }
        const_ref_count(const type& targ) { ref=new ref_base(targ); }
        const_ref_count(const ref_count<type>& targ) {
            ref=targ.ref;
            ref->use();
        }
        const_ref_count(const const_ref_count& targ) {
            ref=targ.ref;
            ref->use();
        }
        const_ref_count& operator=(const const_ref_count& targ) {
            if (targ.ref==ref) return *this;
            if (ref->unuse()) delete ref;
            ref=targ.ref;
            ref->use();
            return *this;
        }
        //
        const type& operator*() const { return ref->targ; }
        const type* operator->() const { return &(ref->targ); }
        //
        ~const_ref_count() {
            if (ref->unuse()) delete ref;
        }
    };
    
    class bulk_memory { //seperate into a different file
        deque<vector<char> > blocks;
        bulk_memory(const bulk_memory& targ) {}
        void operator=(const bulk_memory& targ) {}
        //
        int round_pow2(int targ, int base=1) {
            while (base<targ) base<<=1;
            return base;
        }
        
        public:
        bulk_memory() : blocks(1) {}
        bulk_memory(size_t size) : blocks(1) {
            if (size!=0) {
                size=round_pow2(size, 1024);
                blocks.back().reserve(size);
            }
        }
        
        void* allocate(size_t size) {
            if (size&15) size=(size+16)&(~15);
            vector<char> *cur=&(blocks.back());
            if (cur->capacity()<cur->size()+size) {
                if (cur->size()==0) {
                    cur->reserve(round_pow2(size, 1024));
                } else {
                    size_t old_size=cur->capacity();
                    blocks.push_back(vector<char>());
                    cur=&(blocks.back());
                    cur->reserve(round_pow2(size, old_size<<1));
                }
            }
            void *res=&(*(cur->end()));
            cur->resize(cur->size()+size);
            return res;
        }
        
        void defragment() {
            if (blocks.size()==1) {
                blocks.back().clear();
            } else {
                size_t size_sum=0;
                for (deque<vector<char> >::iterator c=blocks.begin();c!=blocks.end();++c) {
                    size_sum+=c->size();
                }
                size_sum=round_pow2(size_sum);
                blocks.clear();
                blocks.resize(1);
                blocks.back().reserve(size_sum);
            }
        }
        
        size_t total_size() {
            size_t size_sum=0;
            for (deque<vector<char> >::iterator c=blocks.begin();c!=blocks.end();++c) {
                size_sum+=c->size();
            }
            return size_sum;
        }
    };
    
    class bulk_recycler {
        size_t last_size;
        vector<bulk_memory*> cache;
        deque<bulk_memory*> free_cache;
        mutex lock;
        int last_empty;
        static const int max_last_empty=10;
        //
        bulk_recycler(const bulk_recycler& targ) {}
        void operator=(const bulk_recycler& targ) {}
        
        public:
        bulk_recycler() : last_size(0), last_empty(0) {}
        
        bulk_memory& get() {
            scoped_lock<mutex> locker(lock);
            //cout << "GET: " << free_cache.size() << ", " << cache.size() << " : ";
            ++last_empty;
            bulk_memory *res;
            if (free_cache.empty()) {
                res=::new bulk_memory(last_size);
                cache.push_back(res);
                last_empty=0;
            } else {
                res=free_cache.back();
                free_cache.pop_back();
                if (free_cache.empty()) last_empty=0;
            }
            if (last_empty>=max_last_empty) {
                bulk_memory *del=free_cache.front();
                free_cache.pop_front();
                vector<bulk_memory*>::iterator del_pos;
                for (del_pos=cache.begin();del_pos!=cache.end();++del_pos) {
                    if (*del_pos==del) break;
                }
                ::delete *del_pos;
                cache.erase(del_pos);
                last_empty=0;
            }
            //cout << res << "\n";
            return *res;
        }
        
        void put(bulk_memory& targ) {
            //cout << "PUT: " << free_cache.size() << ", " << cache.size() << ":" << (&targ) << "(" << targ.total_size() <<  ")" << "\n";
            scoped_lock<mutex> locker(lock);
            targ.defragment();
            free_cache.push_back(&targ);
            size_t size=targ.total_size();
            last_size=size;
        }
        
        ~bulk_recycler() {
            for (vector<bulk_memory*>::iterator c=cache.begin();c!=cache.end();++c) ::delete *c;
        }
    };
    
    class bulk_recycle {
        bulk_memory &inst;
        bulk_recycler &recycle;
        
        public:
        bulk_recycle() : inst(*((bulk_memory*)0)), recycle(*((bulk_recycler*)0)) {}
        bulk_recycle(bulk_memory& t_inst, bulk_recycler& t_recycle) : inst(t_inst), recycle(t_recycle) {}
        
        bulk_memory& get() { return inst; }
        
        ~bulk_recycle() { recycle.put(inst); }
    };
}

void* operator new(size_t size, memory::bulk_memory& targ) { return targ.allocate(size); }
void* operator new[](size_t size, memory::bulk_memory& targ) { return targ.allocate(size); }
void operator delete(void* mem, memory::bulk_memory& targ) {}
void operator delete[](void* mem, memory::bulk_memory& targ) {}

namespace memory {
    template<class type> class bulk_ref_count {
        struct ref_base {
            int num;
            type targ;
            ref_base() : num(1), targ() {}
            ref_base(type& t_targ) : num(1), targ(t_targ) {}
            ref_base(const type& t_targ) : num(1), targ(t_targ) {}
            //
            void use() {
                ++num;
            }
            bool unuse() {
                --num;
                return num==0;
            }
        };
        ref_base *ref;
        
        public:
        bulk_ref_count() : ref(0) {}
        bulk_ref_count(bulk_memory& mem) { ref=new(mem) ref_base; }
        bulk_ref_count(type& targ, bulk_memory& mem) { ref=new(mem) ref_base(targ); }
        bulk_ref_count(const type& targ, bulk_memory& mem) { ref=new(mem) ref_base(targ); }
        bulk_ref_count(const bulk_ref_count& targ) {
            ref=targ.ref;
            if (ref) ref->use();
        }
        bulk_ref_count& operator=(const bulk_ref_count& targ) {
            if (targ.ref==ref) return *this;
            if (ref && ref->unuse()) ref->~ref_base();
            ref=targ.ref;
            if (ref) ref->use();
            return *this;
        }
        //
        type& operator*() const { return ref->targ; }
        type* operator->() const { return &(ref->targ); }
        //
        ~bulk_ref_count() {
            if (ref && ref->unuse()) ref->~ref_base();
        }
    };
    
    template<class type> struct bulk_auto_ptr_ref {
        type *ptr;
        explicit bulk_auto_ptr_ref(type* t_ptr) : ptr(t_ptr) {}
    };
    
    template<class type> class bulk_auto_ptr {
        type *ptr;
        
        public:
        typedef type element_type;
        
        explicit bulk_auto_ptr(type* t_ptr=0) : ptr(t_ptr) {}
        bulk_auto_ptr(bulk_auto_ptr_ref<type> targ) : ptr(targ.ptr) {}
        template<class type2> bulk_auto_ptr(bulk_auto_ptr<type2>& targ) : ptr(targ.release()) {}
        //
        template<class type2> bulk_auto_ptr& operator=(bulk_auto_ptr<type2>& targ) {
            reset(targ.release());
            return *this;
        }
        bulk_auto_ptr& operator=(bulk_auto_ptr_ref<type> targ) {
            if (ptr!=targ.ptr) {
                if (ptr) ptr->~type();
                ptr=targ.ptr;
            }
            return *this;
        }
        template<class type2> operator bulk_auto_ptr_ref<type2>() { return bulk_auto_ptr_ref<type2>(release()); }
        template<class type2> operator bulk_auto_ptr<type2>() { return bulk_auto_ptr<type2>(release()); }
        
        type& operator*() const { return *ptr; }
        type* operator->() const { return ptr; }
        
        type* get() const { return ptr; }
        type* release() {
            type* res=ptr;
            ptr=0;
            return res;
        }
        void reset(type* targ=0) {
            if (ptr!=targ) {
                if (ptr) ptr->~type();
                ptr=targ;
            }
        }
        
        ~bulk_auto_ptr() { if (ptr) ptr->~type(); }
    };
    
    template<class type> class bulk_allocator {
        public:
        bulk_memory* mem;
        
        typedef type value_type;
        typedef type* pointer;
        typedef const type* const_pointer;
        typedef type& reference;
        typedef const type& const_reference;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        template<class type2> struct rebind {
            typedef bulk_allocator<type2> other;
        };
        
        bulk_allocator(bulk_memory& targ) : mem(&targ) {}
        bulk_allocator(const bulk_allocator& targ) : mem(targ.mem) {}
        template<class type2> bulk_allocator(const bulk_allocator<type2>& targ) : mem(targ.mem) {}
        //
        pointer allocate(size_type num, typename allocator<void>::const_pointer=0) { return (type*)(mem->allocate(num*sizeof(type))); }
        size_type max_size() const { return std::numeric_limits<size_type>::max()/sizeof(type); }
        pointer address(reference targ) const { return &targ; }
        const_pointer address(const_reference targ) const { return &targ; }
        void construct(pointer ptr, const_reference targ) { ::new(ptr) type(targ); }
        void destroy(pointer ptr) { ptr->~type(); }
        void deallocate(pointer ptr, size_type num) {}
        bool operator==(const bulk_allocator& targ) { return mem==targ.mem; }
        bool operator!=(const bulk_allocator& targ) { return !(*this==targ); }
    };
    
    template<class type> struct bulk {
        typedef std::vector<type, bulk_allocator<type> > vector;
        typedef std::deque<type, bulk_allocator<type> > deque;
        typedef std::list<type, bulk_allocator<type> > list;
        //
        static vector make_vector(bulk_memory& mem) { return vector(bulk_allocator<type>(mem)); }
        static deque make_deque(bulk_memory& mem) { return deque(bulk_allocator<type>(mem)); }
        static list make_list(bulk_memory& mem) { return list(bulk_allocator<type>(mem)); }
    };*/
}

#endif
