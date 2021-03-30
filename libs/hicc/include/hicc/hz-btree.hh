//
// Created by Hedzr Yeh on 2021/3/4.
//

#ifndef HICC_CXX_HZ_BTREE_HH
#define HICC_CXX_HZ_BTREE_HH

#include <list>
#include <vector>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <algorithm>
#include <functional>
#include <memory>

#include <type_traits>
#include <typeinfo>

#include "hz-dbg.hh"
#include "hz-defs.hh"
#include "hz-log.hh"
#include "hz-util.hh"


namespace hicc::btree {

    // using counter_type = int;
    using counter_type = short;
    // using counter_type = std::size_t;

    /**
     * @brief provides B-tree data structure in generic style.
     * @tparam T the element/key
     * @tparam Comp 
     * @details The degree, aka the Order of a B-tree, is the maximal children
     * count. In our B-tree model, a node has:
     * 
     *     key:      [degree/2-1, degree-1]
     *     children: [degree/2, degree]
     * 
     * For a 7-order B-tree, each node has between [7/2] and 7 children, so
     * it contains 3,4,5 and 6 keys. NOTE [7/2] is rounded up ceiling.
     * In coding with C/C++:
     * 
     *     const int max_payloads = _degree - 1;
     *     const int min_payloads = max_payloads / 2;
     *     const int max_children = _degree;
     *     const int min_children = (_degree + 1) / 2;
     *     const int M = _degree / 2;
     *     const int _M = min_payloads + 1;
     *     assert(M == _M);
     */
    template<class T, class Comp = std::less<T>>
    class btree {
    public:
        btree(int degree = 4)
            : _degree(degree)
            , _size(0)
            , _root(nullptr) {}
        virtual ~btree() { clear(); }

    public:
        struct node;
        struct traversal_context;

        using self_type = btree;
        using size_type = std::size_t;

        using node_ptr = node *;
        using node_ref = node &;
        using const_node_ptr = node const *;
        using const_node_ref = node const &;
        using node_sptr = std::shared_ptr<node>;

        using elem_type = T;
        using elem_ptr = elem_type *;
        using const_elem_ptr = elem_type const *;
        using elem_ref = elem_type &;
        using const_elem_ref = elem_type const &;

        using visitor_l = std::function<bool(traversal_context const &)>;

        struct traversal_context {
            const_node_ref curr;
            const_elem_ptr el;
            int level;
            int index;
            int loop_base;
            int parent_ptr_index;
            int abs_index;
            bool level_changed;
            bool node_changed;
            bool parent_ptr_index_changed;
        };

        using position = std::tuple<bool /*ok*/, const_node_ref, int>;
        using lite_position = std::tuple<int, const_node_ref>;
        using lite_ptr_position = std::tuple<int, const_node_ptr>;

        struct position_t {
            const_node_ptr ptr;
            int pos; // pointer to the payload or pointers
            const_elem_ptr get() const { return ptr->_payloads[pos]; }
            elem_ptr get() { return const_cast<node_ptr>(ptr)->_payloads[pos]; }
            const_node_ptr operator->() const { return ptr; }
            position_t &operator=(const position_t &o) {
                ptr = o.ptr;
                pos = o.pos;
                return *this;
            }
            position_t &advance_to_first_child() {
                ptr = ptr->child(0);
                return *this;
            }
            position_t &advance_to_last_child() {
                ptr = ptr->child(ptr->payload_count());
                // for (auto t = Degree - 1; t >= 0; t--) {
                //     if (auto *p = ptr->_pointers[t]; p) {
                //         ptr = p;
                //         break;
                //     }
                // }
                return *this;
            }
            position_t last_child() const {
                return {ptr->child(ptr->payload_count()), 0};
                // for (auto t = Degree - 1; t >= 0; t--) {
                //     if (auto *ptr = _pointers[t]; ptr)
                //         return {this, t};
                // }
                // return {nullptr, -1};
            }
            position_t last_one() const {
                return {ptr, ptr->payload_count() - 1};
                // for (auto t = Degree - 1 - 1; t >= 0; t--) {
                //     if (auto *ptr = _payloads[t]; ptr)
                //         return {this, t};
                // }
                // return {nullptr, -1};
            }
            position_t first_child() const { return {ptr, 0}; }
            position_t first_one() const { return {ptr, 0}; }
            bool valid() const { return ptr && pos >= 0 && pos << ptr->_degree; }
            operator bool() const { return valid(); }
            bool operator!() const { return !valid(); }
        };

        struct node {
            int _degree;
            int _count;
            elem_ptr *_payloads;
            node_ptr *_pointers;
            friend class btree;

            node(int degree)
                : _degree(degree)
                , _count(0)
                , _payloads(nullptr)
                , _pointers(nullptr) {
                if (_degree > 0) {
                    const int max_payloads = _degree - 1;
                    // const int min_payloads = max_payloads / 2;
                    _payloads = new elem_ptr[max_payloads];
                    _pointers = new node_ptr[_degree];
                    for (int i = 0; i < max_payloads; i++) _payloads[i] = nullptr;
                    for (int i = 0; i <= max_payloads; i++) _pointers[i] = nullptr;
                }
            }
            ~node() {
                if (_degree > 0) {
#if defined(_DEBUG)
                    auto str = to_string();
                    hicc_verbose_debug("    ~node() for %p, %d payloads: [%s]", this, _count, str.c_str());
#endif
                    // const int max_payloads = _degree - 1;
                    // const int min_payloads = max_payloads / 2;
                    for (int i = 0; i < _count; i++)
                        if (auto *p = _payloads[i]; p) delete p;
                    for (int i = 0; i <= _count; i++)
                        if (auto *p = _pointers[i]; p) delete p;
                    delete[] _pointers;
                    delete[] _payloads;
#if defined(_DEBUG)
                    hicc_verbose_debug("    ~node() for %p, %d payloads: [%s] END", this, _count, str.c_str());
#endif
                }
            }
            node_ptr reset_for_delete() {
                const int max_payloads = _degree - 1;
                // const int min_payloads = max_payloads / 2;
                for (int i = 0; i < max_payloads; i++) _payloads[i] = nullptr;
                for (int i = 0; i <= max_payloads; i++) _pointers[i] = nullptr;
                return this;
            }

