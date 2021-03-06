//
// Created by Hedzr Yeh on 2021/3/4.
//

#ifndef HICC_CXX_HZ_B_TREE_HH
#define HICC_CXX_HZ_B_TREE_HH

#include <list>
#include <vector>

#include <algorithm>
#include <functional>
#include <memory>

#include <type_traits>
#include <typeinfo>

#include "hz-dbg.hh"
#include "hz-defs.hh"
#include "hz-log.hh"


namespace hicc::btree {

    /**
     * @brief provides B/B+-tree data structure in generic style.
     * 
     * @details The order, or branching factor, `b` of a B+ tree measures the capacity of 
     * nodes (i.e., the number of children nodes) for internal nodes in the 
     * tree. The actual number of children for a node, referred to here as `m`, 
     * is constrained for internal nodes so that `b/2 <= m <= b`.
     * 
     * @tparam T your data type
     * @tparam Container vector or list here
     */
    template<class T, int Order = 5, class Comp = std::less<T>>
    class btree final {
    public:
        btree();
        ~btree();

    public:
        struct node;

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

        using visitor = std::function<bool(elem_ref, node_ref, int level, bool node_changed,
                                           int ptr_parent_index, bool parent_ptr_changed, int ptr_index)>;

        struct node {
            // Container<T> _payload{};
            // Container<node_ptr> _pointers{};
            // std::array<node_ptr, Order> _payloads{};
            // std::array<node_ptr, Order+1> _pointers{};
            T *_payloads[Order];
            node_ptr _pointers[Order];
            node_ptr _parent{};

            using self_type = node;
            const int P_LEN = Order - 1;
            const int MID = P_LEN / 2;

            node() {
                for (int i = 0; i < Order; i++) {
                    _pointers[i] = nullptr;
                    _payloads[i] = nullptr;
                }
            }
            explicit node(elem_type *first_data, elem_type *second_data = nullptr)
                : node() {
                _payloads[0] = first_data;
                _payloads[1] = second_data;
            }
            explicit node(node_ptr src, int start, int end)
                : node() {
                for (auto t = start, i = 0; t < end; t++, i++) {
                    _payloads[i] = src->_payloads[t];
                    _pointers[i] = src->_pointers[t];
                }
                _pointers[end - start] = src->_pointers[end];
            }
            node_ptr parent(node_ptr p, bool deep = false) {
                _parent = p;
                if (deep) {
                    for (auto t = 0; t < Order; t++) {
                        if (auto *child = _pointers[t]; child) {
                            child->_parent = this;
                        } else
                            break;
                    }
                }
                return this;
            }
            ~node() { clear(); }

        public:
            template<class A, typename... Args,
                     std::enable_if_t<
                             std::is_constructible<elem_type, A, Args...>::value &&
                                     !std::is_same<std::decay_t<A>, elem_type>::value &&
                                     !std::is_same<std::decay_t<A>, node>::value,
                             int> = 0>
            void emplace(node_ptr &root, A &&a0, Args &&...args) {
                elem_type v(std::forward<A>(a0), std::forward<Args>(args)...);
                insert_one(root, v);
            }
            template<class A,
                     std::enable_if_t<
                             std::is_constructible<elem_type, A>::value &&
                                     !std::is_same<std::decay_t<A>, elem_type>::value &&
                                     !std::is_same<std::decay_t<A>, node>::value,
                             int> = 0>
            void emplace(node_ptr &root, A &&a) {
                elem_type v(std::forward<A>(a));
                insert_one(root, v);
            }

            void insert_one(node_ptr &root, elem_type &&a) { _insert_one_entry(root, &a); }
            // void insert_one(node_ptr &root, elem_type const &a) { _insert_one(root, &a, false); }
            void insert_one(node_ptr &root, elem_type a) { _insert_one_entry(root, new elem_type(std::move(a))); }
            void insert_one(node_ptr &root, elem_type *a) { _insert_one_entry(root, a); }

            void reset() {
                _parent = nullptr;
                for (int i = 0; i < Order; i++) {
                    _pointers[i] = nullptr;
                    _payloads[i] = nullptr;
                }
            }
            void clear() {
                for (int i = 0; i < Order; i++) {
                    if (_pointers[i]) {
                        delete _pointers[i];
                        _pointers[i] = nullptr;
                    }
                    if (_payloads[i]) {
                        delete _payloads[i];
                        _payloads[i] = nullptr;
                    }
                }
            }

