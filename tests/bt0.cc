
#include <array>
#include <initializer_list>
#include <iostream>

#include <cstdio>
#include <iomanip>
#include <iostream>

#include <cstdlib>
#include <ctime>

#include <cmath>
#include <fstream>
#include <memory>
#include <random>


const bool use_auto_dot = false;
const bool use_auto_dot_for_case_1 = true;

#include "hicc/hz-btree.hh"
#include "hicc/hz-chrono.hh"
#include "hicc/hz-dbg.hh"
#include "hicc/hz-process.hh"


namespace hicc::btree {

  /**
     * @brief provides B-tree data structure in generic style.
     * 
     * @details The order, or branching factor, `b` of a B tree measures the capacity of 
     * nodes (i.e., the number of children nodes) for internal nodes in the 
     * tree. The actual number of children for a node, referred to here as `m`, 
     * is constrained for internal nodes so that `b/2 <= m <= b`.
     * 
     * @tparam T your data type
     * @tparam Degree the Order of B-Tree, _keys: [(Degree-1)/2,Degree-1], children: [Degree/2,Degree]
     * @tparam Container vector or list here
     * @tparam auto_dot_png enables internal debugging actions for .dot/.png outputtings every inserted/removed.
     */
  template<class T, int Degree = 5, class Comp = std::less<T>, bool auto_dot_png = false>
  class btreeT {
  public:
    btreeT();
    virtual ~btreeT();

    static const bool IS_ODD = (Degree - Degree / 2 * 2);
    static const int MAX_PAYLOADS = Degree - 1;
    static const int MID = MAX_PAYLOADS / 2;
    static const int MIDDER = (MAX_PAYLOADS + 1) / 2;
    static const int M = Degree / 2; // Just for even tree, _keys: [M-1,2M-1], children: [M,2M]

  public:
    struct node;
    struct traversal_context;

    using self_type = btreeT;
    using size_type = std::size_t;

    using node_ptr = node *;
    using node_ref = node &;
    using const_node_ptr = node const *;
    using const_node_ref = node const &;
    using node_sptr = std::shared_ptr<node>;


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

  public:
    // struct pos_t {
    //     const_node_ptr ptr;
    //     int data_idx;
    //     int child_idx;
    // };

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
      bool valid() const { return ptr && pos >= 0 && pos << Degree; }
      explicit operator bool() const { return valid(); }
      bool operator!() const { return !valid(); }
    };

  public:
    struct node {
      // Container<T> _payload{};
      // Container<node_ptr> _pointers{};
      // std::array<node_ptr, Degree> _payloads{};
      // std::array<node_ptr, Degree+1> _pointers{};
      T *_payloads[Degree + 1];
      node_ptr _pointers[Degree + 1];
      node_ptr _parent{};
      counter_type _count{};

      using self_type = node;

      ~node() { clear(); }

      node() {
        for (int i = 0; i < Degree + 1; i++) _pointers[i] = nullptr;
        for (int i = 0; i < Degree + 1; i++) _payloads[i] = nullptr;
      }

      explicit node(elem_type *first_data, elem_type *second_data = nullptr)
          : node() {
        assert(!second_data || (second_data && first_data));
        if (second_data) {
          _payloads[1] = second_data;
          _count++;
        }
        if (first_data) {
          _payloads[0] = first_data;
          _count++;
        }
      }

      explicit node(node_ptr src, int start, int end)
          : node() {
        for (auto t = start, i = 0; t < end; t++, i++) _payloads[i] = src->_payloads[t];
        for (auto t = start, i = 0; t <= end; t++, i++) _pointers[i] = src->_pointers[t];
        _pointers[end - start] = src->_pointers[end];
        _count = (counter_type) dyn_payload_count();
      }

      bool is_root() const { return _parent == nullptr; }
      node_ptr root() {
        node_ptr p = this;
        while (p->parent()) p = p->parent();
        return p;
      }
      node_ptr parent() { return _parent; }
      const_node_ptr parent() const { return _parent; }
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

    public:
      template<class A, typename... Args,
               std::enable_if_t<
                   std::is_constructible<elem_type, A, Args...>::value && !std::is_same<std::decay_t<A>, elem_type>::value && !std::is_same<std::decay_t<A>, node>::value,
                   int> = 0>
      void emplace(btreeT &bt, A &&a0, Args &&...args) {
        elem_type v(std::forward<A>(a0), std::forward<Args>(args)...);
        insert_one(bt, v);
      }

      template<class A,
               std::enable_if_t<
                   std::is_constructible<elem_type, A>::value && !std::is_same<std::decay_t<A>, elem_type>::value && !std::is_same<std::decay_t<A>, node>::value,
                   int> = 0>
      void emplace(btreeT &bt, A &&a) {
        elem_type v(std::forward<A>(a));
        insert_one(bt, v);
      }

      void insert_one(btreeT &bt, elem_type &&a) { _insert_one_entry(bt, &a); }
      // void insert_one(node_ptr &root, elem_type const &a) { _insert_one(root, &a, false); }
      void insert_one(btreeT &bt, elem_type a) { _insert_one_entry(bt, new elem_type(std::move(a))); }
      void insert_one(btreeT &bt, elem_type *a) { _insert_one_entry(bt, a); }

      void remove(btreeT &bt, elem_type &&a) { _remove_entry(bt, &a); }
      void remove(btreeT &bt, elem_type a) { _remove_entry(bt, &a); }
      void remove(btreeT &bt, elem_type const *a) { _remove_entry(bt, a); }

