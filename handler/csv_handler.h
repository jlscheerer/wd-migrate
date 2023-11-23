#ifndef HANDLER_CSV_HANDLER_H
#define HANDLER_CSV_HANDLER_H

#include "wikidata_handler.h"

namespace wd_migrate {
struct csv_handler : public skip_novalue_handler {
public:
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_string_t &value) -> void {}

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_entity_id_t &value)
      -> void {
    std::cout << get_claim_id(columns) << ' ' << value.value << std::endl;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_text_t &value) -> void {}

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_time_t &value) -> void {}

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_coordinate_t &value)
      -> void {}

  using skip_novalue_handler::handle;

private:
  template <typename columns_type>
  static auto get_claim_id(const columns_type &columns) -> const std::string & {
    return columns.template get_field<kClaimId>();
  }
};
} // namespace wd_migrate

#endif // !HANDLER_CSV_HANDLER_H
