#ifndef HANDLER_WIKIDATA_HANDLER_H
#define HANDLER_WIKIDATA_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <ostream>

namespace wd_migrate {
template <bool fail_if_not_handled = false> struct empty_handler {
  template <typename T> auto handle(const T &value) {
    if constexpr (fail_if_not_handled) {
      std::cerr << "handler failed to handle type." << std::endl;
      std::exit(-1);
    }
  }
};
} // namespace wd_migrate

#endif // !HANDLER_WIKIDATA_HANDLER_H
