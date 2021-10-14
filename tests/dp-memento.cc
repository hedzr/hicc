//
// Created by Hedzr Yeh on 2021/10/3.
//

#include <algorithm>
#include <functional>
#include <memory>
#include <new>
#include <random>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <any>
#include <array>
#include <chrono>
#include <deque>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <math.h>

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-string.hh"
#include "hicc/hz-util.hh"
#include "hicc/hz-x-test.hh"

namespace hicc::dp::command::basic {

    struct command_base {
        virtual ~command_base() {}
        void execute() { do_execute(); }

    protected:
        virtual void do_execute() = 0;
    };

    template<typename CmdT = command_base>
    class command_t : public command_base {
    public:
        virtual ~command_t() {}
        void do_execute() override {
            //
        }
    };

    template<typename CmdT = command_base>
    class history_t {
    public:
        history_t() {}
        virtual ~history_t() { _history.clear(); }

    public:
        using Cmd = command_t<CmdT>;
        using History = std::list<Cmd>;
        void push(Cmd &&cmd) { _history.emplace_back(cmd); }
        Cmd pop() {
            Cmd cmd = _history.back();
            _history.pop_back();
            return cmd;
        }

    protected:
        History _history{};
    };

} // namespace hicc::dp::command::basic

namespace hicc::dp::command::basic::commands {

    struct copy_command : public command_t<copy_command> {
        virtual ~copy_command() {}
        void do_execute() override;
    };

    struct cut_command : public command_t<cut_command> {
        virtual ~cut_command() {}
        void do_execute() override;
    };

    struct paste_command : public command_t<paste_command> {
        virtual ~paste_command() {}
        void do_execute() override;
    };

} // namespace hicc::dp::command::basic::commands

namespace hicc::dp::command::editor {

    template<typename CmdT = basic::command_base,
             typename HistoryT = basic::history_t<CmdT>>
    class editor {
    public:
        editor() {}
        ~editor() {}
        using Cmd = basic::command_t<CmdT>;

    public:
        void enter_loop() {}

    public:
        enum class Modifiers {
            Shift,
            Ctrl,
            Option_or_Alt,
            Command_or_Win,
        };
        struct kb_event {
            int key_code;
            Modifiers modifiers;
        };

        void on_user_actions(kb_event const kbe) {
            using CutCmd = basic::commands::cut_command;
            using CopyCmd = basic::commands::copy_command;
            using PasteCmd = basic::commands::paste_command;
            if ((kbe.modifiers & Modifiers::Ctrl) != 0) {
                switch (kbe.key_code) {
                    case 'X':
                        invoke(CutCmd{});
                        break;
                    case 'C':
                        invoke(CopyCmd{});
                        break;
                    case 'V':
                        invoke(PasteCmd{});
                        break;
                }
            }
        }

    protected:
        void invoke(Cmd &&cmd) {
            cmd.execute();
            _history.push(cmd);
        }

    private:
        HistoryT _history{};
    };
    
} // namespace hicc::dp::command::editor

namespace hicc::dp::command::app {
    template<typename Source>
    class clipboard final {
    public:
        clipboard() {}
        ~clipboard() {}

    public:
        void cut(Source &) {}
        void copy(Source &) {}
        void paste(Source &) {}

    private:
        std::string _text{};
    };

    // kb
    // mouse
    // ole,
    // socket,
    // ...

    template<typename Source>
    struct editors_holder final {
    public:
        editors_holder() {}
        ~editors_holder() {}

    public:
        using Editor = Source;
        using Editors = std::list<Editor>;
        Editors _editors;
        typename Editors::iterator _current;
        Editor &current() { return *_current; }

        void add(Editor &&editor) {
            _editors.emplace_back(editor);
            _current = _editors.end();
            _current--;
        }
    };

    class app {
        struct token {};
        app() {}
        app(app const &) = delete;
        app &operator=(app const) = delete;

    public:
        app(token const &) {}
        static app &instance() {
            static std::unique_ptr<app> instance_{new app{token{}}};
            return *instance_;
        }

    public:
        using Editor = editor::editor<>;
        clipboard<Editor> clipboard;
        editors_holder<Editor> editors;
    };

} // namespace hicc::dp::command::app

namespace hicc::dp::command::basic::commands {
    inline void copy_command::do_execute() {
        auto &curr_editor = hicc::dp::command::app::app::instance().editors.current();
        app::app::instance().clipboard.copy(curr_editor);
    }
    inline void cut_command::do_execute() {
        auto &curr_editor = hicc::dp::command::app::app::instance().editors.current();
        app::app::instance().clipboard.cut(curr_editor);
    }
    inline void paste_command::do_execute() {
        auto &curr_editor = hicc::dp::command::app::app::instance().editors.current();
        app::app::instance().clipboard.paste(curr_editor);
    }
} // namespace hicc::dp::command::basic::commands

void test_command_basic() {
    using namespace hicc::dp::command::basic;
    using namespace hicc::dp::command::editor;
    auto &app = hicc::dp::command::app::app::instance();
    app.editors.add(editor<>{});
    auto &curr_editor = app.editors.current();
    curr_editor.enter_loop();
}

namespace hicc::dp::memento::basic {

    struct state {};
    struct context {};

    namespace detail {
        class undo_base {
        public:
            virtual ~undo_base() {}
            virtual void save(state const &s) = 0;
            virtual void restore(state &s) = 0;
        };
    } // namespace detail

    template<typename T>
    class undoable : public detail::undo_base {
    public:
        virtual ~undoable() {}
    };


} // namespace hicc::dp::memento::basic

void test_memento_basic() {
    using namespace hicc::dp::memento::basic;
}

int main() {

    HICC_TEST_FOR(test_memento_basic);

    return 0;
}