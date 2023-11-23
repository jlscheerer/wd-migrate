#ifndef HANDLER_WIKIDATA_HANDLER_H
#define HANDLER_WIKIDATA_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <type_traits>
#include <utility>

#include "../parser/wikidata_columns.h"

namespace wd_migrate {
template <bool fail_if_unhandled = false> struct empty_handler {
  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns, const result_type &value) -> void {
    if constexpr (fail_if_unhandled) {
      std::cerr << "handler failed to handle type." << std::string(30, ' ')
                << std::endl;
      std::exit(-1);
    }
  }
};

struct skip_novalue_handler : public empty_handler</*fail_if_unhandled=*/true> {
public:
  template <typename columns_type, typename result_type>
  auto handle(const wd_novalue_t<result_type> &value) {}
  using empty_handler::handle;
};

template <typename... handlers> struct stacked_handler;

template <> struct stacked_handler<> {
  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns, const result_type &value) {}
};

template <typename head_type, typename... tail>
struct stacked_handler<head_type, tail...> {
public:
  template <typename... tail_args_types>
  stacked_handler(head_type &&head, tail_args_types &&...tail_args)
      : head_(std::move(head)),
        tail_(std::forward<tail_args_types>(tail_args)...) {}

  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns, const result_type &value) {
    head_.handle(columns, value);
    tail_.handle(columns, value);
  }

  template <typename handler_type> auto &get() {
    if constexpr (std::is_same_v<head_type, handler_type>) {
      return head_;
    } else {
      return tail_.template get<handler_type>();
    }
  }

private:
  head_type head_;
  stacked_handler<tail...> tail_;
};

template <typename... args>
stacked_handler(args &&...) -> stacked_handler<args...>;

struct stats_handler : public empty_handler</*fail_if_unhandled=*/true> {
public:
  auto summary() -> void {
    std::cout << "row count: " << row_count_ << std::endl;
    std::cout << "missing values ("
              << (nv_string_ + nv_entity_ + nv_text_ + nv_time_ + nv_quantity_ +
                  nv_coordinate_)
              << "): " << std::endl;
    std::cout << "  "
              << "string: " << nv_string_ << std::endl;
    std::cout << "  "
              << "entity: " << nv_entity_ << std::endl;
    std::cout << "  "
              << "text: " << nv_text_ << std::endl;
    std::cout << "  "
              << "time: " << nv_time_ << std::endl;
    std::cout << "  "
              << "quantity: " << nv_quantity_ << std::endl;
    std::cout << "  "
              << "coordinate: " << nv_coordinate_ << std::endl;
  }

public: // result handlers
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_string_t &value) -> void {
    ++row_count_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_entity_id_t &value)
      -> void {
    ++row_count_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_text_t &value) -> void {
    ++row_count_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_time_t &value) -> void {
    ++row_count_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
    ++row_count_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_coordinate_t &value)
      -> void {
    ++row_count_;
  }

  // Count Missing Values
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_novalue_t<wd_string_t> &value) -> void {
    ++row_count_, ++nv_string_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_novalue_t<wd_entity_id_t> &value) -> void {
    ++row_count_, ++nv_entity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_novalue_t<wd_text_t> &value)
      -> void {
    ++row_count_, ++nv_text_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_novalue_t<wd_time_t> &value)
      -> void {
    ++row_count_, ++nv_time_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_novalue_t<wd_quantity_t> &value) -> void {
    ++row_count_, ++nv_quantity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_novalue_t<wd_coordinate_t> &value) -> void {
    ++row_count_, ++nv_coordinate_;
  }

  using empty_handler::handle;

private:
  std::uint64_t row_count_ = 0;
  std::uint64_t nv_string_ = 0, nv_entity_ = 0, nv_text_ = 0, nv_time_ = 0,
                nv_quantity_ = 0, nv_coordinate_ = 0;
};

} // namespace wd_migrate

#endif // !HANDLER_WIKIDATA_HANDLER_H
