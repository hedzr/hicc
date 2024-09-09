//
// Created by Hedzr Yeh on 2021/9/3.
//

#ifndef HICC_CXX_DRAWING_HH
#define HICC_CXX_DRAWING_HH

#include "hicc/hz-defs.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-util.hh"

#include <iostream>
#include <math.h>
#include <string>

namespace draw {

    class draw_context {};

    class shape {
    public:
        virtual ~shape() {}
        virtual void on_draw(draw_context &ctx) {
            // shoule be a pure virtual declaration here
            UNUSED(ctx);
        }
    }; // class shape

    namespace basis {

        class point : public shape {
        public:
            float x, y;
        };

        class line : public shape {
        public:
            point start, end;
        };

        class rect : public shape {
        public:
            point tl, tr, bl, br;
        };
    } // namespace basis

    using color_t = uint32_t;
    using border_line_style_t = int;

    using elem_type_t = std::string;
    using elem_id_t = long;
    using elem_t = std::shared_ptr<shape>;
    using shapes_t = std::vector<elem_t>;
    using layer_id_t = int;
    using layered_shapes_t = std::map<layer_id_t, shapes_t>;

    const layer_id_t default_layer_id = 0;
    // const elem_id_t first_elem_id = 0;

    template<typename Super>
    class builder_base {
    public:
        // Shape build() {
        //     Shape s{};
        //     static_cast<Super *>(this)->apply_default_style(s);
        //     return s;
        // }
        Super &border_line(float width, color_t color, border_line_style_t style) {
            UNUSED(width, color, style);
            return *static_cast<Super *>(this);
        }
        // Super &apply_default_style(Shape &s) {
        //     UNUSED(s);
        //     // border line width: 1pt, color: black, style: solid
        //     // fill color
        //     // fill bg
        //     return *static_cast<Super *>(this);
        // }
    };

    namespace fct = hicc::util::factory;
    using typed_elem_builder = fct::factory<shape, basis::point, basis::line, basis::rect>;

    template<typename TB = typed_elem_builder>
    class elem_builder : public builder_base<elem_builder<TB>> {
    public:
        elem_builder() {}
        ~elem_builder() {}
        elem_t el{};
        elem_builder &choose(elem_type_t typ) {
            UNUSED(typ);
            return *this;
        }
    };

    class drawing {
    public:
        drawing() {}
        virtual ~drawing() {}
        typed_elem_builder _builder{};
        layered_shapes_t _all_shapes{};
        template<typename... Args>
        auto add(std::string_view const &type_id, layer_id_t lyid = default_layer_id, Args &&...args) {
            auto el = _builder.make_shared(type_id, args...);
            auto it = _all_shapes[lyid].emplace_back(std::forward<Args>(args)...);
            return it;
        }
        void draw() {
            draw_context ctx;
            for (auto [lyid, shapes] : _all_shapes) {
                for (auto &el : shapes) {
                    el->on_draw(ctx);
                }
            }
        }
    };

    class printable_drawing : public drawing {
    public:
        void print_drawing(draw_context &ctx) {
            UNUSED(ctx);
        }
    };

    template<typename Shape>
    class exportable_drawing {
    public:
        void export_drawing(draw_context &ctx) {
            UNUSED(ctx);
        }
    };

} // namespace draw

#endif //HICC_CXX_DRAWING_HH
