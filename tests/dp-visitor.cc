//
// Created by Hedzr Yeh on 2021/9/13.
//

#include <math.h>

#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "hicc/hz-dbg.hh"
#include "hicc/hz-defs.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-string.hh"
#include "hicc/hz-util.hh"
#include "hicc/hz-x-test.hh"

// a simple drawcli demo
// https://github.com/Microsoft/VCSamples/blob/master/VC2010Samples/MFC/ole/drawcli/drawobj.h
namespace hicc::dp::visitor::basic {

  using draw_id = std::size_t;

  /** @brief a shape such as a dot, a line, a rectangle, and so on. */
  struct drawable_base {
    virtual ~drawable_base() = default;
    drawable_base() = default;
    drawable_base(draw_id id) : _id(id) {}
    draw_id id() const { return _id; }
    void id(draw_id id_) { _id = id_; }

  private:
    draw_id _id;
  };

  template<typename Papa>
  struct drawable : public drawable_base {
    ~drawable() override = default;
    using base_type = drawable<Papa>;
    drawable() = default;
    using drawable_base::drawable_base;
    Papa &This() { return static_cast<Papa &>(*this); }
    Papa const &This() const { return static_cast<Papa const &>(*this); }
    friend std::ostream &operator<<(std::ostream &os, Papa const &o) {
      return os << '<' << o.type_name() << '#' << o.id() << '>';
    }
    constexpr std::string_view type_name() const {
      return hicc::debug::type_name<Papa>();
    }

  protected:
    virtual std::ostream &write(std::ostream &os) = 0;
  };

#define MAKE_DRAWABLE(T)      \
  using base_type::base_type; \
  T() = default;              \
  ~T() override = default;    \
  std::ostream &write(std::ostream &os) override { return os; }

  //@formatter:off
  struct point : public drawable<point> {
    MAKE_DRAWABLE(point)
  };
  struct line : public drawable<line> {
    MAKE_DRAWABLE(line)
  };
  struct rect : public drawable<rect> {
    MAKE_DRAWABLE(rect)
  };
  struct ellipse : public drawable<ellipse> {
    MAKE_DRAWABLE(ellipse)
  };
  struct arc : public drawable<arc> {
    MAKE_DRAWABLE(arc)
  };
  struct triangle : public drawable<triangle> {
    MAKE_DRAWABLE(triangle)
  };
  struct star : public drawable<star> {
    MAKE_DRAWABLE(star)
  };
  struct polygon : public drawable<polygon> {
    MAKE_DRAWABLE(polygon)
  };
  struct text : public drawable<text> {
    MAKE_DRAWABLE(text)
  };
  //@formatter:on
  // note: dot, rect (line, rect, ellipse, arc, text), poly (triangle, star,
  // polygon)

  struct group : public drawable<group>
      , public visitable<drawable<group>> {
    MAKE_DRAWABLE(group)
    using drawable_t = std::unique_ptr<drawable>;
    using drawables_t = std::unordered_map<draw_id, drawable_t>;
    drawables_t drawables;
    void add(drawable_t &&t) { drawables.emplace(t->id(), std::move(t)); }
    return_t accept(visitor_t const &guest) override {
      for (auto const &[did, dr] : drawables) {
        guest.visit(dr);
        UNUSED(did);
      }
    }
  };

  struct layer : public drawable<layer>
      , public visitable<drawable<layer>> {
    MAKE_DRAWABLE(layer)
    // more: attrs, ...

    using drawable_t = std::unique_ptr<drawable>;
    using drawables_t = std::unordered_map<draw_id, drawable_t>;
    drawables_t drawables;
    void add(drawable_t &&t) { drawables.emplace(t->id(), std::move(t)); }

    using group_t = std::unique_ptr<group>;
    using groups_t = std::unordered_map<draw_id, group_t>;
    groups_t groups;
    void add(group_t &&t) { groups.emplace(t->id(), std::move(t)); }

    return_t accept(visitor_t const &guest) override {
      for (auto const &[did, dr] : drawables) {
        guest.visit(dr);
        UNUSED(did);
      }
      for (auto const &[did, dr] : groups) {
        // guest.visit(dr);
        UNUSED(did, dr);
      }
    }
  };

  struct canvas : public visitable<drawable<canvas>> {
    using layer_t = std::unique_ptr<layer>;
    using layers_t = std::unordered_map<draw_id, layer_t>;
    layers_t layers;
    void add(draw_id id) { layers.emplace(id, std::make_unique<layer>(id)); }
    layer_t &get(draw_id id) { return layers[id]; }
    layer_t &operator[](draw_id id) { return layers[id]; }

    return_t accept(visitor_t const &guest) override {
      // dbg_debug("[canva] visiting for: %s", to_string(guest).c_str());
      for (auto const &[lid, ly] : layers) {
        ly->accept(guest);
      }
      return;
    }
  };

  struct screen : public visitor<drawable<screen>> {
    return_t visit(visited_t const &visited) override {
      dbg_debug("[screen][draw] for: %s", to_string(visited.get()).c_str());
      UNUSED(visited);
    }
    friend std::ostream &operator<<(std::ostream &os, screen const &) {
      return os << "[screen] ";
    }
  };

  struct printer : public visitor<drawable<printer>> {
    return_t visit(visited_t const &visited) override {
      dbg_debug("[printer][draw] for: %s", to_string(visited.get()).c_str());
      UNUSED(visited);
    }
    friend std::ostream &operator<<(std::ostream &os, printer const &) {
      return os << "[printer] ";
    }
  };

#undef MAKE_DRAWABLE

} // namespace hicc::dp::visitor::basic

void test_visitor_basic() {
  using namespace hicc::dp::visitor::basic;

  canvas c;
  static draw_id id = 0, did = 0;
  c.add(++id);
  auto layer0 = c[1];
  layer0->add(std::make_unique<line>(++did));
  layer0->add(std::make_unique<line>(++did));
  layer0->add(std::make_unique<rect>(++did));

  screen scr;
  c.accept(scr);
}

int main() {
  HICC_TEST_FOR(test_visitor_basic);
  return 0;
}