        public:
            void insert_non_full(elem_ptr el) {
                hicc_debug("    insert_non_full(%s) for node [%s]", elem_to_string(el).c_str(), to_string().c_str());
                int idx = _count - 1;
                if (is_leaf()) {
                    while (idx >= 0 && is_less_than(el, _payloads[idx])) {
                        _payloads[idx + 1] = _payloads[idx];
                        idx--;
                    }
                    _payloads[idx + 1] = el;
                    _count++;
                    return;
                }

                const int max_payloads = _degree - 1;
                while (idx >= 0 && is_less_than(el, _payloads[idx])) idx--;
                if (_pointers[idx + 1]->_count == max_payloads) {
                    _split_child(idx + 1, _pointers[idx + 1]);
                    if (*_payloads[idx + 1] < *el) idx++;
                }
                _pointers[idx + 1]->insert_non_full(el);
            }

        private:
            void _split_child(int idx, node_ptr y) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);

                hicc_debug("    _split_child(%d) for node [%s]", idx, y->to_string().c_str());

                node_ptr z = new node(y->_degree);
                z->_count = min_payloads;
                for (int j = 0; j < min_payloads; j++) z->_payloads[j] = y->_payloads[j + _M];
                if (!y->is_leaf())
                    for (int j = 0; j < _M; j++) z->_pointers[j] = y->_pointers[j + _M];

                y->_count = min_payloads;

                for (int j = _count; j >= idx + 1; j--) _pointers[j + 1] = _pointers[j];
                _pointers[idx + 1] = z;
                for (int j = _count - 1; j >= idx; j--) _payloads[j + 1] = _payloads[j];
                _payloads[idx] = y->_payloads[_M - 1];
                _count++;

                for (int j = y->_count; j < max_payloads; j++) y->_payloads[j] = nullptr;
                for (int j = y->_count + 1; j < _degree; j++) y->_pointers[j] = nullptr;
            }

        public:
            elem_ptr remove(const_elem_ref el) { return _remove(&el); }
            elem_ptr remove(elem_ptr el) { return _remove(el); }

