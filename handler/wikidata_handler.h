#ifndef HANDLER_WIKIDATA_HANDLER_H
#define HANDLER_WIKIDATA_HANDLER_H

#include <cstdint>
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
      std::cerr << "handler failed to handle type: "
                << typeid(result_type).name() << '.' << std::string(30, ' ')
                << std::endl;
      std::exit(-1);
    }
  }
};

struct skip_novalue_handler : public empty_handler</*fail_if_unhandled=*/true> {
public:
  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns,
              const wd_novalue_t<result_type> &value) {}

  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns,
              const wd_invalid_t<result_type> &value) {}
  using empty_handler::handle;
};

template <typename... handlers> struct stacked_handler;

template <> struct stacked_handler<> {
  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns, const result_type &value) {}
  auto summary() -> void {}
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

  auto summary() -> void {
    head_.summary();
    tail_.summary();
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

template <const bool print_illegal_values = false>
struct stats_handler : public empty_handler</*fail_if_unhandled=*/true> {
public:
  auto summary() -> void {
    std::cout << "row count: " << row_count_ << std::endl;

    std::cout << "parsed values ("
              << (ct_string_ + ct_entity_ + ct_text_ + ct_time_ + ct_quantity_ +
                  ct_coordinate_)
              << "): " << std::endl;
    std::cout << "  "
              << "string: " << ct_string_ << std::endl;
    std::cout << "  "
              << "entity: " << ct_entity_ << std::endl;
    std::cout << "  "
              << "text: " << ct_text_ << std::endl;
    std::cout << "  "
              << "time: " << ct_time_ << std::endl;
    std::cout << "  "
              << "quantity: " << ct_quantity_ << std::endl;
    std::cout << "  "
              << "coordinate: " << ct_coordinate_ << std::endl;

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

    std::cout << "invalid values ("
              << (iv_string_ + iv_entity_ + iv_text_ + iv_time_ + iv_quantity_ +
                  iv_coordinate_)
              << "): " << std::endl;
    std::cout << "  "
              << "string: " << iv_string_ << std::endl;
    std::cout << "  "
              << "entity: " << iv_entity_ << std::endl;
    std::cout << "  "
              << "text: " << iv_text_ << std::endl;
    std::cout << "  "
              << "time: " << iv_time_ << std::endl;
    std::cout << "  "
              << "quantity: " << iv_quantity_ << std::endl;
    std::cout << "  "
              << "coordinate: " << iv_coordinate_ << std::endl;
  }

public: // result handlers
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_string_t &value) -> void {
    ++row_count_, ++ct_string_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_entity_id_t &value)
      -> void {
    ++row_count_, ++ct_entity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_text_t &value) -> void {
    ++row_count_, ++ct_text_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_time_t &value) -> void {
    ++row_count_, ++ct_time_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
    ++row_count_, ++ct_quantity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_coordinate_t &value)
      -> void {
    ++row_count_, ++ct_coordinate_;
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

  // Count Parsing Errors
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_invalid_t<wd_string_t> &value) -> void {
    ++row_count_, ++iv_string_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_invalid_t<wd_entity_id_t> &value) -> void {
    ++row_count_, ++iv_entity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_invalid_t<wd_text_t> &value)
      -> void {
    ++row_count_, ++iv_text_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_invalid_t<wd_time_t> &value)
      -> void {
    if constexpr (print_illegal_values) {
      std::cout << columns.template get_field<detail::kDatavalueString>()
                << std::endl;
    }
    ++row_count_, ++iv_time_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_invalid_t<wd_quantity_t> &value) -> void {
    ++row_count_, ++iv_quantity_;
  }
  template <typename columns_type>
  auto handle(const columns_type &columns,
              const wd_invalid_t<wd_coordinate_t> &value) -> void {
    ++row_count_, ++iv_coordinate_;
  }

  using empty_handler::handle;

private:
  std::uint64_t row_count_ = 0;

  // "novalue" counts.
  std::uint64_t ct_string_ = 0, ct_entity_ = 0, ct_text_ = 0, ct_time_ = 0,
                ct_quantity_ = 0, ct_coordinate_ = 0;

  // "novalue" counts.
  std::uint64_t nv_string_ = 0, nv_entity_ = 0, nv_text_ = 0, nv_time_ = 0,
                nv_quantity_ = 0, nv_coordinate_ = 0;

  // "illegal value" counts.
  std::uint64_t iv_string_ = 0, iv_entity_ = 0, iv_text_ = 0, iv_time_ = 0,
                iv_quantity_ = 0, iv_coordinate_ = 0;
};

struct quantity_scale_handler : empty_handler</*fail_if_unhandled=*/false> {
public:
  auto summary() -> void {
    std::cout << "precision: " << (integer_ + fractional_)
              << ", scale: " << fractional_ << std::endl;
  }

public: // result handlers
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
    const auto dot_index = value.quantity.find(".");
    if (dot_index != std::string::npos) {
      integer_ = std::max(integer_, static_cast<std::uint64_t>(dot_index) - 1);
      std::uint64_t decimals = value.quantity.size() - 1 - dot_index;
      fractional_ = std::max(fractional_, decimals);
    } else {
      integer_ = std::max(
          integer_, static_cast<std::uint64_t>(value.quantity.size()) - 1);
    }
  }

  using empty_handler::handle;

private:
  std::uint64_t integer_ = 0, fractional_ = 0;
};

} // namespace wd_migrate

#endif // !HANDLER_WIKIDATA_HANDLER_H
