#ifndef HANDLER_WIKIDATA_HANDLER_H
#define HANDLER_WIKIDATA_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <utility>

namespace wd_migrate {
template <bool fail_if_not_handled = false> struct empty_handler {
  template <typename result_type> auto handle(const result_type &value) {
    if constexpr (fail_if_not_handled) {
      std::cerr << "handler failed to handle type." << std::endl;
      std::exit(-1);
    }
  }
};

template <typename... handlers> struct stacked_handler;

template <> struct stacked_handler<> {
  template <typename result_type> auto handle(const result_type &value) {}
};

template <typename head_type, typename... tail>
struct stacked_handler<head_type, tail...> {
public:
  template <typename... tail_args_types>
  stacked_handler(head_type &&head, tail_args_types &&...tail_args)
      : head_(std::move(head)),
        tail_(std::forward<tail_args_types>(tail_args)...) {}

  template <typename result_type> auto handle(const result_type &value) {
    head_.handle(value);
    tail_.handle(value);
  }

private:
  head_type head_;
  stacked_handler<tail...> tail_;
};

template <typename... args>
stacked_handler(args &&...) -> stacked_handler<args...>;

} // namespace wd_migrate

#endif // !HANDLER_WIKIDATA_HANDLER_H
