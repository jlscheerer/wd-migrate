#ifndef HANDLER_CSV_HANDLER_H
#define HANDLER_CSV_HANDLER_H

#include "wikidata_handler.h"
#include <exception>
#include <fstream>
#include <sstream>

namespace wd_migrate {
namespace detail {
// TODO(jlscheerer) This design requires an explicit check for datatype.
//                  This is because we would otherwise join with the
//                  calendermodel.
struct claims_csv_output_row {
  std::string entity_id, claim_id, property, datavalue_datatype,
      datavalue_string, datavalue_entity_id, datavalue_time, datavalue_numeric;

  template <typename columns_type>
  static auto prepare_row(const columns_type &columns)
      -> claims_csv_output_row {
    claims_csv_output_row row;
    row.entity_id = columns.template get_field<detail::kEntityId>();
    row.claim_id = columns.template get_field<detail::kClaimId>();
    row.property = columns.template get_field<detail::kPropety>();
    row.datavalue_datatype =
        columns.template get_field<detail::kDatavalueType>();
    return row;
  }
};

template <typename ostream>
auto operator<<(ostream &os, const claims_csv_output_row &row) -> ostream & {
  os << row.entity_id << '\t' << row.claim_id << '\t' << row.property << '\t'
     << row.datavalue_datatype << '\t' << row.datavalue_string << '\t'
     << row.datavalue_entity_id << '\t' << row.datavalue_time << '\t'
     << row.datavalue_numeric << '\n';
  return os;
}

struct qualifiers_csv_output_row {
  std::string claim_id, qualifier_property, datavalue_datatype,
      datavalue_string, datavalue_entity_id, datavalue_time, datavalue_numeric;

  template <typename columns_type>
  static auto prepare_row(const columns_type &columns)
      -> qualifiers_csv_output_row {
    qualifiers_csv_output_row row;
    row.claim_id = columns.template get_field<detail::kClaimId>();
    row.qualifier_property =
        columns.template get_field<detail::kQualifierProperty>();
    row.datavalue_datatype =
        columns.template get_field<detail::kDatavalueType>();
    return row;
  }
};

template <typename ostream>
auto operator<<(ostream &os, const qualifiers_csv_output_row &row)
    -> ostream & {
  os << row.claim_id << '\t' << row.qualifier_property << '\t'
     << row.datavalue_datatype << '\t' << row.datavalue_string << '\t'
     << row.datavalue_entity_id << '\t' << row.datavalue_time << '\t'
     << row.datavalue_numeric << '\n';
  return os;
}

template <typename tag> struct csv_output_row {};
template <> struct csv_output_row<claims_tag_t> {
  using type = claims_csv_output_row;
};
template <> struct csv_output_row<qualifiers_tag_t> {
  using type = qualifiers_csv_output_row;
};
template <typename tag>
using csv_output_row_t = typename csv_output_row<tag>::type;
} // namespace detail

template <typename tag> struct csv_handler : public skip_novalue_handler {
  using csv_output_row = detail::csv_output_row_t<tag>;

public:
  csv_handler(const std::string &filename) { output_.open(filename); }

  auto summary() -> void {
    output_.flush();
    output_.close();
  }

public: // result handlers
  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_string_t &value) -> void {
    csv_output_row row = csv_output_row::prepare_row(columns);
    row.datavalue_string = value.value;
    output_ << row;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_entity_id_t &value)
      -> void {
    csv_output_row row = csv_output_row::prepare_row(columns);
    row.datavalue_entity_id = value.value;
    output_ << row;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_text_t &value) -> void {
    if (value.language != "en") {
      return;
    }
    csv_output_row row = csv_output_row::prepare_row(columns);
    row.datavalue_string = value.text;
    output_ << row;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_time_t &value) -> void {
    if (value.get_year() <= -4713 || value.get_year() >= 294276) {
      return; // NOTE postgres does not support timestamp not within this range.
              // See
              // https://www.postgresql.org/docs/current/datatype-datetime.html.
    }
    csv_output_row row = csv_output_row::prepare_row(columns);
    // NOTE requires setting "set time zone UTC;" in psql
    row.datavalue_time = value.str();
    row.datavalue_entity_id = value.calendermodel;
    output_ << row;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_quantity_t &value) -> void {
    csv_output_row row = csv_output_row::prepare_row(columns);
    row.datavalue_numeric = value.quantity;
    if (value.unit.has_value()) {
      row.datavalue_entity_id = *value.unit;
    }
    output_ << row;
  }

  template <typename columns_type>
  auto handle(const columns_type &columns, const wd_coordinate_t &value)
      -> void {
    // NOTE Coordinates are by far the least common datatype.
    //      Therefore, we skip them for now.
  }

  using skip_novalue_handler::handle;

private:
  std::ofstream output_;
};
} // namespace wd_migrate

#endif // !HANDLER_CSV_HANDLER_H
