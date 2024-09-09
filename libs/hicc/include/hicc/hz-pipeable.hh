//
// Created by Hedzr Yeh on 2021/8/21.
//

#ifndef HICC_CXX_HZ_PIPEABLE_HH
#define HICC_CXX_HZ_PIPEABLE_HH

#include <functional>
#include <utility>

namespace hicc::pipeable {

  template<class F>
  struct pipeable {
  private:
    F f;

  public:
    pipeable(F &&f_)
        : f(std::forward<F>(f_)) {}

    template<class... Xs>
    auto operator()(Xs &&...xs) -> decltype(std::bind(f, std::placeholders::_1, std::forward<Xs>(xs)...)) const {
      return std::bind(f, std::placeholders::_1, std::forward<Xs>(xs)...);
    }
  };

  template<class F>
  pipeable<F> piped(F &&f) { return pipeable<F>{std::forward<F>(f)}; }

  template<class T, class F>
  auto operator|(T &&x, const F &f) -> decltype(f(std::forward<T>(x))) {
    return f(std::forward<T>(x));
  }

} // namespace hicc::pipeable

#endif //HICC_CXX_HZ_PIPEABLE_HH