        private:
            void _insert_one_entry(node_ptr &root, elem_type *a, node_ptr left_child = nullptr, node_ptr right_child = nullptr) {
                hicc_debug("insert '%s' to %s", elem_to_string(a).c_str(), to_string().c_str());
                _insert_one(root, a, left_child, right_child);
            }
            void _insert_one(node_ptr &root, elem_type *a, node_ptr left_child = nullptr, node_ptr right_child = nullptr) {
                for (int i = 0; i < P_LEN; ++i) {
                    auto *vp = _payloads[i];
                    if (vp == nullptr) {
                        if (left_child || right_child) {
                            // recursively:
                            hicc_verbose_debug("  -> splitting...move '%s' up to parent node.", elem_to_string(a).c_str());
                            _payloads[i] = a;
                            if (left_child) _pointers[i] = left_child->parent(this);
                            if (right_child) _pointers[i + 1] = right_child->parent(this);
                            return;
                        }

                        // normally insertion
                        if (_pointers[i]) {
                            _pointers[i]->_insert_one(root, a);
                            return;
                        }

                        _payloads[i] = a;
                        return;
                    }

                    if (comparer()(*a, *vp)) {
                        // less than this one in payloads[], insert it

                        if (!left_child && !right_child) {
                            // has_child? forward a into it
                            if (_pointers[i]) {
                                _pointers[i]->_insert_one(root, a);
                                return;
                            }
                        }

                        // recursively, or no child, insert at position `i`
                        auto [has_slot, j] = find_free_slot(i);
                        if (has_slot) {
                            for (int pos = j; pos > i; --pos) _payloads[pos] = _payloads[pos - 1];
                            _payloads[i] = a;
                            if (left_child || right_child) {
                                hicc_verbose_debug("  -> splitting...move '%s' up to parent node.", elem_to_string(a).c_str());
                                for (auto t = P_LEN; t >= i; t--) _pointers[t] = _pointers[t - 1];
                                if (left_child) _pointers[i] = left_child->parent(this);
                                if (right_child) _pointers[i + 1] = right_child->parent(this);
                            }
                            return;
                        }

                        // split this node
                        for (auto t = Order - 1; t > i; t--) _payloads[t] = _payloads[t - 1];
                        _payloads[i] = a;
                        split_and_raise(root, i, left_child, right_child);
                        return;
                    }
                }

                if (!left_child && !right_child) {
                    // has_child? forward `a` into it
                    if (_pointers[P_LEN]) {
                        _pointers[P_LEN]->_insert_one(root, a);
                        return;
                    }
                }

                // recursively: `a` is greater than every one in this node payloads, create a new parent
                _payloads[P_LEN] = a;
                split_and_raise(root, P_LEN, left_child, right_child);
            }

        public:
            // const_node_ref walk(visitor const &visitor) const { return LNR(visitor, 9); }
            node_ref walk(visitor const &visitor) { return LNR(visitor, 0, 0); }
            node_ref walk_nlr(visitor const &visitor) { return NLR(visitor, 0, 0); }

        private:
            void split_and_raise(node_ptr &root, int a_pos, node_ptr a_left, node_ptr a_right) {
                auto *new_parent_data = _payloads[MID];
                hicc_verbose_debug("  split and raise '%s' up...", elem_to_string(_payloads[MID]).c_str());
                for (auto t = MID; t <= P_LEN; t++) _payloads[t] = _payloads[t + 1];
                if (_parent) {
                    node_ptr left = this;
                    node_ptr right = new node(this, MID, P_LEN);
                    for (auto t = MID; t < Order; t++) _pointers[t] = nullptr;
                    for (auto t = MID; t < Order; t++) _payloads[t] = nullptr;
                    _parent->_insert_one(root, new_parent_data, left, right);
                    if (a_pos < MID) {
                        if (a_left) left->_pointers[a_pos] = a_left->parent(left);
                        if (a_right) left->_pointers[a_pos + 1] = a_right->parent(left);
                    } else if (a_pos > MID) {
                        if (a_left) right->_pointers[0] = a_left->parent(right);
                        if (a_right) right->_pointers[1] = a_right->parent(right);
                    }
                } else {
                    auto *new_parent_node = new node(new_parent_data);
                    node_ptr left = this->parent(new_parent_node);
                    node_ptr right = (new node(this, MID, P_LEN))->parent(new_parent_node, true);
                    for (auto t = Order - 1; t > 0; --t) _pointers[t] = _pointers[t - 1];
                    for (auto t = MID + 1; t < Order; t++) _pointers[t] = nullptr;
                    for (auto t = MID; t < Order; t++) _payloads[t] = nullptr;

                    root = new_parent_node;
                    root->_pointers[0] = left;
                    root->_pointers[1] = right;
                }
            }

            node_ref LNR(visitor const &visitor, int level, int ptr_index) {
                bool node_changed{true};
                for (int i = 0, pi = 0; i < P_LEN; i++, pi++) {
                    auto *n = _pointers[i];
                    if (n) n->LNR(visitor, level + 1, ptr_index);

                    auto *t = _payloads[pi];
                    if (t) {
                        if (visitor)
                            if (!visitor(*t, *this, level, node_changed, ptr_index, false, i))
                                return (*this);
                    }
                    node_changed = false;
                }

                auto *n = _pointers[Order - 1];
                if (n) n->LNR(visitor, level + 1, ptr_index);
                return (*this);
            }