        private:
            /**
             * @brief remove the key 'el' from the sub-tree rooted with this node.
             * @param el the removing key
             * @return the remove key, it should be euqal to 'el' but live with non-'const'
             */
            elem_ptr _remove(const_elem_ptr el) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);
                UNUSED(min_payloads, max_payloads);

                int idx = first_of_insertion_position(el);
                hicc_debug("    _remove(%s) from node [%s] | pos found: %d | D=%d, _M=%d, [%d, %d]", elem_to_string(el).c_str(), to_string().c_str(), idx, _degree, _M, min_payloads, max_payloads);

                // while the key to be removed is present in this node
                if (idx < _count && is_equal_to(el, _payloads[idx])) {
                    // erase it directly
                    if (is_leaf())
                        return _remove_at(idx);
                    // or advance the focus into the internal node and
                    // try removing it recursively.
                    return _remove_from_internal_node(idx);
                }

                // if this node is a leaf node, then the key is not present in tree
                if (is_leaf())
                    return nullptr; // nothing to do

                // The key to be removed is present in the sub-tree rooted
                // with this node, the flag 'rightest' indicates whether the
                // key is present in the sub-tree rooted with the last
                // child of this node
                bool rightest = idx == _count;

                // if the child where the key is supposed to exist has less that _M
                // keys, fill it at first. So we get a half-full node at least for
                // the removing action in the child tree later.
                if (_pointers[idx]->_count < _M)
                    _fill(idx);

                // if the last child has been merged/filled, it must have merged
                // with the previous child and so we recurse on the (idx-1)th
                // child.
                // Or, we recurse on the (idx)th child which now has at least _M
                // keys.
                if (rightest && idx > _count)
                    return _pointers[idx - 1]->_remove(el);
                return _pointers[idx]->_remove(el);
            }

            /**
             * @brief remove the idx-th key from this node, if it is a leaf node
             * @param idx 
             * @return the removed key
             */
            elem_ptr _remove_at(int idx) {
                elem_ptr removing = get_el_ptr(idx);
                hicc_verbose_debug("    _remove_at(%d) (target=%s) for node [%s], cnt=%d", idx, elem_to_string(removing).c_str(), to_string().c_str(), _count);
                for (int i = idx + 1; i < _count; ++i) _payloads[i - 1] = _payloads[i];
                _payloads[_count-- - 1] = nullptr;
                hicc_verbose_debug("    _remove_at(%d) (target=%s) END. node [%s], cnt=%d", idx, elem_to_string(removing).c_str(), to_string().c_str(), _count);
                return removing;
            }

            /**
             * @brief remove the idx-th key from this node - which is a non-leaf node
             * @param idx 
             * @return the removed key
             */
            elem_ptr _remove_from_internal_node(int idx) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);
                UNUSED(min_payloads, max_payloads);

                elem_ptr k = _payloads[idx];
                hicc_debug("    _remove_from_internal_node(%d) [target=%s] for node [%s]", idx, elem_to_string(k).c_str(), to_string().c_str());

                if (node_ptr p = _pointers[idx]; p->_count >= _M) {
                    // If the child that precedes k (Children[idx]) has at least
                    // _M keys, find the predecessor 'pred' of k in the subtree
                    // rooted at Children[idx].
                    // And, replace k by pred.
                    // And, recursively delete pred in Children[idx]
                    elem_ptr pred = _predecessor(idx);
                    _payloads[idx] = pred;
                    p->_remove(pred);
                    return k;
                }

                if (node_ptr p = _pointers[idx + 1]; p->_count >= _M) {
                    // If the child that successor k (Children[idx+1]) has at least
                    // _M keys, find the successor 'succ' of k in the subtree
                    // rooted at Children[idx+1].
                    // And, replace k by succ.
                    // And, recursively delete succ in Children[idx]
                    elem_ptr succ = _successor(idx);
                    _payloads[idx] = succ;
                    p->_remove(succ);
                    return k;
                }

                // If both Children[idx] and Children[idx+1] has less than _M keys,
                // merge k and all of Children[idx+1] into Children[idx].
                // After merged, Children[idx] will contain MAX-PAYLOADS keys.
                // the node Children[idx+1] will be free after k was deleted from
                // Children[idx] recursively.
                _merge(idx);
                return _pointers[idx]->_remove(k);
            }

            /**
             * @brief find and return the predecessor key of this[idx]
             * @param idx 
             * @return the last key of the leaf
             */
            elem_ptr _predecessor(int idx) {
                node_ptr cur = _pointers[idx];
                while (!cur->is_leaf())
                    cur = cur->_pointers[cur->_count];
                return cur->_payloads[cur->_count - 1];
            }

            /**
             * @brief find and return the successor key of this[idx]
             * @param idx 
             * @return the first key of the leaf
             */
            elem_ptr _successor(int idx) {
                node_ptr cur = _pointers[idx + 1];
                while (!cur->is_leaf())
                    cur = cur->_pointers[0];
                return cur->_payloads[0];
            }

            /**
             * @brief to merge Children[idx] with Children[idx+1], and pull 
             * an element from parent node (this node) too.
             * @param idx an index in this node (parent node).
             */
            void _merge(int idx) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);
                UNUSED(min_payloads, max_payloads);

                node_ptr child = _pointers[idx];
                node_ptr sibling = _pointers[idx + 1];
                hicc_debug("    _merge(%d) for node [%s] and sibling [%s]", idx, child->to_string().c_str(), sibling->to_string().c_str());

                assert(child->_count == _M - 1);
                child->_payloads[_M - 1] = _payloads[idx];

                for (int i = 0; i < sibling->_count; ++i) child->_payloads[i + _M] = sibling->_payloads[i];
                if (!child->is_leaf())
                    for (int i = 0; i <= sibling->_count; ++i) child->_pointers[i + _M] = sibling->_pointers[i];

                for (int i = idx + 1; i < _count; ++i) _payloads[i - 1] = _payloads[i];
                for (int i = idx + 2; i <= _count; ++i) _pointers[i - 1] = _pointers[i];
                _pointers[_count] = nullptr;
                _payloads[_count - 1] = nullptr;

                child->_count += sibling->_count + 1;
                _count--;

                delete sibling->reset_for_delete();
            }

            /**
             * @brief to fill Children[idx] which has less than _M-1 keys.
             * @param idx 
             */
            void _fill(int idx) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);
                UNUSED(min_payloads, max_payloads);

                hicc_debug("    _fill(%d) for node [%s]", idx, to_string().c_str());

                if (idx != 0 && _pointers[idx - 1]->_count >= _M)
                    _rotate_from_left(idx);
                else if (idx != _count && _pointers[idx + 1]->_count >= _M)
                    _rotate_from_right(idx);
                else {
                    if (idx != _count)
                        _merge(idx);
                    else
                        _merge(idx - 1);
                }
            }

            /**
             * @brief to rotate a key from Children[idx-1] to parent[idx], and pull
             * the replaced key parent[idx] down to Children[idx]
             * @param idx 
             */
            void _rotate_from_left(int idx) {
                node_ptr child = _pointers[idx];
                node_ptr sibling = _pointers[idx - 1];
                hicc_debug("    _rotate_from_left(%d) for node [%s] and sibling [%s]", idx, child->to_string().c_str(), sibling->to_string().c_str());

                for (int i = child->_count - 1; i >= 0; --i)
                    child->_payloads[i + 1] = child->_payloads[i];

                if (!child->is_leaf()) {
                    for (int i = child->_count; i >= 0; --i)
                        child->_pointers[i + 1] = child->_pointers[i];
                }

                child->_payloads[0] = _payloads[idx - 1];

                if (!child->is_leaf()) {
                    child->_pointers[0] = sibling->_pointers[sibling->_count];
                    sibling->_pointers[sibling->_count] = nullptr;
                }

                _payloads[idx - 1] = sibling->_payloads[sibling->_count - 1];
                sibling->_payloads[sibling->_count - 1] = nullptr;

                child->_count++;
                sibling->_count--;
            }

            /**
             * @brief to rotate a key from Children[idx+1] to parent[idx], and pull
             * the replaced key parent[idx] down to Children[idx]
             * @param idx 
             */
            void _rotate_from_right(int idx) {
                node_ptr child = _pointers[idx];
                node_ptr sibling = _pointers[idx + 1];
                hicc_debug("    _rotate_from_right(%d) for node [%s] and sibling [%s]", idx, child->to_string().c_str(), sibling->to_string().c_str());

                child->_payloads[(child->_count)] = _payloads[idx];

                if (!child->is_leaf())
                    child->_pointers[(child->_count) + 1] = sibling->_pointers[0];

                _payloads[idx] = sibling->_payloads[0];

                for (int i = 1; i < sibling->_count; ++i)
                    sibling->_payloads[i - 1] = sibling->_payloads[i];
                sibling->_payloads[sibling->_count - 1] = nullptr;
                if (!sibling->is_leaf()) {
                    for (int i = 1; i <= sibling->_count; ++i)
                        sibling->_pointers[i - 1] = sibling->_pointers[i];
                    sibling->_pointers[sibling->_count] = nullptr;
                }

                child->_count++;
                sibling->_count--;
            }

            int first_of_insertion_position(const_elem_ptr el) const {
                int idx = 0;
                while (idx < _count && is_less_than(_payloads[idx], el))
                    ++idx;
                return idx;
            }

        public:
            bool is_leaf() const { return _pointers[0] == nullptr; }

            const_elem_ref get_el(int index = 0) const { return *const_cast<node_ptr>(this)->get_el_ptr(index); }
            elem_ref get_el(int index = 0) { return *get_el_ptr(index); }
            elem_ptr get_el_ptr(int index = 0) {
                return (index < 0 || index > _degree - 1) ? &_null_elem()
                                                          : (_payloads[index] ? (_payloads[index]) : &_null_elem());
            }
            const_elem_ptr get_el_ptr(int index = 0) const { return const_cast<node_ptr>(this)->get_el_ptr(index); }

            void set_el(int index, elem_ptr a) { _payloads[index] = a; }

            const_node_ptr child(int i) const { return (i >= 0 && i <= _degree) ? _pointers[i] : nullptr; }
            node_ptr child(int i) { return (i >= 0 && i <= _degree) ? _pointers[i] : nullptr; }

            bool can_get_el(int index) const {
                if (index < 0 || index > _degree - 1) return false;
                return _payloads[index] != nullptr;
            }
            bool can_get_child(int index) const {
                if (index < 0 || index > _degree) return false;
                return _pointers[index] != nullptr;
            }

            elem_type const &operator[](int index) const {
                if (index >= 0 && index < _degree - 1)
                    return _payloads[index] ? *_payloads[index] : _null_elem();
                return _null_elem();
            }

            elem_type &operator[](int index) {
                if (index >= 0 && index < _degree - 1)
                    return _payloads[index] ? *_payloads[index] : _null_elem();
                return _null_elem();
            }

            bool operator!() const { return is_null(*this); }
            operator bool() const { return !is_null(*this); }

            // compare the pointers strictly
            bool operator==(const_node_ref &rhs) const {
                if (rhs._degree == 0 && _degree == 0) return true;
                if (rhs._degree == 0 || _degree == 0) return false;

                for (int k = 0; k < _degree - 1; ++k) {
                    if (_payloads[k] != rhs._payloads[k])
                        return false;
                }
                for (int k = 0; k < _degree; ++k) {
                    if (_pointers[k] != rhs._pointers[k])
                        return false;
                }
                if (_count != rhs._count) return false;
                return true;
            }

            friend std::ostream &operator<<(std::ostream &os, node &o) {
                os << o.to_string();
                return os;
            }

            static T &_null_elem() {
                static T _d{};
                return _d;
            }

            static node_ref _null_node() {
                static node _d{0};
                return _d;
            }

            static bool is_null(T const &data) { return data == _null_elem(); }
            static bool is_null(node const &data) { return data == _null_node(); }

            static Comp &comparer() {
                static Comp _c{};
                return _c;
            }

            static bool is_less_than(const_elem_ref lhs, const_elem_ref rhs) { return node::comparer()(lhs, rhs); }
            static bool is_less_than(const_elem_ptr lhs, const_elem_ptr rhs) { return node::comparer()(*lhs, *rhs); }

            static bool is_equal_to(const_elem_ref a, const_elem_ref b) { return !is_less_than(a, b) && !is_less_than(b, a); }
            static bool is_equal_to(const_elem_ptr a, const_elem_ptr b) { return !is_less_than(a, b) && !is_less_than(b, a); }

        public:
            // const_node_ref walk(visitor const &visitor) const { return LNR(visitor, 9); }

            /**
             * @brief in-order traverse the sub-tree rooted with this node.
             * @param visitor 
             * @return reference to this node
             */
            const_node_ref walk(visitor_l const &visitor) const {
                int abs_index{0};
                return LNR(visitor, abs_index, 0, 0, 0);
            }

            /**
             * @brief pre-order traverse the sub-tree rooted with this node.
             * @param visitor 
             * @return reference to this node
             */
            const_node_ref walk_nlr(visitor_l const &visitor) const {
                int abs_index{0};
                return NLR(visitor, abs_index, 0, 0, 0);
            }

            /**
             * @brief leveled traverse the sub-tree rooted with this node.
             * @param visitor 
             * @return reference to this node.
             */
            const_node_ref walk_level_traverse(visitor_l const &visitor) const { return Level(visitor); }

            /**
             * @brief depth traverse the sub-tree rooted with this node.
             * @param visitor 
             * @return reference to this node.
             */
            const_node_ref walk_depth_traverse(visitor_l const &visitor) const { return Depth(visitor); }

            /**
             * @brief test the existence of a key 'data'
             * @param data 
             * @return whether the key is exist in this tree
             */
            bool exists(elem_type const &data) const {
                int idx = first_of_insertion_position(&data);
                if (idx < _count && is_equal_to(_payloads[idx], &data))
                    return true;
                if (node_ptr p = _pointers[idx]; p)
                    return p->exists(data);
                return false;
            }
            bool exists(elem_type const *data) const { return exists(*data); }

            /** 
             * @brief test the existence of a key 'data' with heavy matching algorithm.
             * @param data 
             * @return 
             */
            bool exists_by_traverse(elem_type const &data) const {
                bool found{};
                walk([data, &found](traversal_context const &ctx) -> bool {
                    if (node::is_equal_to(*ctx.el, data)) {
                        found = true;
                        return false;
                    }
                    if (node::is_less_than(*ctx.el, data)) {
                        // continue to search
                    } else {
                        return false; // not found
                    }
                    return true; // false to terminate the walking
                });
                return found;
            }

            /**
             * @brief find the given key within this tree
             * @param data 
             * @return position object ( a tuple of [ok,node_ref,idx] ) of the search result.
             */
            position find(elem_type const &data) const {
                int index{};
                while (index < _degree - 1 && _payloads[index] && comparer()(*(_payloads[index]), data))
                    index++;
                if (index < _degree - 1 && _payloads[index] && !comparer()(data, *(_payloads[index])))
                    return {true, *this, index};
                if (_pointers[index] == nullptr) // is leaf?
                    return {false, _null_node(), -1};
                return _pointers[index]->find(data);
            }
            position find(elem_type const *data) const { return find(*data); }

            /**
             * @brief find by the given absolute-index within this tree
             * @param data 
             * @return position object ( a tuple of [ok,node_ref,idx] ) of the search result.
             */
            position find_by_index(int index) const {
                node const *res{};
                int saved{index};
                walk([&res, &index, saved](traversal_context const &ctx) -> bool {
                    if (index == ctx.abs_index) {
                        hicc_verbose_debug("find_by_index(%d) -> node(%s) index(%d)",
                                           saved, ctx.curr.to_string().c_str(), ctx.index);
                        res = &ctx.curr;
                        index = ctx.index;
                        return false;
                    }
                    // UNUSED(saved, el, node_this, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                    // UNUSED(saved, el, level);
                    return true; // false to terminate the walking
                });
                if (res)
                    return {true, *res, index};
                return {false, _null_node(), -1};
            }

            /**
             * @brief find this tree with a given match callback function.
             * @param data 
             * @return reference to this node.
             */
            const_node_ref find(std::function<bool(const_elem_ptr, const_node_ptr, int /*level*/, bool /*node_changed*/,
                                                   int /*index*/, int /*abs_index*/)> const &matcher) const {
                walk([matcher](traversal_context const &ctx) -> bool {
                    return matcher(ctx.el, ctx.curr, ctx.level, ctx.node_changed, ctx.index, ctx.abs_index);
                    //return true; // false to terminate the walking
                });
                return (*this);
            }

        protected:
            // in-order traversal
            //
            // never used in context:
            //   loop_base_tmp, level_changed,
            //   parent_ptr_index, parent_ptr_index_changed
            const_node_ref LNR(visitor_l const &visitor, int &abs_index, int level, int parent_ptr_index, int loop_base = 0) const {
                bool parent_ptr_changed{loop_base == 0};
                bool node_changed{true};
                int count{0};
                for (int i = 0, pi = 0; i < _degree - 1; i++, pi++) {
                    auto *n = _pointers[i];
                    if (n) {
                        if (auto &z = n->LNR(visitor, abs_index, level + 1, pi, count); is_null(z))
                            return z;
                        count += n->payload_count() + 1;
                    }

                    auto *t = _payloads[pi];
                    if (t) {
                        if (!visitor(traversal_context{
                                    *this,
                                    t,
                                    level,              // level(),
                                    i,                  // index
                                    loop_base,          // loop_base_tmp,
                                    parent_ptr_index,   // parent_ptr_index
                                    abs_index,          // abs_index or count
                                    false,              // level_changed
                                    node_changed,       // node_changed
                                    parent_ptr_changed, // parent_ptr_index_changed
                            }))
                            return _null_node();
                        abs_index++;
                        node_changed = false;
                        parent_ptr_changed = false;
                    } else
                        break;
                }

                auto *n = _pointers[_degree - 1];
                if (n) {
                    if (auto &z = n->LNR(visitor, abs_index, level + 1, _degree - 1, count); is_null(z))
                        return z;
                    count += n->payload_count() + 1;
                }
                return (*this);
            }

            // pre-order traversal
            //
            // never used in context:
            //   loop_base_tmp, level_changed,
            //   parent_ptr_index, parent_ptr_index_changed
            const_node_ref NLR(visitor_l const &visitor, int &abs_index, int level, int parent_ptr_index, int loop_base = 0) const {
                bool parent_ptr_changed{loop_base == 0};
                if (level == 0) {
                    traversal_context ctx{
                            *this,
                            &get_el(0),
                            level,                        // level(),
                            parent_ptr_index + loop_base, // index
                            loop_base,                    // loop_base_tmp,
                            parent_ptr_index,             // parent_ptr_index
                            abs_index,                    // abs_index or count
                            false,                        // level_changed
                            false,                        // node_changed
                            parent_ptr_changed,           // parent_ptr_index_changed
                    };
                    if (!_visit_payloads(*this, ctx, visitor))
                        return _null_node();
                    parent_ptr_changed = false;
                    abs_index = ctx.abs_index;
                }

                for (int pi = 0; pi < _degree; pi++) {
                    auto *n = _pointers[pi];
                    if (n) {
                        assert(n->_parent == this);
                        traversal_context ctx{
                                *n,
                                &n->get_el(0),
                                level + 1,          // level(),
                                pi + loop_base,     // index
                                loop_base,          // loop_base_tmp,
                                parent_ptr_index,   // parent_ptr_index
                                abs_index,          // abs_index or count
                                false,              // level_changed
                                false,              // node_changed
                                parent_ptr_changed, // parent_ptr_index_changed
                        };
                        if (!_visit_payloads(*n, ctx, visitor))
                            return _null_node();
                        parent_ptr_changed = false;
                        abs_index = ctx.abs_index;
                    } else
                        break;
                }

                int count{0};
                for (int pi = 0; pi < _degree; pi++) {
                    auto *n = _pointers[pi];
                    if (n) {
                        if (auto &z = n->NLR(visitor, abs_index, level + 1, pi, count); is_null(z)) {
                            count += n->payload_count() + 1;
                            return z;
                        }
                        count += n->payload_count() + 1;
                    }
                }
                return (*this);
            }

            // level traversal
            const_node_ref Level(visitor_l const &visitor) const {
                std::list<traversal_context> queue;
                // std::list<std::tuple<node_ptr, int /*index*/, int /*loop_base*/, int /*parent_ptr_index*/>> queue;
                auto abs_index{0}, level{0}, last_level{-1}, last_parent_ptr_index{-1};
                queue.push_back({*this, &get_el(0), level, 0, 0, 0, 0, true, true, true});

                while (!queue.empty()) {
                    // auto [curr, index, loop_base, parent_ptr_index] = queue.front();
                    auto pos = queue.front();
                    queue.pop_front();

                    if (pos.curr) {
                        pos.abs_index = abs_index;
                        abs_index += pos.curr.payload_count();
                        pos.level_changed = last_level != pos.level;
                        last_level = pos.level;
                        if (pos.level_changed) last_parent_ptr_index = -1;
                        pos.parent_ptr_index_changed = last_parent_ptr_index != pos.parent_ptr_index;
                        last_parent_ptr_index = pos.parent_ptr_index;

                        if (!_visit_payloads(pos.curr, pos, visitor))
                            return (*this);

                        auto loop_base_tmp = pos.loop_base;
                        for (auto i = 0; i < _degree; i++) {
                            if (auto *p = pos.curr._pointers[i]; p) {
                                queue.push_back({
                                        *p,
                                        &p->get_el(i),
                                        pos.level + 1,
                                        i,
                                        loop_base_tmp,
                                        pos.index,
                                        0,     // count
                                        false, // level_changed
                                        false, // node_changed
                                        false, // parent_ptr_index_changed
                                });
                                loop_base_tmp += p->payload_count();
                            } else
                                break;
                        }
                    }
                }
                return (*this);
            }

            // depth order traverse
            const_node_ref Depth(visitor_l const &visitor) const {
                std::list<traversal_context> queue;
                queue.push_back({this, 0, 0, 0});
                auto level{0};

                while (!queue.empty()) {
                    auto ctx = queue.back();
                    queue.pop_back();

                    if (ctx.curr) {
                        if (!_visit_payloads(ctx, visitor))
                            return (*this);

                        auto loop_base_tmp = ctx.loop_base;
                        for (auto i = 0; i < _degree; i++) {
                            if (auto p = ctx.curr->_pointers[i]; p) {
                                queue.push_back({
                                        *p,
                                        p->get_el(i),
                                        level,                                    // level(),
                                        i,                                        // index
                                        loop_base_tmp,                            // loop_base_tmp,
                                        ctx.parent_ptr_index + i + loop_base_tmp, // parent_ptr_index
                                        0,                                        // abs_index or count
                                        false,                                    // level_changed
                                        false,                                    // node_changed
                                        false,                                    // parent_ptr_index_changed
                                });
                                loop_base_tmp += p->payload_count();
                            }
                        }
                    }

                    if (ctx.index == 0) level++;
                }
                return (*this);
            }

            static bool _visit_payloads(const_node_ref ref, traversal_context &ctx, visitor_l const &visitor) {
                bool node_changed{true};
                for (int pi = 0; pi < ref._degree - 1; pi++) {
                    auto *t = ref._payloads[pi];
                    if (t) {
                        ctx.node_changed = node_changed;
                        ctx.el = t;
                        if (!visitor(ctx))
                            return false;
                        node_changed = false;
                        ctx.abs_index++;
                    } else
                        break;
                }
                return true;
            }

        public:
            void dbg_dump(size_type known_size, std::ostream &os = std::cout, const char *headline = nullptr) const {
                if (headline) os << headline;

                int count = total_from_calculating();

                Level([known_size, count, &os](traversal_context const &ctx) -> bool {
                    if (ctx.node_changed) {
                        if (ctx.index == 0 && ctx.level_changed)
                            os << '\n'
                               << ' ' << ' ';
                        else if (ctx.parent_ptr_index_changed)
                            os << ' ' << '|' << ' ';
                        else
                            os << ' ';
                        os << '[' << ctx.curr.to_string() << ']';
                        if (ctx.level == 0) {
                            os << '/' << known_size;
                            os << "/dyn:" << count;
                            // assertm(ctx.curr.parent() == nullptr, "root's parent must be nullptr");
                        }
                        // os << '{' << std::boolalpha << ctx.parent_ptr_index << ',' << (ctx.parent_ptr_index_changed?'T':'_') << ',' << ctx.loop_base << '}';
                        // os << '{' << ctx.level << ',' << std::boolalpha
                        //    //  << ctx.level_changed << ',' << ctx.node_changed << ',' << ctx.parent_ptr_changed << ','
                        //    << ctx.parent_ptr_index << ',' << ctx.index << ',' << ctx.abs_index << '}';
                    }
                    os.flush();
                    return true; // false to terminate the walking
                });
                os << '\n';
                // assert(known_size == (size_type) count);
            }

            void assert_it(btree &bt, int level = 0) {
                const int max_payloads = _degree - 1;
                const int min_payloads = max_payloads / 2;
                const int _M = _degree / 2;
                assert(_M == min_payloads + 1);
                UNUSED(_M, min_payloads, max_payloads);

                for (int t = 0; t < _count; t++)
                    if (auto *p = _payloads[t]; !p)
                        assertm(_payloads[t] != nullptr, "the payloads lower than _count must be valid pointers to the elem_type.");
                for (int t = _count; t < max_payloads; t++)
                    if (auto *p = _payloads[t]; p)
                        assertm(_payloads[t] == nullptr, "the payloads larger than _count must be nullptr.");
                for (int t = _count + 1; t < _degree; t++)
                    if (auto *p = _pointers[t]; p)
                        assertm(_pointers[t] == nullptr, "the payloads larger than _count must be nullptr.");

                if (this == bt._root) {
                    std::vector<elem_ptr> els;
                    std::cout << "loop for btree: ";
                    walk([&els](traversal_context const &ctx) -> bool {
                        std::cout << elem_to_string(ctx.el) << ',';
                        if (els.size()) {
                            elem_ptr last = els.back();
                            assert(is_less_than(last, ctx.el));
                        }
                        for (auto *ptr : els) {
                            assert(ptr != ctx.el); // wrong case: elem_ptr duplicated
                        }
                        els.push_back(const_cast<elem_ptr>(ctx.el));
                        return true;
                    });
                    std::cout << '\n';
                } else {
                    assertm(_count >= min_payloads, "_count should be larger than min_payloads.");
                    assertm(_count <= max_payloads, "_count should be lower than max_payloads.");
                }

                for (int t = 0; t <= max_payloads; t++)
                    if (auto *p = _pointers[t]; p)
                        p->assert_it(bt, level + 1);
            }

        public:
            int total_from_calculating() const {
                int count = 0;
                int abs_index = 0;
                int level = 0;
                int parent_ptr_index = 0;
                LNR([&count](traversal_context const &ctx) -> bool {
                    count++;
                    UNUSED(ctx);
                    return true;
                },
                    abs_index, level, parent_ptr_index);
                return count;
            }

            int payload_count() const { return _count; }
            void inc_payload_count() { _count++; }
            void dec_payload_count() { _count--; }

            std::string to_string() const {
                std::ostringstream os;
                for (int i = 0; i < _count; i++) {
                    if (elem_ptr p = _payloads[i]; p) {
                        if (i > 0) os << ',';
                        os << elem_to_string(p);
                    } else
                        break;
                }
                return os.str();
            }
            std::string c_str() const { return to_string(); }
            static std::string elem_to_string(const_elem_ptr el) {
                std::ostringstream os;
                os << (*el);
                return os.str();
            }
            static std::string elem_to_string(const_elem_ref el) {
                std::ostringstream os;
                os << (el);
                return os.str();
            }
        };

    public:
        template<class A, typename... Args,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A, Args...>::value &&
                                 !std::is_same<std::decay_t<A>, elem_type>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a0, Args &&...args) {
            elem_type v(std::forward<A>(a0), std::forward<Args>(args)...);
            insert(v);
        }

        template<class A,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A>::value &&
                                 !std::is_same<std::decay_t<A>, elem_type>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a) {
            elem_type v(std::forward<A>(a));
            insert(v);
        }

    public:
        /**
         * @brief the main function to insert a new key in this tree
         * @param a 
         */
        void insert(elem_type &&a) { _insert(new elem_type(a)); }
        void insert(const_elem_ref el) { _insert(new elem_type(el)); }
        // void insert(elem_type el) { _insert(new elem_type(el)); }

        template<typename... Args>
        void insert(Args &&...args) { (_insert(new elem_type(args)), ...); }
        template<typename... Args>
        void insert(Args const &...args) { (_insert(new elem_type(args)), ...); }
        void insert(elem_ptr a_new_data_ptr) { _insert(a_new_data_ptr); }

    private:
        void _insert(elem_ptr el) {
            auto el_str = node::elem_to_string(el);
            hicc_debug("insert '%s' ...", el_str.c_str());
            if (_root == nullptr) {
                _root = new node(_degree);
                _root->set_el(0, el);
                _root->_count = 1;
                DEBUG_ONLY(_dbg_after_inserted(el_str));
                return;
            }

            const int max_payloads = _degree - 1;
            if (_root->_count == max_payloads) {
                node_ptr np = new node(_degree);
                np->_pointers[0] = _root;
                np->_split_child(0, _root);
                int i = 0;
                if (node::is_less_than(np->_payloads[0], el)) i++;
                np->_pointers[i]->insert_non_full(el);
                _root = np;
                DEBUG_ONLY(_dbg_after_inserted(el_str));
                return;
            }

            _root->insert_non_full(el);
            DEBUG_ONLY(_dbg_after_inserted(el_str));
        }
