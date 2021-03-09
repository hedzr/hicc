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
    template<class T, int Degree = 5, class Comp = std::less<T>>
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

        using size_type = std::size_t;

        using position = std::tuple<bool /*ok*/, const_node_ref, int>;

        using elem_type = T;
        using elem_ptr = elem_type *;
        using const_elem_ptr = elem_type const *;
        using elem_ref = elem_type &;
        using const_elem_ref = elem_type const &;

        using visitor = std::function<bool(elem_ref, node_ref, int level, bool node_changed,
                                           int ptr_parent_index, bool parent_ptr_changed, int ptr_index)>;

        static const int P_LEN = Degree - 1;
        static const int MID = P_LEN / 2;

        struct node {
            // Container<T> _payload{};
            // Container<node_ptr> _pointers{};
            // std::array<node_ptr, Degree> _payloads{};
            // std::array<node_ptr, Degree+1> _pointers{};
            T *_payloads[Degree + 1];
            node_ptr _pointers[Degree + 1];
            node_ptr _parent{};
            size_type _size{};

            using self_type = node;

            node() {
                for (int i = 0; i < Degree + 1; i++) {
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
                for (auto t = start, i = 0; t < end; t++, i++) _payloads[i] = src->_payloads[t];
                for (auto t = start, i = 0; t <= end; t++, i++) _pointers[i] = src->_pointers[t];
                _pointers[end - start] = src->_pointers[end];
            }
            node_ptr parent(node_ptr p, bool deep = false) {
                _parent = p;
                if (deep) {
                    for (auto t = 0; t < Degree; t++) {
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

            void remove(node_ptr &root, elem_type &&a) { _remove_one_entry(root, &a); }
            void remove(node_ptr &root, elem_type a) { _remove_one_entry(root, &a); }
            void remove(node_ptr &root, elem_type const *a) { _remove_one_entry(root, a); }
            void remove_by_position(node_ptr &root, node const &res, int index) { _remove_one_entry(root, res, index); }

            // const_node_ref walk(visitor const &visitor) const { return LNR(visitor, 9); }
            node_ref walk(visitor const &visitor) { return LNR(visitor, 0, 0); }
            node_ref walk_nlr(visitor const &visitor) { return NLR(visitor, 0, 0); }
            node_ref walk_level_traverse(visitor const &visitor) { return Level(visitor); }

            void reset() {
                _parent = nullptr;
                for (int i = 0; i < Degree; i++) {
                    _pointers[i] = nullptr;
                    _payloads[i] = nullptr;
                }
                _size = 0;
            }
            void clear() {
                for (int i = 0; i < Degree; i++) {
                    if (_pointers[i]) {
                        delete _pointers[i];
                        _pointers[i] = nullptr;
                    }
                    if (_payloads[i]) {
                        delete _payloads[i];
                        _payloads[i] = nullptr;
                    }
                }
                _size = 0;
            }

        private:
            void _insert_one_entry(node_ptr &root, elem_type *a, node_ptr left_child = nullptr, node_ptr right_child = nullptr) {
                hicc_debug("insert '%s' to %s", elem_to_string(a).c_str(), to_string().c_str());
                auto *saved = root;
                _insert_one(root, a, left_child, right_child);
                ++_size;
                if (saved != root) ++_size;
            }

            void _insert_one(node_ptr &root, elem_type *a, node_ptr left_child = nullptr, node_ptr right_child = nullptr) {
                for (int i = 0; i < P_LEN; ++i) {
                    auto *vp = _payloads[i];
                    if (vp == nullptr) {
                        if (left_child || right_child) {
                            // recursively:
                            hicc_verbose_debug("  -> put...move '%s' up to parent node.", elem_to_string(a).c_str());
                            _payloads[i] = a;
                            if (left_child) _pointers[i] = left_child->parent(this, true);
                            if (right_child) _pointers[i + 1] = right_child->parent(this, true);
                            return;
                        }

                        // normal insertion
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
                        auto [has_slot, j] = _find_free_slot(i);
                        if (has_slot) {
                            for (int pos = j; pos > i; --pos) _payloads[pos] = _payloads[pos - 1];
                            _payloads[i] = a;
                            if (left_child || right_child) {
                                hicc_verbose_debug("  -> splitting...move '%s' up to parent node.", elem_to_string(a).c_str());
                                for (auto t = Degree; t >= i; t--) _pointers[t] = _pointers[t - 1];
                                if (left_child) _pointers[i] = left_child->parent(this, true);
                                if (right_child) _pointers[i + 1] = right_child->parent(this, true);
                            }
                            return;
                        }

                        // split this node
                        for (auto t = Degree - 1; t > i; t--) _payloads[t] = _payloads[t - 1];
                        for (auto t = Degree; t > i; t--) _pointers[t] = _pointers[t - 1];
                        _payloads[i] = a;
                        if (left_child) _pointers[i] = left_child->parent(this, true);
                        if (right_child) _pointers[i + 1] = right_child->parent(this, true);
                        _split_and_raise(root, i, left_child, right_child);
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
                if (left_child) _pointers[P_LEN] = left_child->parent(this);
                if (right_child) _pointers[P_LEN + 1] = right_child->parent(this);
                _split_and_raise(root, P_LEN, left_child, right_child);
            }

            void _split_and_raise(node_ptr &root, int a_pos, node_ptr a_left, node_ptr a_right) {
                auto *new_parent_data = _payloads[MID];
                hicc_verbose_debug("  split and raise '%s' up...", elem_to_string(new_parent_data).c_str());
                if (_parent) {
                    node_ptr left = this;
                    node_ptr right = new node(this, MID + 1, Degree);
                    for (auto t = MID + 1; t <= Degree; t++) _pointers[t] = nullptr;
                    for (auto t = MID; t < Degree; t++) _payloads[t] = nullptr;
                    _parent->_insert_one(root, new_parent_data, left, right);
                    if (a_pos < MID) {
                        if (a_left) left->_pointers[a_pos] = a_left->parent(left, true);
                        if (a_right) left->_pointers[a_pos + 1] = a_right->parent(left, true);
                    } else if (a_pos > MID) {
                        auto pos = a_pos - MID - 1;
                        if (a_left) right->_pointers[pos] = a_left->parent(right, true);
                        if (a_right) right->_pointers[pos + 1] = a_right->parent(right, true);
                    }
                } else {
                    auto *new_parent_node = new node(new_parent_data);
                    node_ptr left = this->parent(new_parent_node);
                    node_ptr right = (new node(this, MID + 1, Degree))->parent(new_parent_node, true);
                    for (auto t = MID + 1; t <= Degree; t++) _pointers[t] = nullptr;
                    for (auto t = MID; t < Degree; t++) _payloads[t] = nullptr;

                    root = new_parent_node;
                    root->_pointers[0] = left;
                    root->_pointers[1] = right;
                }
            }

            bool _remove_one_entry(node_ptr &root, elem_type const *a) {
                hicc_debug("remove '%s' from %s", elem_to_string(a).c_str(), to_string().c_str());
                node *res{};
                int index{-1};
                LNR([a, &res, &index](
                            typename hicc::btree::btree<T, Degree>::const_elem_ref el,
                            typename hicc::btree::btree<T, Degree>::const_node_ref node,
                            int level, bool node_changed,
                            int parent_ptr_index, bool parent_ptr_changed,
                            int ptr_index) -> bool {
                    if (el == *a) {
                        res = const_cast<node_ptr>(&node);
                        index = ptr_index;
                        return false;
                    }
                    UNUSED(el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                    return true; // false to terminate the walking
                },
                    0, 0);

                auto *saved = root;
                if (auto raised = _remove_one(root, res, index); raised) {
                    delete raised;
                    --_size;
                    if (saved != root) --_size;
                    return true;
                }
                return false;
            }

            bool _remove_one_entry(node_ptr &root, node const &res, int index) {
                hicc_debug("remove from %s by position: node(%s), index=%d", to_string().c_str(), res.to_string().c_str(), index);
                auto *saved = root;
                if (auto raised = _remove_one(root, const_cast<node *>(&res), index); raised) {
                    delete raised;
                    --_size;
                    if (saved != root) --_size;
                    return true;
                }
                return false;
            }

            elem_ptr _remove_one(node_ptr &root, node *res, int index) {
                if (!(res && index >= 0)) return nullptr;

                auto *removing = res->_payloads[index];
                assert(removing && "the 'removing' shouldn't be null");

                if (res->_pointers[index] && res->_pointers[index + 1]) {
                    auto *left = res->_pointers[index];
                    auto *right = res->_pointers[index + 1];
                    _merge(root, res, index, left, right);

                } else if (res->_pointers[index] || res->_pointers[index + 1]) {
                    auto which_one = res->_pointers[index + 1] ? index + 1 : index;
                    auto *raising = res->_pointers[which_one];
                    assert(raising);

                    if (which_one == index + 1) {
                        auto first_index = 0;
                        if (auto *raised = raising->_remove_one(root, raising, first_index); raised) {
                            res->_payloads[index] = raised;
                            if (raising->empty()) {
                                delete raising;
                                for (auto t = which_one; t < Degree; ++t) res->_pointers[t] = res->_pointers[t + 1];
                            }
                        }
                    } else if (auto last_index = raising->last_payload_index(); last_index >= 0) {
                        if (auto *raised = raising->_remove_one(root, raising, last_index); raised) {
                            res->_payloads[index] = raised;
                            if (raising->empty()) {
                                delete raising;
                                for (auto t = which_one; t < Degree; ++t) res->_pointers[t] = res->_pointers[t + 1];
                            }
                        }
                    } else if (raising->empty()) {
                        delete raising;
                        for (auto t = which_one; t < Degree - 1; ++t) res->_pointers[t] = res->_pointers[t + 1];
                    } else {
                        assertm(false, "unbelievable state, an empty payloads child node found");
                        for (auto t = which_one; t < Degree; ++t) res->_pointers[t] = res->_pointers[t + 1];
                        for (auto t = which_one; t > Degree - 1; ++t) res->_payloads[t] = res->_payloads[t + 1];
                    }

                } else {
                    for (auto t = index; t < Degree - 1; ++t) res->_payloads[t] = res->_payloads[t + 1];
                    if (res->empty()) {
                        auto *left = res->_pointers[0];
                        auto *right = res->_pointers[1];
                        _merge(root, res, index, left, right);
                        if (auto *p = res->_parent; p) {
                            int z{};
                            for (; z < Degree; z++) {
                                if (auto *vp = p->_pointers[z]; vp == res) {
                                    // p->_pointers[z] = nullptr;
                                    _rotate_left_std(res, z, p);
                                } else if (!vp)
                                    break;
                            }
                        } else
                            delete res;
                    } else {
                        for (auto t = index; t < Degree; ++t) res->_pointers[t] = res->_pointers[t + 1];
                    }
                }

                return removing;
            }

            bool _merge(node_ptr &root, node_ptr parent, int index, node_ptr left, node_ptr right) {
                if (left && right) {
                    auto lc = left->payload_count(), rc = right->payload_count();
                    if (lc + rc >= Degree) {
                        if (lc < 2) {
                            auto *raising = right;
                            auto idx = 0;
                            if (auto *raised = raising->_remove_one(root, raising, idx); raised) {
                                parent->_payloads[index] = raised;
                                if (raising->empty()) {
                                    delete raising;
                                    for (auto t = 1; t < Degree; ++t) parent->_pointers[t] = parent->_pointers[t + 1];
                                } else {
                                    _rotate_r(parent, index, raising);
                                }
                            }
                        } else {
                            auto *raising = left;
                            auto idx = left->last_payload_index();
                            if (auto *raised = raising->_remove_one(root, raising, idx); raised) {
                                parent->_payloads[index] = raised;
                                if (raising->empty()) {
                                    delete raising;
                                    for (auto t = 0; t < Degree; ++t) parent->_pointers[t] = parent->_pointers[t + 1];
                                } else {
                                    _rotate_1(parent, index, raising);
                                }
                            }
                        }
                    } else {
                        *left += *right;      // append the right payloads into left node
                        left->parent(parent); // and make the left node as new root
                        delete right;
                        for (auto t = index + 1; t < Degree; t++) parent->_pointers[t] = parent->_pointers[t + 1];
                        for (auto t = index; t < Degree; t++) parent->_payloads[t] = parent->_payloads[t + 1];

                        if (parent) {
                            parent->_pointers[index] = left;
                            _rotate_1(parent, index, left);
                        } else {
                            root = left;
                        }
                        return true;
                    }
                } else if (left) {
                    root = left->parent(nullptr);
                    return true;
                }
                UNUSED(parent);
                return false;
            }

            void _rotate_left_std(node_ptr res, int z, node_ptr parent) {
                for (auto x = z; x < Degree; x++) {
                    res->_payloads[0] = parent->_payloads[x];
                    if (parent->_pointers[x + 1]) {
                        node_ptr root_tmp;
                        if (auto *ptr = _remove_one(root_tmp, parent->_pointers[x + 1], 0); ptr) {
                            parent->_payloads[x] = ptr;
                            if (parent->_pointers[x + 1]->empty()) {
                                _rotate_left_std(parent->_pointers[x + 1], x + 1, parent);
                            }
                        }
                    }
                }
            }

            void _rotate_1(node_ptr parent, int index, node_ptr left) {
                if (parent) {
                    // rotate if necessary
                    elem_type *pp = parent->_payloads[index];
                    if (pp) {
                    retry_1:
                        for (auto t = 0; t < P_LEN; t++) {
                            elem_type *vp = left->_payloads[t];
                            if (vp) {
                                if (comparer()(*vp, *pp) || *vp == *pp)
                                    continue;

                                elem_type *tmp = pp;
                                parent->_payloads[index] = vp;
                                left->_payloads[t] = tmp;

                                for (auto k = t; k < P_LEN; k++) {
                                    if (left->_payloads[t + 1]) {
                                        if (comparer()(*left->_payloads[t + 1], *left->_payloads[t]) || *vp == *pp) {
                                            tmp = left->_payloads[t];
                                            left->_payloads[t] = left->_payloads[t + 1];
                                            left->_payloads[t + 1] = tmp;
                                        }
                                    } else
                                        break;
                                }
                                goto retry_1;
                            } else
                                break;
                        }
                    }
                }
            }

            void _rotate_r(node_ptr parent, int index, node_ptr right) {
                if (parent) {
                    // rotate right branch if necessary
                    elem_type *pp = parent->_payloads[index];
                    if (pp) {
                    retry_1:
                        for (auto t = 0; t < P_LEN; t++) {
                            elem_type *vp = right->_payloads[t];
                            if (vp) {
                                if (comparer()(*pp, *vp) || *vp == *pp)
                                    continue;

                                elem_type *tmp = pp;
                                parent->_payloads[index] = vp;
                                right->_payloads[t] = tmp;
                                goto retry_1;
                            } else
                                break;
                        }
                    }
                }
            }

            node &operator+(node &rhs) {
                node &lhs = (*this);
                assert(lhs.payload_count() + rhs.payload_count() < Degree);
                // auto lc = lhs.payload_count(), rc = lhs.payload_count();
                int idx = 0;
                for (int i = 0; i < Degree; ++i) {
                    if (lhs._payloads[i] == nullptr) {
                        idx = i;
                        break;
                    }
                }

                for (int i = 0; i < Degree; ++i) {
                    if (rhs._payloads[i] != nullptr) {
                        lhs._payloads[idx] = rhs._payloads[i];
                        rhs._payloads[i] = nullptr;
                        if (lhs._pointers[idx] && rhs._pointers[i]) {
                            // merge the two children, recursively if necessary
                            // assertm(false, "not implement yet");
                            node_ptr root = &lhs;
                            _merge(root, &lhs, idx, lhs._pointers[idx], rhs._pointers[i]);
                        } else {
                            lhs._pointers[idx] = rhs._pointers[i];
                        }
                        rhs._pointers[i] = nullptr;
                        idx++;
                    }
                }

                return (*this);
            }
            node &operator+=(node &rhs) {
                this->operator+(rhs);
                return (*this);
            }

        public:
            // compare the pointers strictly
            bool operator==(const_node_ref &rhs) const {
                for (int k = 0; k < Degree - 1; ++k) {
                    if (_payloads[k] != rhs._payloads[k])
                        return false;
                }
                for (int k = 0; k < Degree; ++k) {
                    if (_pointers[k] != rhs._pointers[k])
                        return false;
                }
                if (_parent != rhs._parent)
                    return false;
                return true;
            }

        private:
            int last_payload_index() const {
                if (_payloads[0] != nullptr) {
                    for (auto i = 1; i < Degree; ++i) {
                        if (_payloads[i] == nullptr)
                            return (i - 1);
                    }
                }
                return -1;
            }

            node_ref LNR(visitor const &visitor, int level, int ptr_index) {
                bool node_changed{true};
                for (int i = 0, pi = 0; i < P_LEN; i++, pi++) {
                    auto *n = _pointers[i];
                    if (n)
                        if (auto &z = n->LNR(visitor, level + 1, ptr_index); is_null(z))
                            return (*this);

                    auto *t = _payloads[pi];
                    if (t) {
                        if (visitor)
                            if (!visitor(*t, *this, level, node_changed, ptr_index, false, i))
                                return _null_node();
                    }

                    node_changed = false;
                }

                auto *n = _pointers[Degree - 1];
                if (n) n->LNR(visitor, level + 1, ptr_index);
                return (*this);
            }

            node_ref NLR(visitor const &visitor, int level, int parent_ptr_index, int loop_base = 0) {
                bool parent_ptr_changed{loop_base == 0};
                if (level == 0) {
                    if (!_visit_payloads(this, level, parent_ptr_index, parent_ptr_changed, parent_ptr_index + loop_base, visitor))
                        return (*this);
                    parent_ptr_changed = false;
                }

                for (int pi = 0; pi < Degree; pi++) {
                    auto *n = _pointers[pi];
                    if (n) {
                        assert(n->_parent == this);
                        if (!_visit_payloads(n, level + 1, parent_ptr_index, parent_ptr_changed,
                                             pi + loop_base, visitor))
                            return (*this);
                        parent_ptr_changed = false;
                    } else
                        break;
                }

                int count{0};
                for (int pi = 0; pi < Degree; pi++) {
                    auto *n = _pointers[pi];
                    if (n) {
                        if (auto &z = n->NLR(visitor, level + 1, pi, count); is_null(z)) {
                            count += n->payload_count() + 1;
                            return (*this);
                        }
                        count += n->payload_count() + 1;
                    }
                }
                return (*this);
            }

            // level traverse
            node_ref Level(visitor const &visitor) {
                std::list<std::tuple<node_ptr, int /*index*/, int /*loop_base*/, int /*parent_ptr_index*/>> queue;
                queue.push_back({this, 0, 0, 0});
                auto level{0};

                while (!queue.empty()) {
                    auto [curr, index, loop_base, parent_ptr_index] = queue.front();
                    queue.pop_front();

                    if (curr) {
                        if (!_visit_payloads(curr, level, parent_ptr_index, index == 0,
                                             parent_ptr_index + index + loop_base, visitor))
                            return (*this);

                        auto loop_base_tmp = loop_base;
                        for (auto i = 0; i < Degree; i++) {
                            if (auto p = curr->_pointers[i]; p) {
                                queue.push_back({p, i, loop_base_tmp, index});
                                loop_base_tmp += p->payload_count();
                            }
                        }
                    }

                    if (index == 0) level++;
                }
                return (*this);
            }

            // depth order traverse
            node_ref Depth(visitor const &visitor) {
                std::list<std::tuple<node_ptr, int /*index*/, int /*loop_base*/, int /*parent_ptr_index*/>> queue;
                queue.push_back({this, 0, 0, 0});
                auto level{0};

                while (!queue.empty()) {
                    auto [curr, index, loop_base, parent_ptr_index] = queue.back();
                    queue.pop_back();

                    if (curr) {
                        if (!_visit_payloads(curr, level, parent_ptr_index, index == 0,
                                             parent_ptr_index + index + loop_base, visitor))
                            return (*this);

                        auto loop_base_tmp = loop_base;
                        for (auto i = 0; i < Degree; i++) {
                            if (auto p = curr->_pointers[i]; p) {
                                queue.push_back({p, i, loop_base_tmp, index});
                                loop_base_tmp += p->payload_count();
                            }
                        }
                    }

                    if (index == 0) level++;
                }
                return (*this);
            }

            static bool _visit_payloads(node_ptr ptr, int level, int parent_ptr_index, bool parent_ptr_changed, int ptr_index, visitor const &visitor) {
                bool node_changed{true};
                for (int pi = 0; pi < Degree - 1; pi++) {
                    auto *t = ptr->_payloads[pi];
                    if (t) {
                        if (visitor)
                            if (!visitor(*t, *ptr, level, node_changed,
                                         parent_ptr_index, parent_ptr_changed, ptr_index))
                                return false;
                        node_changed = false;
                    } else
                        break;
                }
                return true;
            }

            std::tuple<bool, int> _find_free_slot(int start_index) const {
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

        public:
            static Comp &comparer() {
                static Comp _c{};
                return _c;
            }

            static std::string elem_to_string(elem_type const *a) {
                std::ostringstream os;
                os << (*a);
                return os.str();
            }

            std::string to_string() const {
                std::ostringstream os;
                os << '[';
                for (int i = 0; i < Degree - 1; ++i) {
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
                assert(this);
                int i{0};
                for (; i < Degree - 1; ++i) {
                    if (auto *vp = _payloads[i]; vp == nullptr) {
                        break;
                    }
                }
                return i;
            }

            bool empty() const { return _payloads[0] == nullptr; }
        };

    public:
        template<typename... Args>
        void insert(Args &&...args) {
            (_root->insert_one(_root, args), ...);
            DEBUG_ONLY(assert_it());
        }

        void insert(elem_type *a_new_data_ptr) {
            assert(a_new_data_ptr);
            _root->insert_one(_root, a_new_data_ptr);
            DEBUG_ONLY(assert_it());
        }

        template<class A, typename... Args,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A, Args...>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a0, Args &&...args) {
            _root->emplace(_root, std::forward<A>(a0), std::forward<Args>(args)...);
            DEBUG_ONLY(assert_it());
        }
        template<class A,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a) {
            _root->emplace(_root, std::forward<A>(a));
            DEBUG_ONLY(assert_it());
        }

        template<typename... Args>
        void remove(Args &&...args) {
            (_root->remove(_root, args), ...);
            DEBUG_ONLY(assert_it());
        }

        void remove(elem_type *el) {
            _root->remove(_root, el);
            DEBUG_ONLY(assert_it());
        }

        void remove_by_position(node const &res, int index) {
            _root->remove_by_position(_root, res, index);
            DEBUG_ONLY(assert_it());
        }
        void remove_by_position(position pos) {
            if (std::get<0>(pos))
                _root->remove_by_position(_root, std::get<1>(pos), std::get<2>(pos));
            DEBUG_ONLY(assert_it());
        }

        bool exists(elem_type const &data) const {
            bool found{};
            _root->walk([data, &found](
                                const_elem_ref el,
                                const_node_ref node,
                                int level, bool node_changed,
                                int parent_ptr_index, bool parent_ptr_changed,
                                int ptr_index) -> bool {
                if (el == data) {
                    found = true;
                    return false;
                }
                UNUSED(el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true; // false to terminate the walking
            });
            return found;
        }

        position find(elem_type const &data) const {
            node const *res{};
            int index{};
            _root->walk([data, &res, &index](
                                typename hicc::btree::btree<T, Degree>::const_elem_ref el,
                                typename hicc::btree::btree<T, Degree>::const_node_ref node,
                                int level, bool node_changed,
                                int parent_ptr_index, bool parent_ptr_changed,
                                int ptr_index) -> bool {
                if (el == data) {
                    res = &node;
                    index = ptr_index;
                    return false;
                }
                UNUSED(el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true; // false to terminate the walking
            });
            if (res)
                return {true, *res, index};
            return {false, _null_node(), -1};
        }

        position find_by_index(int index) const {
            node const *res{};
            int saved{index};
            _root->walk([&res, &index, saved](
                                typename hicc::btree::btree<T, Degree>::const_elem_ref el,
                                typename hicc::btree::btree<T, Degree>::const_node_ref node,
                                int level, bool node_changed,
                                int parent_ptr_index, bool parent_ptr_changed,
                                int ptr_index) -> bool {
                if (index-- == 0) {
                    hicc_debug("find_by_index(%d) -> node(%s) index(%d)", saved, node.to_string().c_str(), ptr_index);
                    res = &node;
                    index = ptr_index;
                    return false;
                }
                UNUSED(saved, el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true; // false to terminate the walking
            });
            if (res)
                return {true, *res, index};
            return {false, _null_node(), -1};
        }

        const_node_ptr find(std::function<bool(const_elem_ptr, const_node_ptr)> const &matcher) const {
            UNUSED(matcher);
            return nullptr;
        }

        void reset() { _root->reset(); }
        void clear() { _root->reset(); }
        size_type size() const { return _root->_size + 1; }
        size_type capacity() const { return _root->_size + 1; }

        const_node_ref walk(visitor const &visitor_) const { return _root->walk(visitor_); }
        node_ref walk(visitor const &visitor_) { return _root->walk(visitor_); }
        const_node_ref walk_nlr(visitor const &visitor_) const { return _root->walk_nlr(visitor_); }
        node_ref walk_nlr(visitor const &visitor_) { return _root->walk_nlr(visitor_); }
        const_node_ref walk_level_traverse(visitor const &visitor_) const { return _root->walk_level_traverse(visitor_); }
        node_ref walk_level_traverse(visitor const &visitor_) { return _root->walk_level_traverse(visitor_); }

        static bool is_null(T const &data) { return data == _null_elem(); }
        static bool is_null(node const &data) { return data == _null_node(); }

#if defined(_DEBUG) || 1

        void dbg_dump(std::ostream &os, const char *headline = nullptr) const {
            if (headline) os << headline;
            walk_level_traverse([&os](const_elem_ref el,
                                      const_node_ref node,
                                      int level, bool node_changed,
                                      int parent_ptr_index, bool parent_ptr_changed,
                                      int ptr_index) -> bool {
                if (node_changed) {
                    if (ptr_index == 0)
                        os << '\n'
                           << ' ' << ' '; //  << 'L' << level << '.' << ptr_index;
                    else if (parent_ptr_changed)
                        os << ' ' << '|' << ' ';
                    else
                        os << ' ';
                    // if (node._parent == nullptr) os << '*'; // root here
                    os //<< ptr_index << '.'
                            << node.to_string();
                    // if(level==0)
                    // os << " (size = " << node._size << ')';
                    os << '/' << node._size;
                }
                os.flush();
                UNUSED(el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true; // false to terminate the walking
            });
            os << '\n';
        }

        void assert_it() const {
            walk([this](typename hicc::btree::btree<T, Degree>::const_elem_ref el,
                        typename hicc::btree::btree<T, Degree>::const_node_ref the_node,
                        int level, bool node_changed,
                        int parent_ptr_index, bool parent_ptr_changed,
                        int ptr_index) -> bool {
                int i;
                for (i = 0; i < Degree; i++) {
                    auto const *vp = the_node._pointers[i];
                    if (vp) {
                        if (vp->_parent != &the_node) {
                            std::ostringstream os, os1;
                            if (vp->_parent) os1 << '(' << vp->_parent->to_string() << ')';
                            os << "node " << vp->to_string() << " has parent " << vp->_parent << os1.str() << ". but not equal to " << &the_node << " (" << the_node.to_string() << ").\n";
                            dbg_dump(os);
                            assertm(false, os.str());
                        }
                    }
                }

                for (i = 0; i < Degree; i++) {
                    auto *data = the_node._payloads[i];
                    auto const *cp = the_node._pointers[i];
                    if (data) {
                        if (cp) {
                            for (int k = 0; k < Degree; ++k) {
                                auto *vp = cp->_payloads[k];
                                if (vp) {
                                    if (node::comparer()(*vp, *data) == false && *vp != *data) {
                                        dbg_dump(std::cerr);
                                        std::ostringstream os;
                                        os << "left children (" << node::elem_to_string(vp) << ") should be less than its parent (" << node::elem_to_string(data) << ").";
                                        assertm(false, os.str());
                                    }
                                }
                            }
                        }
                    } else if (cp && i > 0 && the_node._payloads[i - 1] == nullptr) {
                        std::ostringstream os;
                        os << "for node " << the_node.to_string() << ", payloads[" << i << "] and #" << (i - 1) << " are both null but pointers[" << i << "] is pointing to somewhere (" << cp->to_string() << ").";
                        assertm(false, os.str());
                    }
                    if (data == nullptr)
                        break;
                }

                auto *data = the_node._payloads[i - 1];
                auto *cp = the_node._pointers[i];
                if (cp && data) {
                    for (int k = 0; k < Degree; ++k) {
                        auto *vp = cp->_payloads[k];
                        if (vp) {
                            bool lt = node::comparer()(*data, *vp);
                            if (!lt) {
                                // excludes if they are equal.
                                if (*data != *vp) {
                                    std::ostringstream os;
                                    os << "right child " << node::elem_to_string(vp) << " should be greater than its parent " << node::elem_to_string(data) << '\n';
                                    dbg_dump(os);
                                    assertm(lt, os.str());
                                }
                            }
                        }
                    }
                }

                // for (int k = 0; k < Degree; ++k) {
                //     auto *vp = cp->_payloads[k];
                //     if (vp) {
                //         bool lt = node::comparer()(*data, *vp);
                //         if (!lt) {
                //             std::ostringstream os;
                //             os << "right child " << node::elem_to_string(vp) << " should be greater than its parent " << node::elem_to_string(data);
                //             assertm(lt, os.str());
                //         }
                //         // assertm(node::comparer()(*vp, *data), "left children should be less than its parent");
                //     }
                // }

                //                             for (int k = 0; k < Degree; ++k) {
                //                                 auto *vp = cp->_payloads[k];
                //                                 if (vp) {
                //                                     lt = node::comparer()(*data, *vp);
                //                                     if (!lt) {
                //                                         std::ostringstream os;
                //                                         os << "right child " << node::elem_to_string(vp) << " should be greater than its parent " << node::elem_to_string(data);
                //                                         assertm(lt, os.str());
                //                                         std::cerr << os.str() << '\n';
                //                                         // [16,31]
                //                                         // [3,7,10,13] [18,21] [55,87]
                //                                         // [1,2] [5,6] [8,9] [11,12] [14,15] | [17,17] \
// // [19,20] [28,29] | [33,47] [56,66,69] [78]
                //                                     }
                //                                 }
                //                             }

                UNUSED(el, the_node, node_changed, level, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true;
            });
        }

#endif

    protected:
        static T &_null_elem() {
            static T _d;
            return _d;
        }
        static node_ref _null_node() {
            static node _d;
            return _d;
        }
        void _clear_all() {
            // if (_root) {
            _root->reset();
            delete _root;
            _root = nullptr;
            // }
        }

    private:
        node_ptr _root{};
        node_ptr _first_leaf{};
        node_ptr _last_leaf{};
    };

    template<class T, int Degree, class Comp>
    inline btree<T, Degree, Comp>::btree()
        : _root(new node{}) {
    }

    template<class T, int Degree, class Comp>
    inline btree<T, Degree, Comp>::~btree() {
        _clear_all();
    }


} // namespace hicc::btree


#endif //HICC_CXX_HZ_B_TREE_HH