      void remove_by_position(btreeT &bt, node const &res, int index) { _remove_entry(bt, res, index); }

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
            dbg_verbose_debug("find_by_index(%d) -> node(%s) index(%d)",
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
      node_ptr _reset_for_delete() {
        for (int i = 0; i < Degree; i++) {
          _pointers[i] = nullptr;
          _payloads[i] = nullptr;
        }
        _parent = nullptr;
        return this;
      }

    private:
      void _insert_one_entry(btreeT &bt, elem_ptr a) {
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
        std::ostringstream orig, elem;
        orig << to_string();
        elem << elem_to_string(a);
        dbg_debug("insert '%s' to %s", elem.str().c_str(), orig.str().c_str());
#endif
        if (IS_ODD)
          this->_insert_node_for_odd_tree(bt._root, a);
        else
          this->_insert_node_for_even_tree(bt, a);
        bt._size++;
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
        ASSERTIONS_ONLY(dbg_dump(bt, std::cout, "after inserted..."));
        ASSERTIONS_ONLY(bt.assert_it());
#endif
        if (bt._after_inserted)
          bt._after_inserted(bt, a);
        if (bt._after_changed)
          bt._after_changed(bt);
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
        if (auto_dot_png) {
          std::array<char, 200> name;
          std::snprintf(name.data(), name.size(), "auto.%s%04zu._insert.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
          std::ostringstream ttl;
          ttl << "insert '" << elem.str() << "' into " << orig.str();
          bt.dot(name.data(), ttl.str().c_str(), false);
        }
#endif
      }

      void _insert_node_for_even_tree(btreeT &bt, elem_ptr el, int index = -1) {
        int const fc = payload_count();
        if (fc >= MAX_PAYLOADS) {
          if (is_root()) {
            node_ptr p = new node();
            p->_pointers[0] = this->parent(p, true);
            if (p->_split_child_at(0, el)) {
              bt._root = p;
              return;
            }
            index = p->_first_of_insertion(el);
            p->_insert_node_if_ready(bt, el, index);
            bt._root = p;
            // p->fix_two_children(bt);
            return;
          }
        }

        _insert_node_if_ready(bt, el, index);
      }

      void _insert_node_if_ready(btreeT &bt, elem_ptr el, int index = -1) {
        if (index < 0) index = _first_of_insertion(el);

        if (is_leaf()) {
          for (auto t = Degree - 1; t > index; t--) _payloads[t] = _payloads[t - 1];
          _payloads[index] = el;
          _count++;
          return;
        }

        // advance f to f->child(index)
        node_ptr l = child(index);
        if (l == nullptr) {
          for (auto t = Degree - 1; t > index; t--) _payloads[t] = _payloads[t - 1];
          _payloads[index] = el;
          _count++;
          return;
        }
        auto lc = l->payload_count();
        if (lc == MAX_PAYLOADS) {
          // bool should_split{l->is_leaf()};
          // if (!should_split)
          //     if (!can_insert_directly(bt, el, index))
          //         should_split = true;
          // if (should_split) {
          if (_split_child_at(index, el))
            return;
          if (can_get_el(index) && is_less_than(get_el_ptr(index), el))
            index++, l = child(index);
          // }
        }
        l->_insert_node_if_ready(bt, el, -1);
        // post_insert_fix(index, l);
      }

      void fix_two_children(btreeT &bt) {
        assert(is_root());
        if (payload_count() == 1) {
          node_ptr l = child(0);
          node_ptr r = child(1);
          assert(r);
          auto lc = l->payload_count();
          auto rc = r->payload_count();
          if (lc < MID || rc < MID) {
            // merge l & r & parent as new root
            l->set_el(lc, get_el_ptr(0));
            l->inc_payload_count();
            for (auto t = lc + 1, ri = 0; t < MAX_PAYLOADS && ri < rc; t++, ri++) l->_payloads[t] = r->_payloads[ri], l->inc_payload_count();
            for (auto t = lc + 1, ri = 0; t < Degree + 1 && ri < rc + 1; t++, ri++) l->_pointers[t] = r->_pointers[ri];
            delete r->_reset_for_delete();
            bt._root = l->parent(nullptr, true);
            delete _reset_for_delete();
          }
        }
      }

      void post_insert_fix(int parent_idx, node_ptr l) {
        if (is_root()) return;
        auto c = payload_count();
        if (c < MID) {
          int lc = l->payload_count();
          if (lc > MID) {
            if (can_get_el(parent_idx)) {
              elem_ptr el = l->get_el_ptr(lc - 1);
              node_ptr el_right = l->child(lc);
              l->set_el(lc - 1, nullptr);
              l->dec_payload_count();
              for (auto t = Degree - 1; t > parent_idx; t--) _payloads[t] = _payloads[t - 1];
              for (auto t = Degree - 1; t > parent_idx; t--) _pointers[t] = _pointers[t - 1];
              _payloads[0] = el;
              _count++;
              node_ptr r = child(parent_idx + 1);
              int rc = r->payload_count();
              for (auto t = Degree - 1; t > 0; t--) r->_pointers[t] = r->_pointers[t - 1];
              for (auto t = Degree - 1; t > 0; t--) r->_payloads[t] = r->_payloads[t - 1];
              r->_pointers[0] = el_right;
              r->_payloads[0] = _payloads[_count - 1];
              _payloads[_count - 1] = r->_payloads[rc];
              r->_payloads[rc] = nullptr;
            } else {
              if (lc > MID) {
                _payloads[_count++] = l->get_el_ptr(lc - 1);
                l->set_el(lc - 1, nullptr);
                l->dec_payload_count();
              }
            }
          }
        }
      }

      int _first_of_insertion(const_elem_ptr el) {
        int index{0};
        while (/*index < Degree - 1 &&*/ can_get_el(index) && is_less_than(get_el_ptr(index), el))
          index++;
        return index;
      }

      static int _parent_pointers_idx(node const *res) {
        if (res && res->parent()) {
          for (auto t = Degree - 1; t >= 0; t--) {
            if (auto p = res->parent()->_pointers[t]; p == nullptr)
              continue;
            else if (p == res)
              return t;
          }
        }
        return -1;
      }

      // split 'left' child
      bool _split_child_at(int pos_of_child, elem_ptr el) {
        node_ptr left = this->child(pos_of_child);
        int ix = left->_first_of_insertion(el);
        int pos = (ix < MID) ? MID - 1 : MID;
        int fix = ix == MID ? 1 : 0;
        node_ptr nr = new node(left, pos - fix + 1, Degree - 1);
        elem_ptr mid = fix ? el : left->get_el_ptr(pos);
        dbg_debug("  split node %s, move mid '%s' up to parent %s", left->to_string().c_str(), elem_to_string(mid).c_str(), to_string().c_str());
        int lc = left->payload_count();
        for (auto t = pos; t < lc; t++) left->_payloads[t] = nullptr;
        for (auto t = pos - fix + 1; t <= lc; t++) left->_pointers[t] = nullptr;
        left->_count = (counter_type) pos;
        for (auto t = Degree - 1; t > pos_of_child; t--) this->_pointers[t] = this->_pointers[t - 1];
        for (auto t = Degree - 1; t > pos_of_child; t--) this->_payloads[t] = this->_payloads[t - 1];
        this->_payloads[pos_of_child] = mid;
        this->_pointers[pos_of_child] = left->parent(this, true);
        this->_pointers[pos_of_child + 1] = nr->parent(this, true);
        this->inc_payload_count();
        DEBUG_ONLY(root()->dbg_dump(root()->total_from_calculating(), std::cout, "after split..."));
        return ix == MID;
      }

      // static void split_root_if_necessary(btree &bt, node_ptr root, elem_ptr el) {
      //     if (root->parent() == nullptr) {
      //         auto pc = root->payload_count();
      //         bool should_split{};
      //         if (pc == Degree - 1)
      //             should_split = true;
      //         if (should_split) {
      //             auto np = new node();
      //             split(root, pc, np, 0, el);
      //             bt._root = np;
      //             DEBUG_ONLY(np->dbg_dump(bt, std::cout, "  for insertion, root has been split."));
      //             return;
      //         }
      //     }
      // }

      void _insert_node_for_odd_tree(node_ptr &root, elem_ptr el,
                                     node_ptr el_left_child = nullptr,
                                     node_ptr el_right_child = nullptr) {
        bool in_recursive = el_left_child || el_right_child;
        for (int i = 0; i < Degree - 1; i++) {
          auto *vp = _payloads[i];
          if (vp == nullptr) {
            if (in_recursive) {
              dbg_verbose_debug("  -> put...move '%s' up to parent node.", elem_to_string(el).c_str());
              _payloads[i] = el;
              _count++;
              if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
              if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
              return;
            }

            if (auto *child = _pointers[i]; child) {
              // has_child? forward a into it
              child->_insert_node_for_odd_tree(root, el);
              return;
            }

            _payloads[i] = el;
            _count++;
            return;
          }

          if (comparer()(*el, *vp)) {
            // less than this one in payloads[], insert it

            if (!in_recursive) {
              // has_child? forward a into it
              if (auto *child = _pointers[i]; child) {
                child->_insert_node_for_odd_tree(root, el);
                return;
              }
            }

            // recursively, or no child, insert at position `i`
            auto [has_slot, j] = _find_free_slot(i);
            if (has_slot) {
              for (int pos = j; pos > i; --pos) _payloads[pos] = _payloads[pos - 1];
              _payloads[i] = el;
              _count++;
              // if (!in_recursive) root->_size++;
              if (el_left_child || el_right_child) {
                dbg_verbose_debug("  -> splitting...move '%s' up to parent node.", elem_to_string(el).c_str());
                for (auto t = Degree; t >= i; t--) _pointers[t] = _pointers[t - 1];
                if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
                if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
              }
              return;
            }

#ifdef _DEBUG
            if (in_recursive)
              dbg_debug("    split this node again: %s + %s...", this->to_string().c_str(), elem_to_string(el).c_str());
            else
              dbg_debug("  split this node: %s + %s...", this->to_string().c_str(), elem_to_string(el).c_str());
#endif
            for (auto t = Degree - 1; t > i; t--) _payloads[t] = _payloads[t - 1];
            if (_pointers[0])
              for (auto t = Degree; t > i; t--) _pointers[t] = _pointers[t - 1];
            _payloads[i] = el;
            if (el_left_child) _pointers[i] = el_left_child->parent(this, true);
            if (el_right_child) _pointers[i + 1] = el_right_child->parent(this, true);
            _split_and_raise_up(root, i, el_left_child, el_right_child);
            return;
          }
        }

        if (!in_recursive) {
          // has_child? forward `a` into it
          if (auto *child = _pointers[Degree - 1]; child) {
            child->_insert_node_for_odd_tree(root, el);
            return;
          }
        }

        dbg_debug("  recursively: `%s` is greater than every one in node %s, create a new parent if necessary", elem_to_string(el).c_str(), to_string().c_str());
        _payloads[Degree - 1] = el;
        if (el_left_child) _pointers[Degree - 1] = el_left_child->parent(this, true);
        if (el_right_child) _pointers[Degree - 1 + 1] = el_right_child->parent(this, true);
        _split_and_raise_up(root, Degree - 1, el_left_child, el_right_child);
      }

      void _split_and_raise_up(node_ptr &root, int a_pos, node_ptr a_left, node_ptr a_right) {
        auto *new_parent_data = _payloads[MID];
        dbg_debug("    split and raise '%s' up.../ a_pos=%d / MID=%d, Degree=%d", elem_to_string(new_parent_data).c_str(), a_pos, MID, Degree);

        if (parent()) {
          node_ptr left = this;
          node_ptr right = new node(this, MID + 1, Degree);
          for (auto t = MID + 1; t <= Degree; t++) left->_pointers[t] = nullptr;
          for (auto t = MID; t < Degree; t++) left->_payloads[t] = nullptr;
          left->_count = MID;
          parent()->_insert_node_for_odd_tree(root, new_parent_data, left, right);
          if (a_pos <= MID) {
            if (a_left) left->_pointers[a_pos] = a_left->parent(left, true);
            if (a_right) left->_pointers[a_pos + 1] = a_right->parent(left, true);
          } else if (a_pos > MID) {
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
        left->_count = MID;
        new_parent_node->_pointers[0] = left;
        new_parent_node->_pointers[1] = right;

        root = new_parent_node;
      }


      bool can_insert_directly(btreeT &bt, const_elem_ptr el, int index_in_parent, int index = -1) {
        if (index < 0) index = this->_first_of_insertion(el);
        if (this->can_get_el(index) && is_equal_to(el, this->get_el_ptr(index))) {
          return this->payload_count() > MID;
        }

        node_ptr l = this->child(index);
        if (l) return l->can_insert_directly(bt, el, index);
        UNUSED(index_in_parent);
        return index < MAX_PAYLOADS;
      }

      bool can_erase_directly(btreeT &bt, const_elem_ptr removing, int index_in_parent, int index = -1) {
        if (index < 0) index = this->_first_of_insertion(removing);
        if (this->can_get_el(index) && is_equal_to(removing, this->get_el_ptr(index))) {
          return this->payload_count() > MID;
        }

        node_ptr l = this->child(index);
        if (l) return l->can_erase_directly(bt, removing, index);
        UNUSED(index_in_parent);
        return index > MID;
      }

      bool _remove_entry(btreeT &bt, const_node_ref res, int pos) {
        auto *removing = const_cast<node_ref>(res).get_el_ptr(pos);
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
        std::ostringstream orig, elem;
        orig << res.to_string();
        elem << elem_to_string(removing);
        dbg_debug("remove '%s' from %s (pos=%d) | MID=%d, M=%d, Degree=%d.", elem.str().c_str(), orig.str().c_str(), pos, MID, M, Degree);
#endif
        if (elem_ptr removed = this->_remove_from_root(bt, removing); removed) {
          bt._size--;
          delete removed;
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
          ASSERTIONS_ONLY(dbg_dump(bt, std::cout, "after removed..."));
          ASSERTIONS_ONLY(bt.assert_it());
#endif
          if (bt._after_removed)
            bt._after_removed(bt, removing);
          if (bt._after_changed)
            bt._after_changed(bt);
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
          if (auto_dot_png) {
            std::array<char, 200> name;
            std::snprintf(name.data(), name.size(), "auto.%s%04zu._remove.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
            std::ostringstream ttl;
            ttl << "remove '" << elem.str() << "' from " << orig.str();
            bt.dot(name.data(), ttl.str().c_str(), false);
          }
#endif
          return true;
        }
        return false;
      }

      bool _remove_entry(btreeT &bt, const_elem_ptr removing) {
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
        std::ostringstream orig, elem;
        orig << to_string();
        elem << elem_to_string(removing);
        dbg_debug("remove '%s' from %s | MID=%d, M=%d, Degree=%d.", elem.str().c_str(), orig.str().c_str(), MID, M, Degree);
#endif
        if (elem_ptr removed = this->_remove_from_root(bt, removing); removed) {
          bt._size--;
          delete removed;
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
          ASSERTIONS_ONLY(dbg_dump(bt, std::cout, "after removed..."));
          ASSERTIONS_ONLY(bt.assert_it());
#endif
          if (bt._after_removed)
            bt._after_removed(bt, removing);
          if (bt._after_changed)
            bt._after_changed(bt);
#if defined(HICC_ENABLE_PRECONDITION_CHECKS)
          if (auto_dot_png) {
            std::array<char, 200> name;
            std::snprintf(name.data(), name.size(), "auto.%s%04zu._remove.dot", bt._seq_prefix.c_str(), bt.dot_seq_inc());
            std::ostringstream ttl;
            ttl << "remove '" << elem.str() << "' from " << orig.str();
            bt.dot(name.data(), ttl.str().c_str(), false);
          }
#endif
          return true;
        }
        return false;
      }

      elem_ptr _remove_from_root(btreeT &bt, const_elem_ptr removing) {
        assert(this == bt._root);
        elem_ptr removed = this->_remove_it(removing);
        if (bt._root->payload_count() == 0) {
          if (!bt._root->is_leaf()) {
            node_ptr tmp = bt._root;
            dbg_debug("remove old root %s, and reduce the tree height.", tmp->to_string().c_str());
            bt._root = bt._root->child(0);
            delete tmp;
          }
        }
        return removed;
      }

      elem_ptr _remove_it(const_elem_ptr removing, int index = -1) {
        if (index < 0) index = this->_first_of_insertion(removing);

        if (this->can_get_el(index) && is_equal_to(removing, this->get_el_ptr(index))) {
          if (is_leaf())
            return _remove_leaf(removing, index);
          return _remove_internal_node(index);
        }

        if (is_leaf())
          return nullptr; // the 'removing' not found in this tree

        // The key to be removed is present in the sub-tree rooted with this node
        // The flag indicates whether the key is present in the sub-tree rooted
        // with the last child of this node
        bool const is_rightest{index == payload_count()};
        // If the child where the key is supposed to exist has less than M-1 _payloads,
        // we fill that child
        node_ptr at = child(index);
        if (at->payload_count() < M - 1)
          _fill(index);
        // If the last child has been merged, it must have merged with the previous
        // child and so we recurse on the (idx-1)th child. Else, we recurse on the
        // (idx)th child which now has atleast t _keys
        if (is_rightest && index > payload_count())
          return child(index - 1)->_remove_it(removing);
        return at->_remove_it(removing);
      }

      elem_ptr _remove_leaf(const_elem_ptr removing, int index = -1) {
        elem_ptr removed = get_el_ptr(index);
        UNUSED(removing);
        for (auto t = index; t < payload_count(); ++t) _payloads[t] = _payloads[t + 1];
        dec_payload_count();
        return removed;
      }

      elem_ptr _remove_internal_node(int index) {
        elem_ptr k = get_el_ptr(index);

        // If the child that precedes k (C[idx]) has atleast t _keys,
        // find the predecessor 'pred' of k in the subtree rooted at
        // C[idx]. Replace k by pred. Recursively delete pred
        // in C[idx]
        if (node_ptr c = child(index); c->payload_count() >= M) {
          position_t pos = _predecessor(index);
          elem_ptr el = pos.get();
          _payloads[index] = el;
          return c->_remove_it(el);
        }

        // If the child C[idx] has less that t _keys, examine C[idx+1].
        // If C[idx+1] has atleast t _keys, find the successor 'succ' of k in
        // the subtree rooted at C[idx+1]
        // Replace k by succ
        // Recursively delete succ in C[idx+1]
        if (node_ptr c = child(index + 1); c->payload_count() >= M) {
          position_t pos = _successor(index);
          elem_ptr el = pos.get();
          _payloads[index] = el;
          return c->_remove_it(el);
        }

        // If both C[idx] and C[idx+1] has less that t _keys,merge k and all of C[idx+1]
        // into C[idx]
        // Now C[idx] contains 2t-1 _keys
        // Free C[idx+1] and recursively delete k from C[idx]
        _merge(index);
        return child(index)->_remove_it(k);
      }

      void _fill(int index) {
        node_ptr mid = child(index);
        // If the previous child(C[idx-1]) has more than t-1 _keys, borrow a key
        // from that child
        if (index > 0 && child(index - 1)->payload_count() >= M)
          _rotate_from_left(index, mid, child(index - 1));
        // If the next child(C[idx+1]) has more than t-1 _keys, borrow a key
        // from that child
        else if (index < payload_count() && child(index + 1)->payload_count() >= M)
          _rotate_from_right(index, mid, child(index + 1));
        else {
          // Merge C[idx] with its sibling
          // If C[idx] is the last child, merge it with with its previous sibling
          // Otherwise merge it with its next sibling
          if (index != payload_count())
            _merge(index);
          else
            _merge(index - 1);
        }
      }

      void _rotate_from_left(int index, node_ptr mid, node_ptr sibling) {
        for (auto t = mid->payload_count() - 1; t >= 0; --t) mid->_payloads[t + 1] = mid->_payloads[t];
        if (!mid->is_leaf())
          for (auto t = mid->payload_count(); t >= 0; --t) mid->_pointers[t + 1] = mid->_pointers[t];
        mid->_payloads[0] = this->_payloads[index - 1];
        if (!mid->is_leaf())
          mid->_pointers[0] = sibling->_pointers[sibling->payload_count()];
        _payloads[index - 1] = sibling->_payloads[sibling->payload_count() - 1];
        mid->inc_payload_count();
        sibling->dec_payload_count();
      }

      void _rotate_from_right(int index, node_ptr mid, node_ptr sibling) {
        mid->_payloads[mid->payload_count()] = _payloads[index];
        if (!mid->is_leaf())
          mid->_pointers[mid->payload_count() + 1] = sibling->_pointers[0];
        _payloads[index] = sibling->_payloads[0];
        for (auto t = 1; t < sibling->payload_count(); t++) sibling->_payloads[t - 1] = sibling->_payloads[t];
        if (!sibling->is_leaf())
          for (auto t = 1; t <= sibling->payload_count(); t++) sibling->_pointers[t - 1] = sibling->_pointers[t];
        mid->inc_payload_count();
        sibling->dec_payload_count();
      }

      void _merge(int index) {
        node_ptr mid = child(index);
        node_ptr sibling = child(index + 1);
        mid->_payloads[M - 1] = _payloads[index];
        for (auto t = 0; t < sibling->payload_count(); t++) mid->_payloads[t + M] = sibling->_payloads[t];
        if (!mid->is_leaf())
          for (auto t = 0; t <= sibling->payload_count(); t++) mid->_pointers[t + M] = sibling->_pointers[t];
        for (auto t = index + 1; t < payload_count(); t++) _payloads[t - 1] = _payloads[t];
        for (auto t = index + 2; t <= payload_count(); t++) _pointers[t - 1] = _pointers[t];
        mid->_count += (counter_type) (sibling->payload_count() + 1);
        dec_payload_count();
        delete sibling->_reset_for_delete();
      }

#if NEVER_USED_CODES
      elem_ptr _remove_general_old(btree &bt, const_elem_ptr removing) {
        node_ptr next = this;
        if (int cnt = next->payload_count(); cnt == 1) {
          if (next->parent() != nullptr) { // // just one payload in 'this' node ?
            // If node[0] has two children and all payloads in its and 'this' payload
            // are lower than Degree-1, descend it.
            // NOTE that don't merge children on root node.
            node_ptr l = this->child(0), r = this->child(1);
            int lc = l->payload_count(), rc = r->payload_count();
            if (lc + rc + 1 <= Degree - 1) {
              l = _merge_two_child(bt, next, 0, l, r, lc, rc);
              if (l) {
                // remove the old root, and descend the btree height
                if (node_ptr p = next->parent(); p == bt._root)
                  bt._root = l->parent(p, true);
                else
                  l->parent(p, true);
                delete next->_reset_for_delete(); // delete 'next' node with ignoring the remnant objects
                next = l;
              }
            }
          }
        } // If not, advance the focus pointer along with the TR chain.
        // TR chain: we assume it is removing chained list which holds all parents of the target 'removing'.
        return next->_remove_a_payload(bt, removing);
      }

      elem_ptr _remove_a_payload(btree &bt, const_elem_ptr removing, int index = -1) {
        if (this->is_leaf()) {
          if (index < 0) index = this->_first_of_insertion(removing);
          if (this->can_get_el(index) && is_equal_to(removing, this->get_el_ptr(index))) {
            return this->_remove_payload_at(index);
          }
          // shouldn't fall through to this scene, or the target 'removing' cannot be found
          return nullptr;
        }
        return this->_remove_on_internal_node(bt, removing, index);
      }

      elem_ptr _remove_on_internal_node(btree &bt, const_elem_ptr removing, int index = -1) {
        if (index < 0) index = this->_first_of_insertion(removing);
        if (this->can_get_el(index) && is_equal_to(removing, this->get_el_ptr(index))) {
          node_ptr l = this->child(index);
          int lc = l->payload_count();

          if (lc > MID) {
            position_t pre = l->_predecessor();
            auto *replaced_with = pre.get();
            auto *replaced = this->get_el_ptr(index);
            this->set_el(index, replaced_with);
            l->_remove_a_payload(bt, replaced_with);
            return replaced;
          }

          node_ptr r = this->child(index + 1);
          int rc = r ? r->payload_count() : 0;
          if (rc > MID) {
            position_t next = r->_successor();
            auto *replaced_with = next.get();
            auto *replaced = this->get_el_ptr(index);
            this->set_el(index, replaced_with);
            r->_remove_a_payload(bt, replaced_with);
            return replaced;
          }

          if (r) {
            self_type::_merge_two_child(bt, this, index, l, r, lc, rc);
            return l->_remove_general(bt, removing);
          }

          // oops!
          // no right branch, delete the payload directly
          return this->_remove_payload_at(index);
        }

        // not found in this level, so entering into the next level
        node_ptr l = this->child(index);
        int lc = l->payload_count();
        bool critical_state{};
        if (IS_ODD)                   // Degree is odd, such as 5
          critical_state = lc == MID; // D=5,t=2*,t-1=1,MID=2
        else                          // else
          critical_state = lc == MID; // D=4.t=2,t-1=1,MID=1
        if (critical_state) {
          node_ptr r = this->child(index + 1);
          int rc = r ? r->payload_count() : 0;
          node_ptr p = this->child(index - 1);
          int pc = p ? p->payload_count() : 0;
          if (pc > MID)
            self_type::_rotate_to_right_child(this, index - 1, p, l, pc);
          else if (rc > MID)
            self_type::_rotate_to_left_child(this, index, l, r, lc);
          else if (!l->can_erase_directly(bt, removing, index)) {
            if (p)
              l = self_type::_merge_two_child(bt, this, index - 1, p, l, pc, lc);
            else
              self_type::_merge_two_child(bt, this, index, l, r, lc, rc);
          } else {
            return l->_remove_general(bt, removing); // for root node, just advance the focus ptr.
          }
        } // else lc > MID
        return l->_remove_a_payload(bt, removing);
      }

      elem_ptr _remove_payload_at(int index) {
        elem_ptr removing = this->_payloads[index];
        assert(removing && "the 'removing' shouldn't be null");
        for (auto t = index; t < Degree; t++) this->_payloads[t] = this->_payloads[t + 1];
        if (!this->is_leaf()) {
          for (auto t = index; t < Degree; t++) this->_pointers[t] = this->_pointers[t + 1];
          this->_pointers[Degree] = nullptr;
        }
        dec_payload_count();
        return removing;
      }

      static node_ptr _merge_two_child(btree &bt, node_ptr parent, int pos, node_ptr l = nullptr, node_ptr r = nullptr) {
        if (!l) l = parent->_pointers[pos];
        if (!r) r = parent->_pointers[pos + 1];
        assert(r);
        auto lc = l->payload_count();
        auto rc = r->payload_count();
        return _merge_two_child(bt, parent, pos, l, r, lc, rc);
      }

      static node_ptr _merge_two_child(btree &bt, node_ptr parent, int pos, node_ptr l, node_ptr r, int lc, int rc) {
        // allow Degree - 1 + 1 payloads temporarily here, and the merged parent[pos] will aalways be removed later
        // merge root && right into left, erase right, erase root, and re-root

        l->_payloads[lc] = parent->get_el_ptr(pos), l->inc_payload_count();

        const int fix = 0;

        for (int t = pos; t < Degree; t++) parent->_payloads[t] = parent->_payloads[t + 1];
        for (int t = pos + 1; t < Degree; t++) parent->_pointers[t] = parent->_pointers[t + 1];
        parent->_pointers[Degree] = nullptr;
        parent->dec_payload_count();
        if (parent->empty()) {
          l->parent(parent->parent(), true);
          delete parent->_reset_for_delete();
          if (l->parent() == nullptr)
            bt._root = l;
        }

        assert(r);
        for (int t = lc + 1, tr = 0; t < lc + 1 + rc; t++, tr++) l->_payloads[t] = r->get_el_ptr(tr), l->inc_payload_count();
        if (!r->is_leaf())
          for (int t = lc + 1 + fix, tr = 0; t < lc + 1 + rc + 1; t++, tr++) l->_pointers[t] = r->child(tr)->parent(l);
        delete r->_reset_for_delete(); // delete 'r' node with ignoring the remanent objects
        return l;
      }
#endif

#if NEVER_USED_CODES
      static node_ptr _merge_two_child_via_rotation(btree &bt, node_ptr parent, int pos, node_ptr l, node_ptr r, int lc, int rc) {
        // allow MAX_PAYLOADS+1 payloads temporarily here, and the merged parent[pos] will aalways be removed later
        // merge root && right into left, erase right, erase root, and re-root

        l->_payloads[lc] = parent->get_el_ptr(pos);

        const int fix = 0;

        for (int t = pos; t < Degree; t++) parent->_payloads[t] = parent->_payloads[t + 1];
        for (int t = pos + 1; t < Degree; t++) parent->_pointers[t] = parent->_pointers[t + 1];
        parent->_pointers[Degree] = nullptr;
        if (parent->empty()) {
          l->parent(parent->parent(), true);
          delete parent->_reset_for_delete();
          if (l->parent() == nullptr)
            bt._root = l;
        }

        assert(r);
        for (int t = lc + 1, tr = 0; t < lc + 1 + rc; t++, tr++) l->_payloads[t] = r->get_el_ptr(tr);
        if (!r->is_leaf())
          for (int t = lc + 1 + fix, tr = 0; t < lc + 1 + rc + 1; t++, tr++) l->_pointers[t] = r->child(tr)->parent(l);
        delete r->_reset_for_delete(); // delete 'r' node with ignoring the remanent objects
        return l;
      }

      static node_ptr _merge_two_child_if_verified(node_ptr parent, int pos, node_ptr l, node_ptr r, int lc, int rc) {
        if (lc + rc + 1 <= MAX_PAYLOADS + 1) { // allow MAX_PAYLOADS+1 payloads temporarily here, and the merged parent[pos] will aalways be removed later
          // merge root && right into left, erase right, erase root, and re-root
          return _merge_two_child(parent, pos, l, r, lc, rc);
        }
        // UNUSED(parent, pos, l, r);
        return nullptr;
      }
#endif

#if NEVER_USED_CODES
      // shift one key in 'left' sibling to 'right' one.
      static void _rotate_to_right_child(node_ptr parent, int pos, node_ptr l, node_ptr r, int lc = -1) {
        if (lc < 0) lc = l->payload_count();
        for (auto t = Degree - 1 - 1; t > 0; t--) r->_payloads[t] = r->_payloads[t - 1];
        r->_payloads[0] = parent->_payloads[pos];
        r->inc_payload_count();
        parent->_payloads[pos] = l->_payloads[lc - 1];
        if (l->_pointers[lc - 1]) {
          for (auto t = Degree - 1; t > 0; t--) r->_pointers[t] = r->_pointers[t - 1];
          int ix = l->_pointers[lc] ? lc : lc - 1;
          r->_pointers[0] = l->_pointers[ix]->parent(r, true);
          for (auto t = ix; t < Degree; t++) l->_pointers[t] = l->_pointers[t + 1];
        }
        l->_payloads[lc - 1] = nullptr;
        l->dec_payload_count();
      }

      // shift one key in 'right' sibling to 'left' one.
      static void _rotate_to_left_child(node_ptr parent, int pos, node_ptr l, node_ptr r, int lc = -1) {
        if (lc < 0) lc = l->payload_count();
        l->_payloads[lc] = parent->_payloads[pos];
        l->inc_payload_count();
        parent->_payloads[pos] = r->_payloads[0];
        if (r->_pointers[0]) {
          int ix = l->_pointers[lc] == nullptr ? lc : lc + 1;
          l->_pointers[ix] = r->_pointers[0]->parent(l, true);
          for (auto t = 0; t < Degree; t++) r->_pointers[t] = r->_pointers[t + 1];
        }
        for (auto t = 0; t < Degree - 1; t++) r->_payloads[t] = r->_payloads[t + 1];
        r->dec_payload_count();
      }
#endif

    private:
      // find the rightmost data payload under all children of 'parent'
      position_t _predecessor(int idx = 0, const_node_ptr parent = nullptr) const {
        const_node_ptr p = parent ? parent : this;
        p = p->child(idx);
        while (!p->is_leaf()) p = p->child(p->payload_count());
        return {p, p->payload_count() - 1};
        // position_t ptr{parent ? parent : this, idx};
        // while (!ptr->is_leaf()) ptr.advance_to_last_child();
        // return ptr.last_one();
      }
      position_t _successor(int idx, const_node_ptr parent = nullptr) const {
        const_node_ptr p = parent ? parent : this;
        p = p->child(idx + 1);
        while (!p->is_leaf()) p = p->child(0);
        return {p, 0};
        // position_t ptr{parent ? parent : this, 0};
        // while (!ptr->is_leaf()) ptr.advance_to_first_child();
        // return ptr.first_one();
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

      node &operator+=(node &rhs) { return this->operator+(rhs); }

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
      explicit operator bool() const { return !is_null(*this); }

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
        for (int i = 0, pi = 0; i < Degree - 1; i++, pi++) {
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
          abs_index = ctx.abs_index;
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
            abs_index = ctx.abs_index;
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

      static bool _visit_payloads(const_node_ref ref, traversal_context &ctx, visitor_l const &visitor) {
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

    private:
      std::tuple<bool, int> _find_free_slot(int start_index) const {
        bool has_slot = false;
        int j = start_index + 1;
        for (; j < Degree - 1; ++j) {
          auto *vpt = _payloads[j];
          if (vpt == nullptr) {
            has_slot = true;
            break;
          }
        }
        return {has_slot, j};
      }

      static bool is_less_than(const_elem_ref lhs, const_elem_ref rhs) { return node::comparer()(lhs, rhs); }
      static bool is_less_than(const_elem_ptr lhs, const_elem_ptr rhs) { return node::comparer()(*lhs, *rhs); }

      static bool is_equal_to(const_elem_ref a, const_elem_ref b) { return !is_less_than(a, b) && !is_less_than(b, a); }
      static bool is_equal_to(const_elem_ptr a, const_elem_ptr b) { return !is_less_than(a, b) && !is_less_than(b, a); }

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
          if (auto *d = _payloads[i]; d) {
            if (i > 0) os << ',';
            os << (*d);
          } else
            break;
        }
        os << ']';
        return os.str();
      }

      std::string c_str() const { return to_string(); }

      int degree() const { return Degree; }

      int level() const {
        int l{};
        node_ptr p = this;
        while (p->_parent)
          p = p->_parent, l++;
        return l;
      }


      bool can_get_el(int index) const {
        if (index < 0 || index > Degree - 1) return false;
        return _payloads[index] != nullptr;
      }
      bool can_get_child(int index) const {
        if (index < 0 || index > Degree) return false;
        return _pointers[index] != nullptr;
      }

      const_elem_ref get_el(int index = 0) const { return *const_cast<node_ptr>(this)->get_el_ptr(index); }
      elem_ref get_el(int index = 0) { return *get_el_ptr(index); }
      elem_ptr get_el_ptr(int index = 0) {
        return (index < 0 || index > Degree - 1) ? &_null_elem()
                                                 : (_payloads[index] ? (_payloads[index]) : &_null_elem());
      }
      const_elem_ptr get_el_ptr(int index = 0) const { return const_cast<node_ptr>(this)->get_el_ptr(index); }

      void set_el(int index, elem_ptr a) { _payloads[index] = a; }

      const_node_ptr child(int i) const { return (i >= 0 && i <= Degree) ? _pointers[i] : nullptr; }
      node_ptr child(int i) { return (i >= 0 && i <= Degree) ? _pointers[i] : nullptr; }

#if NEVER_USED_CODES
      int _last_payload_index() const {
        if (_payloads[0] != nullptr) {
          for (auto i = 1; i < Degree; ++i) {
            if (_payloads[i] == nullptr)
              return (i - 1);
          }
        }
        return -1;
      }

      bool has_children_heavy() const {
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
        if (auto const *p = res->parent(); p) {
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
        if (auto const *p = res->parent(); p) {
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
        if (auto const *p = res->parent(); p) {
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
        if (auto const *p = res->parent(); p) {
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

      static bool is_rightest(const_node_ptr parent, int parent_idx) {
        return (parent->_pointers[parent_idx] && parent->_pointers[parent_idx] == nullptr);
      }

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
#endif
#if 0
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
                    if (auto *p = ptr->parent(); p != nullptr) {
                        if (auto pi = _parent_pointers_idx(ptr); pi >= 0) {
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
#endif
      static lite_position next_payload(node const *res, int index, elem_ptr removing) {
        assert(index >= -1 && index < Degree);
        for (auto t = index + 1; t < Degree; t++) {
          if (auto *p = res->_pointers[t]; p != nullptr)
            return next_payload(p, -1, removing);

          if (auto *p = res->_payloads[t]; p != nullptr)
            return {t, *res};
        }

        if (auto *p = res->parent(); p != nullptr) {
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

      int payload_count() const { return _count; }
      int inc_payload_count() { return ++_count; }
      int dec_payload_count() { return --_count; }

      int dyn_payload_count() const {
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

      /**
             * @brief returns the height from root to this node. for root node, depth is 0.
             * @return the height
             */
      int depth() const {
        int i = 0;
        auto *p = this;
        while ((p = p->_parent) != nullptr) i++;
        return i;
      }

      bool empty() const { return _payloads[0] == nullptr; }
      bool is_leaf() const { return _pointers[0] == nullptr; }

    public:
      void dbg_dump(btreeT const &bt, std::ostream &os = std::cout, const char *headline = nullptr) const {
        bt._root->dbg_dump(bt._size, os, headline);
      }
      void dbg_dump(size_type known_size, std::ostream &os = std::cout, const char *headline = nullptr) const {
        if (headline) os << headline;

        int count = total_from_calculating();

        walk_level_traverse([known_size, count, &os](traversal_context const &ctx) -> bool {
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
              os << '/' << known_size;
              os << "/dyn:" << count;
              assertm(ctx.curr.parent() == nullptr, "root's parent must be nullptr");
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
        assert(known_size == (size_type) count);
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
    void insert(Args &&...args) { (_root->insert_one(*this, args), ...); }

    void insert(elem_type *a_new_data_ptr) {
      assert(a_new_data_ptr);
      _root->insert_one(*this, a_new_data_ptr);
    }

    template<class A, typename... Args,
             std::enable_if_t<
                 std::is_constructible<elem_type, A, Args...>::value && !std::is_same<std::decay_t<A>, node>::value,
                 int> = 0>
    void emplace(A &&a0, Args &&...args) {
      _root->emplace(*this, std::forward<A>(a0), std::forward<Args>(args)...);
    }

    template<class A,
             std::enable_if_t<
                 std::is_constructible<elem_type, A>::value && !std::is_same<std::decay_t<A>, node>::value,
                 int> = 0>
    void emplace(A &&a) {
      _root->emplace(*this, std::forward<A>(a));
    }

    template<typename... Args>
    void remove(Args &&...args) { (_root->remove(*this, args), ...); }
    void remove(elem_type *el) { _root->remove(*this, el); }

    void remove_by_position(node const &res, int index) { _root->remove_by_position(*this, res, index); }
    void remove_by_position(position pos) {
      if (std::get<0>(pos))
        _root->remove_by_position(*this, std::get<1>(pos), std::get<2>(pos));
    }

    //

    bool exists(elem_type const &data) const { return _root->exists(data); }

    //

    position find(elem_type const &data) const { return _root->find(data); }
    position find_by_index(int index) const { return _root->find_by_index(index); }
    const_node_ref find(std::function<bool(const_elem_ptr, const_node_ptr, int /*level*/, bool /*node_changed*/,
                                           int /*ptr_index*/)> const &matcher) const { return find(matcher); }


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

#if 0
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
#endif

    node_ptr _reset_for_delete() { return _root->_reset_for_delete(); }
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

      int height = -1;
      walk([this, &height](traversal_context const &ctx) -> bool {
        int i;

        for (i = 0; i < Degree; i++) {
          const_node_ptr vp = ctx.curr._pointers[i];
          if (vp) {
            if (vp->parent() != &ctx.curr) {
              std::ostringstream os, os1;
              auto *pp = vp->parent();
              if (pp) os1 << '(' << pp->to_string() << ')';
              os << "node " << vp->to_string() << " has parent " << pp << '(' << os1.str()
                 << "). but not equal to " << &ctx.curr << " (" << ctx.curr.to_string() << ").\n";
              dbg_dump(os);
              assertm(false, os.str());
            }
            assertm(vp->dyn_payload_count() == vp->payload_count(), "counter is wrong");
          }
        }

        if (ctx.curr.is_leaf()) {
          if (height < 0) height = ctx.curr.depth();
          else if (height != ctx.curr.depth()) {
            std::ostringstream os, os1;
            os1 << '(' << ctx.curr.to_string() << ')';
            os << "for the node " << os1.str() << ", its depth (" << ctx.curr.depth() << ") isn't equal to the known tree height: " << height << ". we're expecting all leaf nodes have the same height/depth.";
            assertm(height == ctx.curr.depth(), os.str());
          }
        }

        const_elem_ptr last_el{nullptr};
        for (i = 0; i < Degree; i++) {
          if (auto const *vp = ctx.curr._pointers[i]; !vp)
            break;

          if (last_el && ctx.curr.can_get_el(i)) {
            if (!(*last_el <= ctx.curr.get_el(i))) {
              std::ostringstream os, os1;
              os1 << '(' << ctx.curr.to_string() << ')';
              os << "in node " << os1.str() << ", keys[" << (i - 1) << "] must be less than keys[" << i << "].";
              assertm(*last_el <= ctx.curr.get_el(i), os.str());
            }
          }
          last_el = ctx.curr.get_el_ptr(i);
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
        if (ctx.curr.parent() != nullptr && i < MID) {
          dbg_dump();
          std::ostringstream os;
          os << "payloads count (" << i << ") must be greater than MID-1 (i.e. Degree DIV 2). MID = " << MID << ", Degree = " << Degree << ".";
          assertm(i >= MID, os.str().c_str());
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
            } else
              break;
          }
        }

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
                std::cout << filename << " was written." << '\n';
                std::array<char, 512> cmd;
                std::snprintf(cmd.data(), cmd.size(), "dot '%s' -T png -o '%s.png' %s", filename, filename, verbose ? "-v" : "");
                std::cout << "executing: " << std::quoted(cmd.data()) << '\n';
                // process::exec dot("dot aa.dot -T png -o aa.png");
                process::exec const dot(cmd.data());
                std::cout << dot.rdbuf();
                std::cout.flush();
                std::cout << "executed: " << '\n';
              });

      btreeT::size_type total = size();
      walk_level_traverse([total, title, &ofs](btreeT::traversal_context const &ctx) -> bool {
        if (ctx.abs_index == 0) {
          *ofs << "graph {" << '\n';
          if (title && *title)
            *ofs << "labelloc=\"t\";\n"
                 << "label=\"" << title << "\";" << '\n';
        }
        if (ctx.node_changed) {
          auto from = ctx.curr.to_string();
          if (ctx.abs_index == 0) {
            *ofs << std::quoted(from) << " [color=green];" << '\n';
          }
          for (int i = 0; i < ctx.curr.degree(); ++i) {
            if (const_node_ptr child = ctx.curr.child(i); child) {
              *ofs << std::quoted(from) << " -- ";
              *ofs << std::quoted(child->to_string()) << ';'
                   << ' ' << '#' << ' ' << std::boolalpha
                   << ctx.abs_index << ':' << ' '
                   << ctx.level << ',' << ctx.index << ',' << ctx.parent_ptr_index << ','
                   << ctx.loop_base << ','
                   << ctx.level_changed << ','
                   << ctx.parent_ptr_index_changed
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
    btreeT &dot_seq(size_type seq_no) {
      _seq_no = seq_no;
      return *this;
    }
    btreeT &dot_prefix(const char *s) {
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

    std::function<void(btreeT &, const_elem_ptr a)> _after_inserted;
    std::function<void(btreeT &, const_elem_ptr a)> _after_removed;
    std::function<void(btreeT &)> _after_changed;

    size_type _seq_no{};
    std::string _seq_prefix{};
  };

  template<class T, int Degree, class Comp, bool B>
  inline btreeT<T, Degree, Comp, B>::btreeT()
      : _root(new node{}) {
  }

  template<class T, int Degree, class Comp, bool B>
  inline btreeT<T, Degree, Comp, B>::~btreeT() {
    _clear_all();
  }

  // template<class T, int Degree, class Comp>
  // inline typename btree<T, Degree, Comp>::size_type btree<T, Degree, Comp>::node::_size{};

} // namespace hicc::btree


template<class T, int Degree = 5>
void dbg_dump(std::ostream &os, hicc::btree::btreeT<T, Degree, std::less<T>, use_auto_dot> const &bt) {
  os << "DUMP...";
  bt.walk_level_traverse([&os](typename hicc::btree::btreeT<T, Degree, std::less<T>, use_auto_dot>::traversal_context const &ctx) -> bool {
    if (ctx.node_changed) {
      if (ctx.index == 0)
        os << '\n'
           << ' ' << ' '; //  << 'L' << level << '.' << ptr_index;
      else
        os << ' ';
      // if (node.parent() == nullptr) os << '*'; // root here
      os //<< ptr_index << '.'
          << ctx.curr.to_string();
    }
    os.flush();
    return true; // false to terminate the walking
  });
  os << '\n';
}

void test_btree() {

  auto source = {9, 11, 2, 7, 3, 5, 13, 17, 19, -19, 19, 21, 6, 8, 12, 29, 31, 18, 20, 16, 14, 15, -15, 15, 1, -1, 1, 10, 33, 47, 28, 55, 56, 66, 78, 88, 89, 69, -69, 69};

  hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
    std::cout << "test_btree_even<4>: It took " << duration << '\n';
    return false;
  });

  using btree = hicc::btree::btreeT<int, 5, std::less<int>, use_auto_dot && use_auto_dot_for_case_1>;
  btree bt;
  bt.dot_prefix("tree");
  for (auto const &v : source) {
    if (v > 0) {
      bt.insert(v);
    } else {
      bt.remove(-v);
    }
    NO_ASSERTIONS_ONLY(bt.dbg_dump());
  }
}

void test_btree_even() {

  auto source = {9, 11, 2, 7, 3, 5, 13, 17, 19, -19, 19, 21, 6, 8, 12, 29, 31, 18, 20, 16, 14, 15, -15, 15, 1, -1, 1, 10, 33, 47, 28, 55, 56, 66, 78, 88, 89, 69, -69, 69};

  hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
    std::cout << "test_btree_even<4>: It took " << duration << '\n';
    return false;
  });

  using btree = hicc::btree::btreeT<int, 4, std::less<int>, use_auto_dot && use_auto_dot_for_case_1>;
  btree bt;
  bt.dot_prefix("tree");
  for (auto const &v : source) {
    if (v > 0) {
      bt.insert(v);
    } else {
      if (v == -1)
        std::cout << v << '\n';
      bt.remove(-v);
    }
    NO_ASSERTIONS_ONLY(bt.dbg_dump());
  }
}

void test_btree_1_emplace() {
  std::string str(5, 's');
  hicc::btree::btreeT<std::string> bts;
  bts.emplace(5, 's');
}

hicc::btree::btreeT<int, 5, std::less<int>, use_auto_dot> test_btree_1() {
  using hicc::terminal::colors::colorize;
  // colorize c;
  // std::cout << c.bold().s("test_btree_1") << '\n';
  std::cout << "test_btree_1" << '\n';
  hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
    std::cout << "It took " << duration << '\n';
    return false;
  });

  using btree = hicc::btree::btreeT<int, 5, std::less<int>, use_auto_dot>;
  btree bt;
  bt.dot_prefix("tr01");
  bt.insert(39, 22, 97, 41);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(53);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(13, 21, 40);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(30, 27, 33);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(36, 35, 34);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(24, 29);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(26);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(17, 28, 23);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(31, 32);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());

  std::cout << '\n';
  int count = 0;
  bt.walk([&count](btree::traversal_context const &ctx) -> bool {
#ifdef _DEBUG
    static int last_one = 0;
    assertm(last_one <= *ctx.el, "expecting a ascend sequences");
    last_one = *ctx.el;
#endif
    std::cout << ctx.el << ',';
    count++;
    return true;
  });
  std::cout << '\n';
  std::cout << "total: " << count << " elements" << '\n';

  return bt;
}

void test_btree_1_remove(hicc::btree::btreeT<int, 5, std::less<int>, use_auto_dot> &bt) {
  if (auto [idx, ref] = bt.next_payload(bt.find(21)); ref) {
    assert(ref[idx] == 22);
  } else {
    assert(false && "'21' not found");
  }
#if 0
    if (auto [idx, ref] = bt.next_minimal_payload(bt.find(32)); ref) {
        // std::cout << ref[idx] << '\n';
        assert(ref[idx] == 33);
    } else {
        assert(false && "'32' not found");
    }
#endif

#if 0
    auto vec = bt.to_vector();

    for (auto it = vec.begin(); it != vec.end(); it++) {
        auto val = (*it);
        if (auto [ok, rr, ii] = bt.find(val); ok) {
            if (auto [idx, ref] = bt.next_minimal_payload(rr, ii); ref) {
                std::cout << ref[idx] << " >> " << val << '\n';

                auto tmp = it;
                tmp++;
#ifdef _DEBUG
                auto next_val = (*tmp);
                assert(ref[idx] == next_val);
                UNUSED(next_val);
#endif
            } else if (val != vec.back()) {
                std::ostringstream msg;
                msg << '"' << val << '"' << " next_minimal_payload failed.";
                assertm(false, msg.str().c_str());
            }
        } else {
            std::ostringstream msg;
            msg << '"' << val << '"' << " cannot be found.";
            assertm(false, msg.str().c_str());
        }
    }
#endif

  bt.remove(21);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(17);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(22);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(13);
  bt.remove(23);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(24);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(26);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(27);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.remove(28);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());

#if 0
    for (auto v : vec) {
        if (v == 97) {
            // bt.dbg_dump();
            v += 0;
        }
        bt.remove(v);
        NO_ASSERTIONS_ONLY(bt.dbg_dump());
    }
#endif
}

void test_btree_2() {
  using hicc::terminal::colors::colorize;
  colorize c;
  std::cout << c.bold().s("test_btree_2") << '\n';

  hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
    std::cout << "It took " << duration << '\n';
    return false;
  });

  using btree = hicc::btree::btreeT<int, 3, std::less<int>, true>;
  btree bt;
  bt.dot_prefix("test-02");
  // bt.insert(50, 128, 168, 140, 145, 270, 250, 120, 105, 117, 264, 269, 320, 439, 300, 290, 226, 155, 100, 48, 79);
  bt.insert(50, 128, 168, 140, 145, 270);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(250);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());

  // NO_ASSERTIONS_ONLY(bt.dbg_dump());
  // // dbg_dump<int, 3>(std::cout, bt);
  // NO_ASSERTIONS_ONLY(bt.dbg_dump());
  // bt.insert(250);
  // NO_ASSERTIONS_ONLY(bt.dbg_dump());

  bt.insert(120, 105, 117, 264);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(269, 320, 439, 300);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(290, 226, 155, 100);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(48);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());
  bt.insert(79);
  NO_ASSERTIONS_ONLY(bt.dbg_dump());

  // bt.dot("bb2.1.dot");

  // bt.remove(79);
  // NO_ASSERTIONS_ONLY(bt.dbg_dump());
  // //bt.remove(250);
  // //NO_ASSERTIONS_ONLY(bt.dbg_dump());
  // // bt.dot("bb2.2.dot");

  auto l = [&bt](int k) {
    if (auto [ok, ref, idx] = bt.find(k); ok) {
      std::cout << "searching '" << k << "' found: " << ref.to_string() << ", idx = " << idx << '\n';
    } else {
      std::cerr << "searching '" << k << "': not found\n";
    }
  };
  l(270);
  l(226);
  l(100);

  for (auto &v : bt.to_vector()) {
    std::cout << v << ',';
  }
  std::cout << '\n';

  bt.walk([](btree::traversal_context const &ctx) -> bool {
    std::cout << std::boolalpha << *ctx.el << "/L" << ctx.level << "/IDX:" << ctx.abs_index << '/' << ctx.index
              << "/PI:" << ctx.parent_ptr_index << "/" << ctx.parent_ptr_index_changed << "/" << ctx.loop_base << '\n';
    return true;
  });
  std::cout << '\n';
  std::cout << "NLR:" << '\n';
  bt.walk_nlr([](btree::traversal_context const &ctx) -> bool {
    std::cout << std::boolalpha << *ctx.el << "/L" << ctx.level << "/IDX:" << ctx.abs_index << '/' << ctx.index
              << "/PI:" << ctx.parent_ptr_index << "/" << ctx.parent_ptr_index_changed << "/" << ctx.loop_base << '\n';
    return true;
  });

  auto l2 = [&bt](int index) {
    if (auto [ok, ref, idx] = bt.find_by_index(index); ok) {
      std::cout << "searching index '" << index << "' found: " << ref.to_string() << ", idx = " << idx << '\n';
    } else {
      std::cerr << "searching index '" << index << "': not found\n";
    }
  };
  l2(0);
  l2(1);
  l2(2);
  l2(3);
}

void test_btree_b() {
  hicc::chrono::high_res_duration hrd;

  int ix{};
  using btree = hicc::btree::btreeT<int, 4, std::less<int>, false>;
  btree bt;
  bt.dot_prefix("tr.b");
  // auto numbers = {12488, 2222, 26112, -1, -1, 13291, 23251, 32042, 19569};
  // auto numbers = {1607, 4038, 1100, 2937, 3657, 10151};
  // auto numbers = {15345, 5992, 21742, 3387, 9020, 18896, 27523, 28967, -1};
  // auto numbers = {21140, 16213, 3732, 27322, 27594, -2, -1, -1, 7347, -1, 5134, -1, 19740,28912, 17131};
  // auto numbers = {7789, 20579, 1622, 153, 1449, 12733, 9056, 11643, 25126, 18723, 17262, 23071, 31250, 31575, -10};
  // [7789,20579]/11
  //              [1622]/9 [12733]/0 [25126]/0
  //              [153,1449]/17 [5564,8669]/0 | [9056,11643]/0 [18723]/0 | [17262,23071]/0 [31350,31575]/0
  // auto numbers = {9004, 13782, 3060, 11383, 15034, 15737, -1};
  // [15315]/0
  //         [14925]/9 [29854,31496]/0
#if 0
    auto numbers = {
            -1,
            -1,
            -1,
            -1,
            -1,
            25356,
            -2,
            29923,
            -1,
            7611,
            -1,
            28963,
            3610,
            -4,
            1825,
            -5,
            12286,
            -3,
            25236,
            5144,
            14000,
            -8,
            -4,
    };
    auto numbers = {
            19220,
            -2,
            -2,
            -2,
            24999,
            30209,
            -4,
            -4,
            15934,
            8618,
            -4,
            360,
            -4,
    };
#endif
  auto numbers = {
      // 4440,14556,-1,8514,20332,16408,-3,24296,-3,15175,22193,13697,20236,-4,29256,11686,2896,32147,11373,13255,-2,
      // 30771,26685,20943,17678,-3,-2,4221,31252,29131,3195,4062,-1,31907,10329,21963,8360,12800,18882,-2,3157,18342,-9,-11,18055,-5,3959,21628,-11,27577,942,-1,-2,21983,5437,28586,26237,23079,30784,9932,-16,29678,7680,15816,955,-6,28249,31202,-13,
      17248,
      16636,
      30956,
      23976,
      18576,
      11060,
      22035,
      14448,
      28732,
      17548,
      -4,
      -5,
      -8,
      -1,
      14715,
      29947,
      3582,
      3080,
      12961,
      -4,
      24900,
      6814,
      -4,
      24086,
      21141,
      5905,
      32365,
      13570,
      11597,
      15846,
      6851,
      26260,
      31117,
      2616,
      23987,
      -16,
      -16,
      27614,
      13368,
      29770,
      12809,
      20397,
      32017,
      18304,
      -8,
      28557,
      9308,
      31800,
      8588,
      11801,
      15623,
      23233,
      -29,
      11534,
      -4,
      1622,
      1553,
      28163,
      23118,
      19468,
      -22,
      -12,
      -32,
      6064,
      20133,
      -12,
      16583,
      -12,
      26075,
      15757,
      -11,
      -12,
      21571,
      14138,
      -12,
      17863,
      10946,
      -28,
      -35,
      11968,
      -33,
      14295,
      29345,
      15662,
      28329,
      -23,
      3820,
      21112,
      1804,
      2771,
      -23,
      27808,
      5651,
      18434,
      23125,
      -45,
      22159,
      15528,
      23750,
      19367,
      -31,
      5796,
      -46,
      9343,
      26523,
      -44,
      -33,
      22595,
      -3,
      -5,
      12701,
      32581,
      13332,
      10288,
      22860,
      13970,
      14102,
  };

  for (auto n : numbers) {
    dbg_debug("step %d: n = %d", ix, n);
    if (n > 0) {
      if (ix == 116 || ix == 29)
        ix += 0;
      bt.insert(n);
      // bt.dot("00.last.dot", "");
      // bt.dbg_dump();
    } else {
      auto index = abs(1 + n);
      auto pos = bt.find_by_index(index);
      if (ix == 117 || ix == 22 || ix == 11)
        ix += 0;
      bt.remove_by_position(pos);
      NO_ASSERTIONS_ONLY(bt.dbg_dump());
    }
    ix++;
  }
}

void test_btree_rand() {
  hicc::chrono::high_res_duration hrd;

  std::cout << '\n'
            << "random test..." << '\n'
            << '\n';

  const int MAX = 32768;
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, MAX);

  std::srand((unsigned int) std::time(nullptr)); // use current time as seed for random generator

  using btree = hicc::btree::btreeT<int, 4, std::less<int>, false>;
  std::vector<int> series;
  btree bt;
  bt.dot_prefix("rand");
  for (int ix = 0; ix < 2000 * 1000; ix++) {
    // int choice = std::rand();
    int choice = uniform_dist(e1);
    if (choice > MAX / 3) {
      // int random_variable = std::rand();
      int random_variable = uniform_dist(e1);
      series.push_back(random_variable);
      for (auto vv : series) std::cout << vv << ',';
      std::cout << '\n'
                << "cnt: " << series.size() << '\n';
      bt.insert(random_variable);
    } else {
      // int index1 = std::rand();
      // auto index = (int) (((double) index1) / RAND_MAX * bt.size());
      std::uniform_int_distribution<int> uniform_dist_s(0, (int) bt.size());
      int index = uniform_dist_s(e1);
      auto pos = bt.find_by_index(index);
      if (std::get<1>(pos)) {
        series.push_back(-1 - index);
        for (auto vv : series) std::cout << vv << ',';
        std::cout << '\n'
                  << "cnt: " << series.size() << '\n';
        bt.remove_by_position(pos);
      }
    }

    // bt.dbg_dump();
    std::cout << '.';
    if (ix % 1000 == 0)
      std::cout << '\n'
                << '\n'
                << ix << ':' << '\n';
  }
  std::cout << '\n';

  NO_ASSERTIONS_ONLY(bt.dbg_dump(std::cout));
  // dbg_dump<int, 3>(std::cout, bt);
}

template<class T, class _D = std::default_delete<T>>
class dxit {
public:
  dxit() {}
  ~dxit() {}

private:
};


// Driver program to test above functions
int main() {
#if 0
    test_base(2,
              {1, 3, 7, 10, 11, 13, 14, 15, 18, 16, 19, 24, 25, 26, 21, 4, 5, 20, 22, 2, 17, 12, 6},
              {6, 13, 7, 4, 2, 16});

    test_base(3,
              {1, 3, 7, 10, 11, 13, 14, 15, 18, 16, 19, 24, 25, 26, 21, 4, 5, 20, 22, 2, 17, 12, 6},
              {6, 13, 7, 4, 2, 16});

    test_base(4,
              {1, 3, 7, 10, 11, 13, 14, 15, 18, 16, 19, 24, 25, 26, 21, 4, 5, 20, 22, 2, 17, 12, 6},
              {6, 13, 7, 4, 2, 16});

    test_base(5,
              {1, 3, 7, 10, 11, 13, 14, 15, 18, 16, 19, 24, 25, 26, 21, 4, 5, 20, 22, 2, 17, 12, 6},
              {6, 13, 7, 4, 2, 16});
#endif

  // test_gg(2, {9, 11, 2, 7, 3, 5, 13, 17, 19, -19, 19, 21, 6, 8, 12, 29, 31, 18, 20, 16, 14, 15, -15, 15, 1, -1, 1, 10, 33, 47, 28, 55, 56, 66, 78, 88, 89, 69, -69, 69});

#if 0

#if 1
    // assertm(1 == 0, "bad");
    test_btree();

    auto bt = test_btree_1();
#if 0
    if (false) {
        using btree = decltype(bt);
        btree::size_type total = bt.size();
        {
            // auto ofs = std::make_unique<std::ofstream>("aa.dot", std::ofstream::out);
            std::unique_ptr<std::ofstream, std::function<void(std::ofstream *)>> ofs(new std::ofstream("aa.dot", std::ofstream::out), [](std::ofstream *) {
                std::cout << "aa.dot pre-closed.\n";
            });
            *ofs.get() << "1";
        }
        std::ofstream ofs("aa.dot", std::ofstream::out);
        hicc::util::defer ofs_closer(ofs, []() {
            std::cout << "aa.dot was written." << '\n';
            hicc::process::exec dot("dot aa.dot -T png -o aa.png");
            std::cout << dot.rdbuf();
        });
        hicc::util::defer<bool> a_closer([&total]() { total = 0; });
        bt.walk_level_traverse([total, &ofs](btree::traversal_context const &ctx) -> bool {
            if (ctx.abs_index == 0) {
                ofs << "graph {" << '\n';
            }
            if (ctx.node_changed) {
                auto from = std::quoted(ctx.curr.to_string());
                if (ctx.abs_index == 0) {
                    ofs << from << " [color=green];" << '\n';
                }
                for (int i = 0; i < ctx.curr.degree(); ++i) {
                    if (auto const *child = ctx.curr.child(i); child) {
                        ofs << from << " -- ";
                        ofs << std::quoted(child->to_string()) << ';'
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
                ofs << '\n';
            }
            if (ctx.abs_index == (int) total - 1) {
                ofs << "}" << '\n';
            }
            // std::cout << "abs_idx: " << ctx.abs_index << ", total: " << total << '\n';
            return true;
        });
    }
#endif
    // bt.dot("aa.dot");
    test_btree_1_remove(bt);
#endif

    test_btree_2();

#elif 1
  // test_btree();
  test_btree_even();
  // test_btree_b();
#else
  test_btree_rand();
#endif

  return 0;
}