#if defined(_DEBUG)
        void _dbg_after_inserted(std::string const &el_str) {
            std::ostringstream os;
            os << "after '" << el_str << "' inserted .";
            DEBUG_ONLY(dbg_dump(std::cout, os.str().c_str()));
            DEBUG_ONLY(assert_it());
        }
#endif

    public:
        /**
         * @brief the main function to remove a given key within this tree
         * @param a 
         */
        void remove(elem_type &&a) { _remove(&a); }
        void remove(const_elem_ref el) { _remove(el); }
        void remove(elem_ptr el) { _remove(el); }

        template<typename... Args>
        void remove(Args &&...args) { (_remove(&args), ...); }
        template<typename... Args>
        void remove(Args const &...args) { (_remove(&args), ...); }

    private:
        void _remove(const_elem_ref el) {
            auto el_str = node::elem_to_string(el);
            hicc_debug("remove '%s' ...", el_str.c_str());
            if (_root == nullptr)
                return;
            elem_ptr removed = _root->remove(el);
            _check_root_is_empty(el_str);
            if (removed)
                delete removed;
        }
        void _remove(elem_ptr el) {
            auto el_str = node::elem_to_string(el);
            hicc_debug("remove '%s' ...", el_str.c_str());
            if (_root == nullptr)
                return;
            elem_ptr removed = _root->remove(el);
            _check_root_is_empty(el_str);
            if (removed)
                delete removed;
        }
        void _check_root_is_empty(const std::string &el_str) {
            if (_root->_count == 0) {
                node_ptr tmp = _root;
                if (_root->is_leaf())
                    _root = nullptr;
                else
                    _root = _root->_pointers[0];
                delete tmp->reset_for_delete();
            }

#if defined(_DEBUG)
            std::ostringstream os;
            os << "after '" << el_str << "' removed .";
            DEBUG_ONLY(dbg_dump(std::cout, os.str().c_str()));
            DEBUG_ONLY(assert_it());
#else
            UNUSED(el_str);
#endif
        }

    public:
        void clear() {
            if (_root)
                delete _root;
        }

        void dbg_dump(std::ostream &os = std::cout, const char *headline = nullptr) const {
            if (_root)
                _root->dbg_dump(_size, os, headline);
        }

        void assert_it() {
            if (_root)
                _root->assert_it(*this, 0);
        }

        int total_count() const {
            if (_root)
                return _root->total_from_calculating();
            return 0;
        }
        size_type size() const { return total_count(); }

        bool exists(const_elem_ref data) const {
            if (_root)
                return _root->exists(data);
            return false;
        }
        bool exists(const_elem_ptr data_ptr) const { return exists(*data_ptr); }

        position find(elem_type const &data) const {
            if (_root)
                return _root->find(data);
            return {false, node::_null_node(), -1};
        }
        position find_by_index(int index) const {
            if (_root)
                return _root->find_by_index(index);
            return {false, node::_null_node(), -1};
        }
        const_node_ref find(std::function<bool(const_elem_ptr, const_node_ptr, int /*level*/, bool /*node_changed*/,
                                               int /*index*/, int /*abs_index*/)> const &matcher) const {
            if (_root)
                return _root->find(matcher);
            return node::_null_node();
        }

        /**
         * @brief in-order traverse the sub-tree rooted with this node.
         * @param visitor 
         * @return reference to this node
         */
        const_node_ref walk(visitor_l const &visitor) const {
            if (_root)
                return _root->walk(visitor);
            return node::_null_node();
        }

        /**
         * @brief pre-order traverse the sub-tree rooted with this node.
         * @param visitor 
         * @return reference to this node
         */
        const_node_ref walk_nlr(visitor_l const &visitor) const {
            if (_root)
                return _root->walk_nlr(visitor);
            return node::_null_node();
        }

        /**
         * @brief leveled traverse the sub-tree rooted with this node.
         * @param visitor 
         * @return reference to this node.
         */
        const_node_ref walk_level_traverse(visitor_l const &visitor) const {
            if (_root)
                return _root->walk_level_traverse(visitor);
            return node::_null_node();
        }

        /**
         * @brief depth traverse the sub-tree rooted with this node.
         * @param visitor 
         * @return reference to this node.
         */
        const_node_ref walk_depth_traverse(visitor_l const &visitor) const {
            if (_root)
                return _root->walk_depth_traverse(visitor);
            return node::_null_node();
        }

        std::vector<elem_type> to_vector() const {
            std::vector<elem_type> vec;
            vec.reserve(size());
            typename decltype(vec)::iterator it{vec.begin()};
            walk([&vec, &it](traversal_context const &ctx) -> bool {
                vec.insert(it++, *ctx.el);
                return true;
            });
            return vec;
        }

        std::string to_string() const {
            if (_root) return _root->to_string();
            return std::string{};
        }

        friend std::ostream &operator<<(std::ostream &os, btree &o) {
            o.dbg_dump(os);
            return os;
        }

    private:
        int _degree;
        int _size;
        node_ptr _root;

    }; // btree<T>


} // namespace hicc::btree



#endif //HICC_CXX_HZ_BTREE_HH
