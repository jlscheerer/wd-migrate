#ifndef HANDLER_CSV_HANDLER_H
#define HANDLER_CSV_HANDLER_H

#include "wikidata_handler.h"
#include <exception>
#include <fstream>
#include <sstream>

namespace wd_migrate {
struct csv_handler : public skip_novalue_handler {
public:
  csv_handler(const std::string &filename) { output_.open(filename); }

  auto summary() -> void {
    output_.flush();
    output_.close();
  }

public: // result handlers
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_string_t &value) -> void {
    csv_output_row row = prepare_row(columns);
    row.datavalue_string = value.value;
    write(row);
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_entity_id_t &value)
      -> void {
    csv_output_row row = prepare_row(columns);
    row.datavalue_entity_id = value.value;
    write(row);
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_text_t &value) -> void {
    if (value.language != "en") {
      return;
    }
    csv_output_row row = prepare_row(columns);
    row.datavalue_string = value.text;
    write(row);
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_time_t &value) -> void {
    csv_output_row row = prepare_row(columns);
    // NOTE requires setting "set time zone UTC;" in psql
    row.datavalue_time = date::format("%Y-%m-%dT%T%z", value.iso8601);
    row.datavalue_entity_id = value.calendermodel;
    write(row);
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
    csv_output_row row = prepare_row(columns);
    row.datavalue_numeric = value.quantity;
    if (value.unit.has_value()) {
      row.datavalue_entity_id = *value.unit;
    }
    write(row);
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_coordinate_t &value)
      -> void {
    // NOTE Coordinates are by far the least common datatype.
    //      Therefore, we skip them for now.
  }

  using skip_novalue_handler::handle;

private:
  // clang-format off
  // <claims_id> <qualifier_property> <datavalue_datatype> <datavalue_string> <datavalue_entity_id> <datavalue_time> <datavalue_numeric>
  // clang-format on
  struct csv_output_row {
    std::string claim_id, qualifier_property, datavalue_datatype,
        datavalue_string, datavalue_entity_id, datavalue_time,
        datavalue_numeric;
  };

  auto write(const csv_output_row &row) -> void {
    output_ << row.claim_id << '\t' << row.qualifier_property << '\t'
            << row.datavalue_datatype << '\t' << row.datavalue_string << '\t'
            << row.datavalue_entity_id << '\t' << row.datavalue_time << '\t'
            << row.datavalue_numeric << '\n';
  }

  template <typename columns_type>
  static auto prepare_row(const columns_type &columns) -> csv_output_row {
    csv_output_row row;
    row.claim_id = get_claim_id(columns);
    row.qualifier_property = columns.template get_field<kQualifierProperty>();
    row.datavalue_datatype = columns.template get_field<kDatavalueType>();
    return row;
  }

  template <typename columns_type>
  static auto get_claim_id(const columns_type &columns) -> const std::string & {
    return columns.template get_field<kClaimId>();
  }

  std::ofstream output_;
};
} // namespace wd_migrate

#endif // !HANDLER_CSV_HANDLER_H
