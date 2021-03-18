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
    template<class T, int Degree = 5, class Comp = std::less<T>, bool auto_dot_png = false>
    class btree final {
    public:
        btree();

        ~btree();

    public:
        struct node;
        struct traversal_context;

        using node_ptr = node *;
        using node_ref = node &;
        using const_node_ptr = node const *;
        using const_node_ref = node const &;
        using node_sptr = std::shared_ptr<node>;

        using size_type = std::size_t;

        using position = std::tuple<bool /*ok*/, const_node_ref, int>;
        using lite_position = std::tuple<int, const_node_ref>;
        using lite_ptr_position = std::tuple<int, const_node_ptr>;

        using elem_type = T;
        using elem_ptr = elem_type *;
        using const_elem_ptr = elem_type const *;
        using elem_ref = elem_type &;
        using const_elem_ref = elem_type const &;

        // using visitor = std::function<bool(const_elem_ref, const_node_ref, int level, bool node_changed,
        //                                    int ptr_parent_index, bool parent_ptr_changed, int ptr_index)>;

        using visitor_l = std::function<bool(traversal_context const &)>;

        static const int P_LEN = Degree - 1;
        static const int MID = P_LEN / 2;

    public:
        struct node {
            // Container<T> _payload{};
            // Container<node_ptr> _pointers{};
            // std::array<node_ptr, Degree> _payloads{};
            // std::array<node_ptr, Degree+1> _pointers{};
            T *_payloads[Degree + 1];
            node_ptr _pointers[Degree + 1];
            node_ptr _parent{};

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
            void emplace(btree &bt, A &&a0, Args &&...args) {
                elem_type v(std::forward<A>(a0), std::forward<Args>(args)...);
                insert_one(bt, v);
            }

            template<class A,
                     std::enable_if_t<
                             std::is_constructible<elem_type, A>::value &&
                                     !std::is_same<std::decay_t<A>, elem_type>::value &&
                                     !std::is_same<std::decay_t<A>, node>::value,
                             int> = 0>
            void emplace(btree &bt, A &&a) {
                elem_type v(std::forward<A>(a));
                insert_one(bt, v);
            }

            void insert_one(btree &bt, elem_type &&a) { _insert_one_entry(bt, &a); }
            // void insert_one(node_ptr &root, elem_type const &a) { _insert_one(root, &a, false); }
            void insert_one(btree &bt, elem_type a) { _insert_one_entry(bt, new elem_type(std::move(a))); }
            void insert_one(btree &bt, elem_type *a) { _insert_one_entry(bt, a); }

            void remove(btree &bt, elem_type &&a) { _remove_one_entry(bt, &a); }
            void remove(btree &bt, elem_type a) { _remove_one_entry(bt, &a); }
            void remove(btree &bt, elem_type const *a) { _remove_one_entry(bt, a); }

            void remove_by_position(btree &bt, node const &res, int index) { _remove_one_entry(bt, res, index); }

            // const_node_ref walk(visitor const &visitor) const { return LNR(visitor, 9); }
            const_node_ref walk(visitor_l const &visitor) const {
                int x{0};
                return LNR(visitor, x, 0, 0, 0);
            }

            const_node_ref walk_nlr(visitor_l const &visitor) const {
                int x{0};
                return NLR(visitor, x, 0, 0, 0);
            }

            const_node_ref walk_level_traverse(visitor_l const &visitor) const { return Level(visitor); }

            const_node_ref walk_depth_traverse(visitor_l const &visitor) const { return Depth(visitor); }

            bool exists(elem_type const &data) const {
                bool found{};
                walk([data, &found](
                             const_elem_ref el,
                             const_node_ref node_this,
                             int level, bool node_changed,
                             int parent_ptr_index, bool parent_ptr_changed,
                             int ptr_index) -> bool {
                    if (el == data) {
                        found = true;
                        return false;
                    }
                    if (el < data) {
                        // continue to search
                    } else {
                        return false; // not found
                    }
                    UNUSED(el, node_this, level, node_changed);
                    UNUSED(parent_ptr_index, parent_ptr_changed, ptr_index);
                    return true; // false to terminate the walking
                });
                return found;
            }

            position find(elem_type const &data) const {
                int index{};
#if 1
                while (index < Degree - 1 && _payloads[index] && comparer()(*(_payloads[index]), data))
                    index++;
                if (index < Degree - 1 && _payloads[index] && !comparer()(data, *(_payloads[index])))
                    return {true, *this, index};
                if (_pointers[index] == nullptr) // is leaf?
                    return {false, _null_node(), -1};
                return _pointers[index]->find(data);
#else
                node const *res{};
                walk([data, &res, &index](
                             const_elem_ref el,
                             const_node_ref node_this,
                             int level, bool /*node_changed*/,
                             int /*parent_ptr_index*/, bool /*parent_ptr_changed*/,
                             int ptr_index) -> bool {
                    if (el == data) {
                        res = &node_this;
                        index = ptr_index;
                        return false;
                    }
                    if (el < data) {
                        //
                    } else {
                        return false; // not found
                    }
                    UNUSED(level);
                    return true; // false to terminate the walking
                });
                if (res)
                    return {true, *res, index};
                return {false, _null_node(), -1};
#endif
            }

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

            const_node_ref find(std::function<bool(const_elem_ptr, const_node_ptr, int /*level*/, bool /*node_changed*/,
                                                   int /*index*/, int /*abs_index*/)> const &matcher) const {
                walk([matcher](traversal_context const &ctx) -> bool {
                    return matcher(ctx.el, ctx.curr, ctx.level, ctx.node_changed, ctx.index, ctx.abs_index);
                    //return true; // false to terminate the walking
                });
                return (*this);
            }

            void _reset() {
                for (int i = 0; i < Degree; i++) {
                    _pointers[i] = nullptr;
                    _payloads[i] = nullptr;
                }
                _parent = nullptr;
                //                _size = 0;
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
                _parent = nullptr;
                // _size = 0;
            }

        private:
            void _insert_one_entry(btree &bt, elem_ptr a) {
                std::ostringstream orig, elem;
                orig << to_string();
                elem << elem_to_string(a);
                hicc_debug("insert '%s' to %s", elem.str().c_str(), orig.str().c_str());

                _insert_node(bt._root, a, nullptr, nullptr);
                bt._size++;
                if (bt._after_inserted)
                    bt._after_inserted(bt, a);
                if (bt._after_changed)
                    bt._after_changed(bt);
                if (auto_dot_png) {
                    std::array<char, 200> name;
                    std::sprintf(name.data(), "auto.%s%04lu._insert.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
                    std::ostringstream ttl;
                    ttl << "insert '" << elem.str() << "' into " << orig.str();
                    bt.dot(name.data(), ttl.str().c_str());
                }
            }

            void _insert_node(node_ptr &root, elem_ptr el,
                              node_ptr el_left_child = nullptr,
                              node_ptr el_right_child = nullptr) {
                bool in_recursive = el_left_child || el_right_child;
                for (int i = 0; i < P_LEN; i++) {
                    auto *vp = _payloads[i];
                    if (vp == nullptr) {
                        if (in_recursive) {
                            hicc_verbose_debug("  -> put...move '%s' up to parent node.", elem_to_string(el).c_str());
                            _payloads[i] = el;
                            if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
                            if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
                            return;
                        }

                        if (_pointers[i]) {
                            // has_child? forward a into it
                            _pointers[i]->_insert_node(root, el);
                            return;
                        }

                        _payloads[i] = el;
                        // root->_size++;
                        return;
                    }

                    if (comparer()(*el, *vp)) {
                        // less than this one in payloads[], insert it

                        if (!in_recursive) {
                            // has_child? forward a into it
                            if (_pointers[i]) {
                                _pointers[i]->_insert_node(root, el);
                                return;
                            }
                        }

                        // recursively, or no child, insert at position `i`
                        auto [has_slot, j] = _find_free_slot(i);
                        if (has_slot) {
                            for (int pos = j; pos > i; --pos) _payloads[pos] = _payloads[pos - 1];
                            _payloads[i] = el;
                            // if (!in_recursive) root->_size++;
                            if (el_left_child || el_right_child) {
                                hicc_verbose_debug("  -> splitting...move '%s' up to parent node.",
                                                   elem_to_string(el).c_str());
                                for (auto t = Degree; t >= i; t--) _pointers[t] = _pointers[t - 1];
                                if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
                                if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
                            }
                            return;
                        }

#ifdef _DEBUG
                        if (in_recursive)
                            hicc_debug("    split this node again: %s + %s...", this->to_string().c_str(), elem_to_string(el).c_str());
                        else
                            hicc_debug("  split this node: %s + %s...", this->to_string().c_str(), elem_to_string(el).c_str());
#endif
                        for (auto t = Degree - 1; t > i; t--) _payloads[t] = _payloads[t - 1];
                        for (auto t = Degree; t > i; t--) _pointers[t] = _pointers[t - 1];
                        _payloads[i] = el;
                        // if (!in_recursive) root->_size++;
                        if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
                        if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
                        _split_and_raise_up(root, i, el_left_child, el_right_child);
                        return;
                    }
                }

                if (!in_recursive) {
                    // has_child? forward `a` into it
                    if (_pointers[P_LEN]) {
                        _pointers[P_LEN]->_insert_node(root, el);
                        return;
                    }
                }

                hicc_debug("  recursively: `%s` is greater than every one in node %s, create a new parent if necessary", elem_to_string(el).c_str(), to_string().c_str());
                _payloads[P_LEN] = el;
                // root->_size++;
                if (el_left_child) _pointers[P_LEN] = el_left_child->parent(this, true);
                if (el_right_child) _pointers[P_LEN + 1] = el_right_child->parent(this, true);
                _split_and_raise_up(root, P_LEN, el_left_child, el_right_child);
            }

            void _split_and_raise_up(node_ptr &root, int a_pos, node_ptr a_left, node_ptr a_right) {
                auto *new_parent_data = _payloads[MID];
                hicc_debug("    split and raise '%s' up...", elem_to_string(new_parent_data).c_str());

                if (_parent) {
                    node_ptr left = this;
                    node_ptr right = new node(this, MID + 1, Degree);
                    for (auto t = MID + 1; t <= Degree; t++) _pointers[t] = nullptr;
                    for (auto t = MID; t < Degree; t++) _payloads[t] = nullptr;
                    _parent->_insert_node(root, new_parent_data, left, right);
                    if (a_pos < MID) {
                        if (a_left) left->_pointers[a_pos] = a_left->parent(left, true);
                        if (a_right) left->_pointers[a_pos + 1] = a_right->parent(left, true);
                    } else if (a_pos >= MID) {
                        auto pos = a_pos - MID - 1;
                        if (a_left) right->_pointers[pos] = a_left->parent(right, true);
                        if (a_right) right->_pointers[pos + 1] = a_right->parent(right, true);
                    }
                    return;
                }

                auto *new_parent_node = new node(new_parent_data);
                node_ptr left = this->parent(new_parent_node, true);
                node_ptr right = (new node(this, MID + 1, Degree))->parent(new_parent_node, true);
                for (auto t = MID + 1; t <= Degree; t++) left->_pointers[t] = nullptr;
                for (auto t = MID; t < Degree; t++) left->_payloads[t] = nullptr;
                new_parent_node->_pointers[0] = left;
                new_parent_node->_pointers[1] = right;

                root = new_parent_node;
            }

            bool _remove_one_entry(btree &bt, const_elem_ptr a) {
                std::ostringstream orig, elem;
                orig << to_string();
                elem << elem_to_string(a);
                hicc_debug("remove '%s' from %s", elem.str().c_str(), orig.str().c_str());
                auto [ok, res, index] = find(*a);
                if (ok) {
                    if (auto raised = _remove_one(bt, bt._root, const_cast<node_ptr>(&res), index); raised) {
                        delete raised;
                        if (bt._root) {
#ifdef _DEBUG
                            if (bt._size <= 0)
                                assert(false && "null root node?");
#endif
                            --bt._size;
                        }
                        if (bt._after_removed)
                            bt._after_removed(bt, a);
                        if (bt._after_changed)
                            bt._after_changed(bt);
                        if (auto_dot_png) {
                            std::array<char, 200> name;
                            std::sprintf(name.data(), "auto.%s%04lu._remove.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
                            std::ostringstream ttl;
                            ttl << "remove '" << elem.str() << "' from " << orig.str();
                            bt.dot(name.data(), ttl.str().c_str());
                        }
                        return true;
                    }
                }
                return false;
            }

            bool _remove_one_entry(btree &bt, const_node_ref res, int index) {
                std::ostringstream orig, elem;
                orig << to_string();
                elem << elem_to_string(res.get_el(index));
                hicc_debug("remove '%s' from %s by position: node(%s), index=%d",
                           elem.str().c_str(), to_string().c_str(), res.to_string().c_str(), index);
                // auto *saved = root;
                if (auto raised = _remove_one(bt, bt._root, const_cast<node *>(&res), index); raised) {
                    --bt._size;
                    if (bt._after_removed)
                        bt._after_removed(bt, raised);
                    if (bt._after_changed)
                        bt._after_changed(bt);
                    if (auto_dot_png) {
                        std::array<char, 200> name;
                        std::sprintf(name.data(), "auto.%s%04lu._remove.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
                        std::ostringstream ttl;
                        ttl << "remove '" << elem.str() << "' from " << orig.str();
                        bt.dot(name.data(), ttl.str().c_str());
                    }
                    delete raised;
                    // if (saved != root) --_size;
                    return true;
                }
                return false;
            }

            elem_ptr _remove_one(btree &bt, node_ptr &root, node *res, int index, bool dont_balance = false) {
                if (!(res && index >= 0)) return nullptr;

                auto *removing = res->_payloads[index];
                assert(removing && "the 'removing' shouldn't be null");

                if (res->has_children()) {
                    if (auto [idx, ref] = next_minimal_payload(res, index, removing); ref) {
                        hicc_debug("      . replace with '%s' (idx=%d)...", elem_to_string(ref._payloads[idx]).c_str(), idx);
                        if (auto raised = _remove_one(bt, root, const_cast<node_ptr>(&ref), idx, true); raised) {
                            res->_payloads[index] = raised;
                            if (ref.empty()) {
                                if (auto ii = parent_pointers_idx(&ref); ii >= 0) {
                                    if (auto *sibling = res->_pointers[ii + 1]; sibling) {
                                        _rotate_sibling_always(root, const_cast<node_ptr>(&ref), 0, ref._parent, ii, sibling->_payloads[0]);
                                        _remove_one(bt, root, sibling, 0, true);
                                        if (sibling->empty()) {
                                            delete res->_pointers[ii + 1];
                                            res->_pointers[ii + 1] = nullptr;
                                        }
                                    } else {
                                        delete res->_pointers[ii];
                                        res->_pointers[ii] = nullptr;
                                        //         for (auto t = ii; t < Degree; t++) res->_pointers[t] = res->_pointers[t + 1];}
                                    }
                                }
                            } else if (res->_parent) {
                                DEBUG_ONLY(dbg_dump(bt, std::cout, "BEFORE rebalance/pull_up..."));
                                auto child_cnt = ref.payload_count();
                                auto cnt = res->payload_count();
                                if (cnt + child_cnt <= Degree - 1)
                                    _pull_up(root, res, index, const_cast<node_ptr>(&ref));
                                else
                                    _rebalance(bt, root, const_cast<node_ptr>(&ref), idx, ref._payloads[idx]);
                            }
                        }
                        return removing;
                    }
                    // return removing;
                }

                // remove 'removing' directly
                for (auto t = index; t < Degree - 1; t++) res->_payloads[t] = res->_payloads[t + 1];
                if (!dont_balance)
                    _rebalance(bt, root, res, index, removing);

                UNUSED(root, res, index);
                return removing;
            }

            void _rebalance(btree &bt, node_ptr &root, node *res, int index, elem_ptr removing) {
                if (auto cnt = res->payload_count(); cnt < Degree / 2 && res->_parent) {
                    auto parent_idx = parent_pointers_idx(res);
                    if (parent_idx >= 0) {
                        auto parent = res->_parent;
                        if (cnt == 0) {
                            auto *p = parent->_pointers[parent_idx];
                            parent->_pointers[parent_idx] = nullptr;
                            if (auto *c = p->_pointers[0]; c) {
                                parent->_pointers[parent_idx] = c->parent(p->_parent);
                                p->_reset();
                            }
                            delete p;
                            return;
                        }
                        if (cnt == 1) {
                            if (auto parent_cnt = parent->payload_count(); parent_cnt + cnt < Degree) {
                                bool simple{};
                                if (res->_payloads[index] == nullptr && is_rightest(parent, parent_idx))
                                    index--, simple = true; // rightest branch
                                else if (parent_idx == 0) {
                                    // // if (res->_pointers[index] && res->_pointers[index + 1])
                                    // //     simple = true; // leftest branch
                                }
                                if (simple) {
                                    _raise_up(root, res, index, parent, parent_idx);
                                    return;
                                }
                            }
                        }
                        if (auto ptr = parent->_pointers[parent_idx + 1]; parent_idx + 1 < Degree && ptr) {
                            // has right sibling
                            if (auto c = ptr->payload_count(); c > Degree / 2) {
                                _rotate_sibling(bt, root, res, index, res->_parent, parent_idx, ptr);
                                return;
                            }
                            _merge_sibling(bt, root, res, index, res->_parent, parent_idx, const_cast<node_ptr>(ptr), true);
                            return;
                        }
                        if (auto ptr = parent->_pointers[parent_idx - 1]; parent_idx > 0 && ptr) {
                            // has left sibling
                            if (ptr->payload_count() > Degree / 2) {
                                _rotate_sibling(bt, root, res, index, res->_parent, parent_idx, ptr);
                                return;
                            }
                            _merge_sibling(bt, root, res, index, res->_parent, parent_idx, const_cast<node_ptr>(ptr), false);
                            return;
                        }
                    }
                    _pull_up(root, res, index, res);

                } else if (cnt == 0) {
                    auto *p = res->_parent;
                    if (p) {
                        for (auto t = 0; t <= Degree; t++) {
                            if (p->_pointers[t] == res) {
                                p->_pointers[t] = nullptr;
                                delete res;
                                break;
                            }
                            if (p->_pointers[t] == nullptr)
                                break;
                        }
                    }
                }
                UNUSED(root, res, index);
                UNUSED(removing);
            }

            void
            _merge_sibling(btree &bt, node_ptr &root, node *res, int index, node_ptr parent,
                           int parent_idx, node_ptr sibling, bool right_side) {
                // for (auto t = index; t < Degree - 1; t++) {
                //     if (res->_payloads[t])
                //         res->_payloads[t] = res->_payloads[t + 1];
                //     else
                //         break;
                // }

                if (right_side) {
                    bool find_first = is_last_one(parent_idx, parent);
                    int saved = find_first ? 0 : res->_last_payload_index();
                    int parent_idx_real = parent_idx;
                    if (find_first) {
                        // last child
                        for (auto t = Degree; t > 0; t--) res->_payloads[t] = res->_payloads[t - 1];
                        for (auto t = Degree; t > 0; t--) res->_pointers[t] = res->_pointers[t - 1];
                        res->_payloads[0] = parent->_payloads[parent_idx];
                    } else {
                        index = saved + 1;
                        // pull down the parent's one into position 'saved'
                        res->_payloads[index++] = parent->_payloads[parent_idx];
                        // and appends all payloads in sibling
                        for (auto t = 0; t <= Degree; t++) {
                            if (auto *p = sibling->_payloads[t]; p) {
                                res->_payloads[index] = p;
                                res->_pointers[index] = sibling->_pointers[t];
                                index++;
                            } else {
                                res->_pointers[index] = sibling->_pointers[t];
                                break;
                            }
                        }
                    }
                    if (sibling) {
                        sibling->_reset();
                        delete sibling;
                    }

                    for (auto t = parent_idx_real; t < Degree; t++) parent->_payloads[t] = parent->_payloads[t + 1];
                    for (auto t = parent_idx + 1; t < Degree; t++) parent->_pointers[t] = parent->_pointers[t + 1];

                    if (parent->empty()) {
                        // res->_size = root->_size;
                        root = res->parent(nullptr, true);
                        parent->_reset();
                        delete parent;
                    } else
                        _rebalance(bt, root, parent, parent_idx_real, parent->_payloads[parent_idx_real]);
                    return;
                }

                auto pi = parent->_payloads[parent_idx] ? parent_idx : parent_idx - 1;

                if (pi < parent_idx) {
                    _merge_sibling(bt, root, sibling, 0, parent, pi, res, true);
                    return;
                }

                for (auto t = Degree; t > 0; t--) res->_payloads[t] = res->_payloads[t - 1];
                for (auto t = Degree; t > 0; t--) res->_pointers[t] = res->_pointers[t - 1];
                res->_payloads[0] = parent->_payloads[pi];

                for (auto x = 1; x < Degree - 1; x++) {
                    if (auto vd = sibling->_payloads[x]; vd) {
                        for (auto t = Degree; t > 0; t--) res->_payloads[t] = res->_payloads[t - 1];
                        for (auto t = Degree; t > 0; t--) res->_pointers[t] = res->_pointers[t - 1];
                        res->_payloads[0] = vd;
                    } else
                        break;
                }

                parent->_payloads[pi] = sibling->_payloads[0];
                for (auto t = parent_idx; t < Degree - 1; t++) parent->_pointers[t] = parent->_pointers[t + 1];
                parent->_pointers[pi] = res;
                sibling->_reset();
                delete sibling;
            }

            // pull down parent[parent_idx] to position 'index'
            void _pull_down(node_ptr &root, node *res, int index, node_ptr parent, int parent_idx) {
                for (auto t = Degree; t > index; t--) res->_payloads[t] = res->_payloads[t - 1];
                for (auto t = Degree; t > index; t--) res->_pointers[t] = res->_pointers[t - 1];
                res->_payloads[index] = parent->_payloads[parent_idx];
                UNUSED(root, res, index);
            }

            void _raise_up(node_ptr &root, node *res, int index, node_ptr parent, int parent_idx) {
                for (auto t = Degree; t > parent_idx; t--) parent->_payloads[t] = parent->_payloads[t - 1];
                for (auto t = Degree; t > parent_idx; t--) parent->_pointers[t] = parent->_pointers[t - 1];
                // if (res->_payloads[index] == nullptr) index--;
                parent->_payloads[parent_idx] = res->_payloads[index];
                parent->_pointers[parent_idx] = res->_pointers[index]->parent(parent);
                parent->_pointers[parent_idx + 1] = res->_pointers[index + 1]->parent(parent);
                res->_reset();
                delete res;
                UNUSED(root);
            }

            //
            void _pull_up(node_ptr &root, node *res, int index, node_ptr child) {
                auto parent_idx = parent_pointers_idx(child);
                res->_pointers[parent_idx] = nullptr;
                for (auto t = 0; t < Degree - 1; t++) {
                    if (auto vd = child->_payloads[t]; vd) {
                        for (auto x = Degree - 1; x > index; x--) res->_payloads[x] = res->_payloads[x - 1];
                        for (auto x = Degree; x > parent_idx; x--) res->_pointers[x] = res->_pointers[x - 1];
                        res->_payloads[++index] = vd;
                    } else
                        break;
                }
                child->_reset();
                delete child;
                UNUSED(root);
            }

            //
            void _rotate_sibling(btree &bt, node_ptr &root, node *res, int index, node_ptr parent, int parent_idx, const_node_ptr sibling) {
                // if(sibling->payload_count()>Degree/2){
                //     // rotate directly
                // }else{
                //     // delete first one, and rotate
                // }
                if (sibling->payload_count() >= Degree / 2 + 1) {
                    auto *removing = sibling->_payloads[0];
                    if (auto *removed = _remove_one(bt, root, const_cast<node_ptr>(sibling), 0, true); removed) {
                        assert(removed == removing);
                        if (_rotate_sibling_always(root, res, index, parent, parent_idx, removed))
                            return;
                    }
                    UNUSED(removing);
                } else {
                    return;
                }
                assert(false && "unexpected error");
            }

            //
            bool _rotate_sibling_always(node_ptr &root, node *res, int index, node_ptr parent, int parent_idx, elem_ptr removed) {
                auto *pull_down = parent->_payloads[parent_idx];
                parent->_payloads[parent_idx] = removed;
                auto ii = index;
                if (res->_payloads[ii] == nullptr) ii--;
                if (ii >= 0) {
                    if (!comparer()(*pull_down, *(res->_payloads[ii])))
                        ii++;
                    for (auto t = Degree; t > ii; t--) res->_payloads[t] = res->_payloads[t - 1];
                    for (auto t = Degree; t > ii; t--) res->_pointers[t] = res->_pointers[t - 1];
                } else
                    ii = 0;
                res->_payloads[ii] = pull_down;
                UNUSED(root);
                return true;
            }

        public:
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

            elem_type const &operator[](int index) const {
                if (index >= 0 && index < Degree - 1)
                    return _payloads[index] ? *_payloads[index] : _null_elem();
                return _null_elem();
            }

            elem_type &operator[](int index) {
                if (index >= 0 && index < Degree - 1)
                    return _payloads[index] ? *_payloads[index] : _null_elem();
                return _null_elem();
            }

            bool operator!() const { return is_null(*this); }

            operator bool() const { return !is_null(*this); }

            bool is_last_one_of_parent(const_node_ptr ptr, int parent_idx) const {
                if (auto parent = ptr->_parent; parent) {
                    if (auto cnt = parent->payload_count(); parent_idx >= cnt)
                        return true;
                }
                return false;
            }
            bool is_last_one(int parent_idx, const_node_ptr parent) const {
                if (parent) {
                    if (auto cnt = parent->payload_count(); parent_idx >= cnt)
                        return true;
                }
                return false;
            }

            int level() const {
                int l{};
                auto *p = this;
                while (p->_parent)
                    p = p->_parent, l++;
                return l;
            }

        private:
            // in-order traversal
            //
            // never used in context:
            //   loop_base_tmp, level_changed,
            //   parent_ptr_index, parent_ptr_index_changed
            const_node_ref LNR(visitor_l const &visitor, int &abs_index, int level, int parent_ptr_index, int loop_base = 0) const {
                bool parent_ptr_changed{loop_base == 0};
                bool node_changed{true};
                int count{0};
                for (int i = 0, pi = 0; i < P_LEN; i++, pi++) {
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

                auto *n = _pointers[Degree - 1];
                if (n) {
                    if (auto &z = n->LNR(visitor, abs_index, level + 1, Degree - 1, count); is_null(z))
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
                    abs_index=ctx.abs_index;
                }

                for (int pi = 0; pi < Degree; pi++) {
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
                        abs_index=ctx.abs_index;
                    } else
                        break;
                }

                int count{0};
                for (int pi = 0; pi < Degree; pi++) {
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
                queue.push_back({*this, &get_el(), level, 0, 0, 0, 0, true, true, true});

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
                        for (auto i = 0; i < Degree; i++) {
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
                        for (auto i = 0; i < Degree; i++) {
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

            static bool
            _visit_payloads(const_node_ref ref, traversal_context &ctx, visitor_l const &visitor) {
                bool node_changed{true};
                for (int pi = 0; pi < Degree - 1; pi++) {
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

            const_elem_ref get_el(int index = 0) const {
                return (index < 0 || index >= Degree - 1) ? _null_elem()
                                                          : (_payloads[index] ? *(_payloads[index]) : _null_elem());
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

            int _last_payload_index() const {
                if (_payloads[0] != nullptr) {
                    for (auto i = 1; i < Degree; ++i) {
                        if (_payloads[i] == nullptr)
                            return (i - 1);
                    }
                }
                return -1;
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

            static std::string elem_to_string(elem_type const &a) {
                std::ostringstream os;
                os << a;
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

            int degree() const { return Degree; }

            const_node_ptr child(int i) const {
                if (i >= 0 && i < Degree) return _pointers[i];
                return nullptr;
            }

            bool has_children() const {
                for (auto i = 0; i < Degree; ++i) {
                    if (_pointers[i] != nullptr)
                        return true;
                }
                return false;
            }

            bool has_children(node const *res, int index) const {
                assert(index >= 0 && index < Degree);
                return (res->_pointers[index] || res->_pointers[index + 1]);
            }

            bool has_left_child(node const *res, int index) const {
                assert(index >= 0 && index < Degree);
                return (res->_pointers[index]);
            }

            bool has_right_children(node const *res, int index) const {
                assert(index >= 0 && index < Degree);
                return (res->_pointers[index + 1]);
            }

            bool has_right_sibling(node const *res, int index, elem_ptr removing) const {
                assert(index >= 0 && index < Degree);
                if (auto const *p = res->_parent; p) {
                    for (auto t = Degree - 1 - 1; t >= 0; t--) {
                        if (p->_payloads[t] == nullptr) continue;
                        if (*removing < *(p->_payloads[t])) continue;
                        if (p->_pointers[t + 1] != nullptr)
                            return p->_pointers[t + 1] != res;
                        break;
                    }
                    return p->_pointers[1] != nullptr;
                }
                UNUSED(index);
                return false;
            }

            lite_ptr_position find_right_sibling(node const *res, int index, elem_ptr removing) const {
                assert(index >= 0 && index < Degree);
                if (auto const *p = res->_parent; p) {
                    for (auto t = Degree - 1 - 1; t >= 0; t--) {
                        if (p->_payloads[t] == nullptr) continue;
                        if (*removing < *(p->_payloads[t])) continue;
                        if (p->_pointers[t + 1] != nullptr)
                            return {t + 1, p->_pointers[t + 1] == res ? nullptr : p->_pointers[t + 1]};
                        break;
                    }
                    return {0, p->_pointers[1]};
                }
                UNUSED(index);
                return {-1, nullptr};
            }

            bool has_left_sibling(node const *res, int index, elem_ptr removing) const {
                assert(index >= 0 && index < Degree);
                if (auto const *p = res->_parent; p) {
                    for (auto t = Degree - 1 - 1; t >= 0; t--) {
                        if (p->_payloads[t] == nullptr) continue;
                        if (*removing < *(p->_payloads[t]))
                            return p->_pointers[t - 1] != nullptr;
                    }
                    if (p->_pointers[0] != res) return true;
                }
                UNUSED(index);
                return false;
            }

            lite_ptr_position find_left_sibling(node const *res, int index, elem_ptr removing) const {
                assert(index >= 0 && index < Degree);
                if (auto const *p = res->_parent; p) {
                    for (auto t = Degree - 1 - 1; t >= 0; t--) {
                        if (p->_payloads[t] == nullptr) continue;
                        if (*removing < *(p->_payloads[t])) {
                            if (p->_pointers[t - 1] != nullptr)
                                return {t - 1, p->_pointers[t - 1]};
                            break;
                        }
                    }
                    return {0, p->_pointers[0] == res ? nullptr : p->_pointers[0]};
                }
                UNUSED(index);
                return {-1, nullptr};
            }

            static int parent_pointers_idx(node const *res) {
                if (res && res->_parent) {
                    for (auto t = Degree - 1; t >= 0; t--) {
                        if (auto p = res->_parent->_pointers[t]; p == nullptr)
                            continue;
                        else if (p == res)
                            return t;
                    }
                }
                return -1;
            }

            static lite_position next_minimal_payload(node const *res, int index, elem_ptr removing) {
                if (res) {
                    assert(index >= -1 && index < Degree);
                    for (auto t = index + 1; t < Degree; t++) {
                        if (auto *p = res->_pointers[t]; p != nullptr)
                            return next_minimal_payload(p, -1, removing);
                        if (auto *p = res->_payloads[t]; p != nullptr)
                            return {t, *res};
                        break;
                    }
                    auto *ptr = res;
                retry_parent:
                    if (auto *p = ptr->_parent; p != nullptr) {
                        if (auto pi = parent_pointers_idx(ptr); pi >= 0) {
                            if (p->_payloads[pi] == nullptr) {
                                ptr = p;
                                goto retry_parent;
                            }
                            if (comparer()(*removing, *(p->_payloads[pi])))
                                return {pi, *p};
                            return next_minimal_payload(p->_pointers[pi + 1], -1, removing);
                        }
                        return {0, *p};
                    }
                }
                return {-1, _null_node()};
            }

            static lite_position next_payload(node const *res, int index, elem_ptr removing) {
                assert(index >= -1 && index < Degree);
                for (auto t = index + 1; t < Degree; t++) {
                    if (auto *p = res->_pointers[t]; p != nullptr)
                        return next_payload(p, -1, removing);

                    if (auto *p = res->_payloads[t]; p != nullptr)
                        return {t, *res};
                }

                if (auto *p = res->_parent; p != nullptr) {
                    for (auto t = Degree - 1; t >= 0; t--) {
                        if (auto *x = p->_payloads[t]; x == nullptr)
                            continue;
                        else if (comparer()(*x, *removing)) {
                            // auto pos = t + 1;
                            return next_payload(p, t, removing);
                        }
                    }
                    return {0, *p};
                }

                return {-1, _null_node()};
            }

            static bool is_rightest(const_node_ptr parent, int parent_idx) {
                return (parent->_pointers[parent_idx] && parent->_pointers[parent_idx] == nullptr);
            }

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

            int total_from_calculating() const {
                int count = 0;
                walk([&count](traversal_context const &ctx) -> bool {
                    count++;
                    UNUSED(ctx);
                    return true;
                });
                return count;
            }

            bool empty() const { return _payloads[0] == nullptr; }

        public:
            void dbg_dump(btree const &bt, std::ostream &os = std::cout, const char *headline = nullptr) const {
                if (headline) os << headline;

                int count = total_from_calculating();

                walk_level_traverse([&bt, count, &os](traversal_context const &ctx) -> bool {
                    if (ctx.node_changed) {
                        if (ctx.index == 0 && ctx.level_changed)
                            os << '\n'
                               << ' ' << ' ';
                        else if (ctx.parent_ptr_index_changed)
                            os << ' ' << '|' << ' ';
                        else
                            os << ' ';
                        os << ctx.curr.to_string();
                        if (ctx.level == 0) {
                            os << '/' << bt._size;
                            os << "/dyn:" << count;
                            assertm(ctx.curr._parent == nullptr, "root's parent must be nullptr");
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
            }
        };

    public:
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

    public:
        template<typename... Args>
        void insert(Args &&...args) {
            (_root->insert_one(*this, args), ...);
            DEBUG_ONLY(assert_it());
        }

        void insert(elem_type *a_new_data_ptr) {
            assert(a_new_data_ptr);
            _root->insert_one(*this, a_new_data_ptr);
            DEBUG_ONLY(assert_it());
        }

        template<class A, typename... Args,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A, Args...>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a0, Args &&...args) {
            _root->emplace(*this, std::forward<A>(a0), std::forward<Args>(args)...);
            DEBUG_ONLY(assert_it());
        }

        template<class A,
                 std::enable_if_t<
                         std::is_constructible<elem_type, A>::value &&
                                 !std::is_same<std::decay_t<A>, node>::value,
                         int> = 0>
        void emplace(A &&a) {
            _root->emplace(*this, std::forward<A>(a));
            DEBUG_ONLY(assert_it());
        }

        template<typename... Args>
        void remove(Args &&...args) {
            (_root->remove(*this, args), ...);
            DEBUG_ONLY(assert_it());
        }

        void remove(elem_type *el) {
            _root->remove(*this, el);
            DEBUG_ONLY(assert_it());
        }

        void remove_by_position(node const &res, int index) {
            _root->remove_by_position(*this, res, index);
            DEBUG_ONLY(assert_it());
        }

        void remove_by_position(position pos) {
            if (std::get<0>(pos))
                _root->remove_by_position(*this, std::get<1>(pos), std::get<2>(pos));
            DEBUG_ONLY(assert_it());
        }

        //

        bool exists(elem_type const &data) const { return _root->exists(data); }

        //

        position find(elem_type const &data) const { return _root->find(data); }

        position find_by_index(int index) const { return _root->find_by_index(index); }

        const_node_ref find(std::function<bool(const_elem_ptr, const_node_ptr, int /*level*/, bool /*node_changed*/,
                                               int /*ptr_index*/)> const &matcher) const { return find(matcher); }

        //

        lite_position next_payload(const_node_ptr ptr, int index) const {
            return node::next_payload(ptr, index, ptr->_payloads[index]);
        }

        lite_position next_payload(lite_position const &pos) const {
            auto [idx, ptr] = pos;
            return node::next_payload(ptr, idx, ptr->_payloads[idx]);
        }

        lite_position next_payload(position const &pos) const {
            auto [ok, ref, idx] = pos;
            if (ok)
                return node::next_payload(&ref, idx, ref._payloads[idx]);
            return {-1, _null_node()};
        }

        lite_position next_minimal_payload(const_node_ref ref, int index) const {
            return node::next_minimal_payload(&ref, index, ref._payloads[index]);
        }

        lite_position next_minimal_payload(lite_position const &pos) const {
            auto [idx, ptr] = pos;
            return node::next_minimal_payload(ptr, idx, ptr->_payloads[idx]);
        }

        lite_position next_minimal_payload(position const &pos) const {
            auto [ok, ref, idx] = pos;
            if (ok)
                return node::next_minimal_payload(&ref, idx, ref._payloads[idx]);
            return {-1, _null_node()};
        }

        //

        void _reset() { _root->_reset(); }
        void clear() {
            _root->clear();
            _size = 0;
        }

        size_type size() const { return _size; }
        size_type capacity() const { return _size; }

        const_node_ref walk(visitor_l const &v) const { return _root->walk(v); }
        node_ref walk(visitor_l const &v) { return const_cast<node_ref>(_root->walk(v)); }

        const_node_ref walk_nlr(visitor_l const &v) const { return _root->walk_nlr(v); }
        node_ref walk_nlr(visitor_l const &v) { return const_cast<node_ref>(_root->walk_nlr(v)); }

        const_node_ref walk_level_traverse(visitor_l const &v) const { return _root->walk_level_traverse(v); }
        node_ref walk_level_traverse(visitor_l const &v) { return const_cast<node_ref>(_root->walk_level_traverse(v)); }

        const_node_ref walk_depth_traverse(visitor_l const &v) const { return _root->walk_depth_traverse(v); }
        node_ref walk_depth_traverse(visitor_l const &v) { return const_cast<node_ref>(_root->walk_depth_traverse(v)); }

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

        //

        static bool is_null(T const &data) { return data == _null_elem(); }

        static bool is_null(node const &data) { return data == _null_node(); }

        int total_from_calculating() const { return _root->total_from_calculating(); }

#if defined(_DEBUG) || 1

        void dbg_dump(std::ostream &os = std::cout, const char *headline = nullptr) const { _root->dbg_dump(*this, os, headline); }

        void assert_it() const {

            int count = total_from_calculating();
            if ((std::size_t) count != _size)
                dbg_dump();
            assertm((std::size_t) count == _size,
                    "expecting _size is equal to the btree size (countof) exactly");

            walk([this](traversal_context const &ctx) -> bool {
                int i;
                for (i = 0; i < Degree; i++) {
                    auto const *vp = ctx.curr._pointers[i];
                    if (vp) {
                        if (vp->_parent != &ctx.curr) {
                            std::ostringstream os, os1;
                            if (vp->_parent) os1 << '(' << vp->_parent->to_string() << ')';
                            os << "node " << vp->to_string() << " has parent " << vp->_parent << os1.str()
                               << ". but not equal to " << &ctx.curr << " (" << ctx.curr.to_string() << ").\n";
                            dbg_dump(os);
                            assertm(false, os.str());
                        }
                    }
                }

                for (i = 0; i < Degree; i++) {
                    if (auto const *vp = ctx.curr._pointers[i]; !vp)
                        break;
                }
                for (; i < Degree + 1; i++) {
                    auto const *vp = ctx.curr._pointers[i];
                    std::ostringstream os;
                    if (vp) {
                        dbg_dump();
                        os << "extra slot " << i << " in pointers must be nullptr";
                    }
                    assertm(vp == nullptr, os.str().c_str());
                }

                for (i = 0; i < Degree; i++) {
                    if (auto const *vp = ctx.curr._payloads[i]; !vp)
                        break;
                }
                for (; i < Degree + 1; i++) {
                    auto const *vp = ctx.curr._payloads[i];
                    std::ostringstream os;
                    if (vp) {
                        dbg_dump();
                        os << "extra slot " << i << " in payloads must be nullptr";
                    }
                    assertm(vp == nullptr, os.str().c_str());
                }

                for (i = 0; i < Degree; i++) {
                    auto *data = ctx.curr._payloads[i];
                    auto const *cp = ctx.curr._pointers[i];
                    if (data) {
                        if (cp) {
                            for (int k = 0; k < Degree; ++k) {
                                auto *vp = cp->_payloads[k];
                                if (vp) {
                                    if (node::comparer()(*vp, *data) == false && *vp != *data) {
                                        dbg_dump(std::cerr);
                                        std::ostringstream os;
                                        os << "left children (" << node::elem_to_string(vp)
                                           << ") should be less than its parent (" << node::elem_to_string(data)
                                           << ").";
                                        assertm(false, os.str());
                                    }
                                }
                            }
                        }
                    } else if (cp && i > 0 && ctx.curr._payloads[i - 1] == nullptr) {
                        std::ostringstream os;
                        os << "for node " << ctx.curr.to_string() << ", payloads[" << i << "] and #" << (i - 1)
                           << " are both null but pointers[" << i << "] is pointing to somewhere (" << cp->to_string()
                           << ").";
                        assertm(false, os.str());
                    }
                    if (data == nullptr)
                        break;
                }

                auto *data = ctx.curr._payloads[i - 1];
                auto *cp = ctx.curr._pointers[i];
                if (cp && data) {
                    for (int k = 0; k < Degree; ++k) {
                        auto *vp = cp->_payloads[k];
                        if (vp) {
                            bool lt = node::comparer()(*data, *vp);
                            if (!lt) {
                                // excludes if they are equal.
                                if (*data != *vp) {
                                    std::ostringstream os;
                                    os << "right child " << node::elem_to_string(vp)
                                       << " should be greater than its parent " << node::elem_to_string(data) << '\n';
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

                /* for (int k = 0; k < Degree; ++k) {
                    auto *vp = cp->_payloads[k];
                    if (vp) {
                        lt = node::comparer()(*data, *vp);
                        if (!lt) {
                            std::ostringstream os;
                            os << "right child " << node::elem_to_string(vp) << " should be greater than its parent " << node::elem_to_string(data);
                            assertm(lt, os.str());
                            std::cerr << os.str() << '\n';
                            // [16,31]
                            // [3,7,10,13] [18,21] [55,87]
                            // [1,2] [5,6] [8,9] [11,12] [14,15] | [17,17]
                            // [19,20] [28,29] | [33,47] [56,66,69] [78]
                        }
                    }
                } */

                // UNUSED(el, node_changed, level, parent_ptr_index, parent_ptr_changed, ptr_index);
                return true;
            });
        }

        /**
         * @brief dot generate a graphviz .dot file and transform it as a PNG too.
         * @param filename such as 'aa.dot'
         * @param verbose assume 'dot -v ...'
         * @details graphviz must be installed at first, 'dot' should be PATH-searchable.
         */
        void dot(const char *filename, const char *title, bool verbose = false) {
            UNUSED(filename);
            std::unique_ptr<std::ofstream, std::function<void(std::ofstream *)>>
                    ofs(new std::ofstream(filename),
                        [filename, verbose](std::ofstream *os) {
                            os->close();
                            std::cout << "aa.dot was written." << '\n';
                            std::array<char, 512> cmd;
                            std::sprintf(cmd.data(), "dot '%s' -T png -o '%s.png' %s", filename, filename, verbose ? "-v" : "");
                            std::cout << "executing: " << std::quoted(cmd.data()) << '\n';
                            // process::exec dot("dot aa.dot -T png -o aa.png");
                            process::exec dot(cmd.data());
                            std::cout << dot.rdbuf();
                        });

            btree::size_type total = size();
            walk_level_traverse([total, title, &ofs](btree::traversal_context const &ctx) -> bool {
                if (ctx.abs_index == 0) {
                    *ofs << "graph {" << '\n';
                    if (title && *title)
                        *ofs << "labelloc=\"t\";\n"
                             << "label=\"" << title << "\";" << '\n';
                }
                if (ctx.node_changed) {
                    auto from = std::quoted(ctx.curr.to_string());
                    if (ctx.abs_index == 0) {
                        *ofs << from << " [color=green];" << '\n';
                    }
                    for (int i = 0; i < ctx.curr.degree(); ++i) {
                        if (auto const *child = ctx.curr.child(i); child) {
                            *ofs << from << " -- ";
                            *ofs << std::quoted(child->to_string()) << ';'
                                 << ' ' << '#' << ' ' << std::boolalpha
                                 << ctx.abs_index << ':' << ' '
                                 << ctx.level << ',' << ctx.index << ',' << ctx.parent_ptr_index
                                 << ',' << ctx.loop_base << ','
                                 << ctx.level_changed
                                 << ',' << ctx.parent_ptr_index_changed
                                 << '\n';
                        } else
                            break;
                    }
                    *ofs << '\n';
                }
                if (ctx.abs_index == (int) total - 1) {
                    *ofs << "}" << '\n';
                }
                // std::cout << "abs_idx: " << ctx.abs_index << ", total: " << total << '\n';
                return true;
            });
        }
#endif

        size_type dot_seq_inc() { return _seq_no++; }
        btree &dot_seq(size_type seq_no) {
            _seq_no = seq_no;
            return *this;
        }
        btree &dot_prefix(const char *s) {
            if (s && *s) {
                _seq_prefix = s;
                if (_seq_prefix[_seq_prefix.length() - 1] != '.')
                    _seq_prefix += '.';
            } else {
                _seq_prefix.clear();
            }
            return *this;
        }

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
            _root->clear();
            delete _root;
            _root = nullptr;
        }

    private:
        node_ptr _root{};
        size_type _size{};
        node_ptr _first_leaf{};
        node_ptr _last_leaf{};

        std::function<void(btree &, const_elem_ptr a)> _after_inserted;
        std::function<void(btree &, const_elem_ptr a)> _after_removed;
        std::function<void(btree &)> _after_changed;

        size_type _seq_no{};
        std::string _seq_prefix{};
    };

    template<class T, int Degree, class Comp, bool B>
    inline btree<T, Degree, Comp, B>::btree()
        : _root(new node{}) {
    }

    template<class T, int Degree, class Comp, bool B>
    inline btree<T, Degree, Comp, B>::~btree() {
        _clear_all();
    }

    // template<class T, int Degree, class Comp>
    // inline typename btree<T, Degree, Comp>::size_type btree<T, Degree, Comp>::node::_size{};

} // namespace hicc::btree


#endif //HICC_CXX_HZ_BTREE_HH