            node_ref NLR(visitor const &visitor, int level, int parent_ptr_index, int loop_base = 0) {
                bool parent_ptr_changed{true};
                if (level == 0) {
                    if (!visit_payloads(this, level, parent_ptr_index, parent_ptr_changed, parent_ptr_index + loop_base, visitor))
                        return (*this);
                    parent_ptr_changed = false;
                }

                for (int pi = 0; pi < Order; pi++) {
                    auto *n = _pointers[pi];
                    if (n) {
                        assert(n->_parent == this);
                        if (!visit_payloads(n, level + 1, parent_ptr_index, parent_ptr_changed, pi + loop_base, visitor))
                            return (*this);
                        parent_ptr_changed = false;
                    } else
                        break;
                }

                int count{0};
                for (int pi = 0; pi < Order; pi++) {
                    auto *n = _pointers[pi];
                    if (n) n->NLR(visitor, level + 1, pi, count), count += n->payload_count() + 1;
                }
                return (*this);
            }

            static bool visit_payloads(node_ptr ptr, int level, int parent_ptr_index, bool parent_ptr_changed, int ptr_index, visitor const &visitor) {
                bool node_changed{true};
                for (int pi = 0; pi < Order - 1; pi++) {
                    auto *t = ptr->_payloads[pi];
                    if (t) {
                        if (visitor)
                            if (!visitor(*t, *ptr, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index))
                                return false;
                        node_changed = false;
                    } else
                        break;
                }
                return true;
            }

            std::tuple<bool, int> find_free_slot(int start_index) const {
                bool has_slot = false;
                int j = start_index + 1;
                for (; j < P_LEN; ++j) {
                    auto *vpt = _payloads[j];
                    if (vpt == nullptr) {
                        has_slot = true;
                        break;
                    }
                }
                return {has_slot, j};
            }

            static Comp &comparer() {
                static Comp _c{};
                return _c;
            }

            std::string elem_to_string(elem_type *a) const {
                std::ostringstream os;
                os << (*a);
                return os.str();
            }

        public:
            std::string to_string() const {
                std::ostringstream os;
                os << '[';
                for (int i = 0; i < Order - 1; ++i) {
                    if (_payloads[i]) {
                        if (i > 0) os << ',';
                        os << (*_payloads[i]);
                    }
                }
                os << ']';
                return os.str();
            }
            std::string c_str() const { return to_string(); }
            int payload_count() const {
                int i{0};
                for (; i < Order - 1; ++i) {
                    if (!_payloads[i]) {
                        break;
                    }
                }
                return i;
            }
        };

    public:
        template<typename... Args>
        void insert(Args &&...args) {
            (_root->insert_one(_root, args), ...);
#ifdef _DEBUG
            assert_it();
#endif
        }

        void insert(elem_type *a_new_data_ptr) {
            assert(a_new_data_ptr);
            _root->insert_one(_root, a_new_data_ptr);
#ifdef _DEBUG
            assert_it();
#endif
        }

        template<class A, typename... Args,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A, Args...>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a0, Args &&...args) {
            _root->emplace(_root, std::forward<A>(a0), std::forward<Args>(args)...);
        }
        template<class A,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a) {
            _root->emplace(_root, std::forward<A>(a));
        }

        void reset() { _root->reset(); }
        void clear() {
            // if (_root) {
            _root->reset();
            delete _root;
            _root = nullptr;
            // }
        }

#ifdef _DEBUG
        void assert_it() const {
            walk([](typename hicc::btree::btree<T, Order>::const_elem_ref el,
                    typename hicc::btree::btree<T, Order>::const_node_ref node,
                    int level, bool node_changed,
                    int parent_ptr_index, bool parent_ptr_changed,
                    int ptr_index) -> bool {
                for (int i = 0; i < Order; i++) {
                    auto const *vp = node._pointers[i];
                    if (vp) {
                        std::ostringstream os, os1;
                        if (vp->_parent) os1 << '(' << vp->_parent->to_string() << ')';
                        os << "node " << vp->to_string() << " has parent " << vp->_parent << os1.str() << ". but not equal to " << &node << " (" << node.to_string() << ").\n";
                        assert(print_if_false(vp->_parent == &node, os.str()));
                    }
                }
                UNUSED(el, node, node_changed, level, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true;
            });
        }
#endif

        const_node_ptr find(std::function<bool(const_elem_ptr, const_node_ptr)> const &matcher) const {
            UNUSED(matcher);
            return nullptr;
        }

        const_node_ref walk(visitor const &visitor) const { return _root->walk(visitor); }
        node_ref walk(visitor const &visitor) { return _root->walk(visitor); }
        const_node_ref walk_nlr(visitor const &visitor) const { return _root->walk_nlr(visitor); }
        node_ref walk_nlr(visitor const &visitor) { return _root->walk_nlr(visitor); }

        bool is_null(T const &data) const { return data == _null_elem(); }
        bool is_null(node const &data) const { return data == _null_node(); }

    protected:
        static T &_null_elem() {
            static T _d;
            return _d;
        }
        static node_ref _null_node() {
            static node _d;
            return _d;
        }

    private:
        node_ptr _root{};
        node_ptr _first_leaf{};
        node_ptr _last_leaf{};
    };

    template<class T, int Order, class Comp>
    inline btree<T, Order, Comp>::btree()
        : _root(new node{}) {
    }

    template<class T, int Order, class Comp>
    inline btree<T, Order, Comp>::~btree() {
    }


} // namespace hicc::btree


#endif //HICC_CXX_HZ_B_TREE_HH
