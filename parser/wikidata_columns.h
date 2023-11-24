#ifndef PARSER_WIKIDATAUMNS_H
#define PARSER_WIKIDATAUMNS_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include "../utils/date.h"

namespace wd_migrate {
struct claims_tag_t {};
struct qualifiers_tag_t {};

using iso_time_t = date::sys_time<std::chrono::milliseconds>;

struct wd_string_t {
  const std::string value;
};

struct wd_entity_id_t {
  const std::string value;
};

struct wd_text_t {
  const std::string text;
  const std::string language;
};

struct wd_time_t {
public:
  const std::string time;
  const iso_time_t iso8601;

  // NOTE For calendarmodel we have (Q1985727 = "Gregorian Calendar", >99%)
  //      and (Q1985786 = "Julian Calendar")
  const std::string calendermodel;

  // NOTE >99.9% of values have timezone = 0. The others are 1 and 60.
  const std::uint64_t timezone;

  const std::uint64_t before;
  const std::uint64_t after;
  const std::uint64_t precision;
};

struct wd_quantity_t {
  const std::string quantity;

  const std::optional<std::string> unit;

  const std::string lower_bound;
  const std::string upper_bound;
};

struct wd_coordinate_t {
  const std::string latitude;
  const std::string longitude;
  const std::string altitude;

  const std::string precision;
  const std::string globe;
};

template <typename type> struct wd_novalue_t {};
template <typename type> struct wd_invalid_t : public wd_novalue_t<type> {};

namespace detail {
template <const char *column_name, typename column_type> struct wd_column_info {
public:
  template <const char *field_name> static constexpr auto is_named() {
    return column_name == field_name;
  }

  column_type data_;
};

template <typename... columns> struct wd_column_pack;

template <> struct wd_column_pack<> {
  static constexpr auto size() -> std::uint64_t { return 0; }

  template <typename reader_type, typename... column_types>
  auto read_row(reader_type &reader, column_types &&...columns) -> bool {
    return reader.read_row(std::forward<column_types>(columns)...);
  }
};

template <typename head, typename... tail>
struct wd_column_pack<head, tail...> {
  static constexpr auto size() -> std::uint64_t {
    return 1 + decltype(tail_)::size();
  }

  template <const char *field_name> const auto &get_field() const {
    if constexpr (head::template is_named<field_name>()) {
      return head_.data_;
    } else {
      return tail_.template get_field<field_name>();
    }
  }

  template <typename reader_type, typename... column_types>
  auto read_row(reader_type &reader, column_types &&...columns) -> bool {
    return tail_.read_row(reader, std::forward<column_types>(columns)...,
                          head_.data_);
  }

private:
  head head_;
  wd_column_pack<tail...> tail_;
};

static const char kEntityId[] = "entity_id";
using col_entity_id = wd_column_info<kEntityId, std::string>;

static const char kClaimsType[] = "type";
using col_claims_type = wd_column_info<kClaimsType, std::string>;

static const char kClaimsRank[] = "rank";
using col_claims_rank = wd_column_info<kClaimsRank, std::string>;

static const char kClaimId[] = "claim_id";
using col_claims_id = wd_column_info<kClaimId, std::string>;

static const char kPropety[] = "property";
using col_property = wd_column_info<kPropety, std::string>;

static const char kHash[] = "hash";
using col_hash = wd_column_info<kHash, std::string>;

static const char kSnaktype[] = "snaktype";
using col_snaktype = wd_column_info<kSnaktype, std::string>;

static const char kQualifierProperty[] = "qualifier_property";
using col_qualifier_property = wd_column_info<kQualifierProperty, std::string>;

static const char kDatavalueString[] = "datavalue_string";
using col_datavalue_string = wd_column_info<kDatavalueString, std::string>;

static const char kDatavalueEntity[] = "datavalue_entity";
using col_datavalue_entity = wd_column_info<kDatavalueEntity, std::string>;

static const char kDatavalueDate[] = "datavalue_date";
using col_datavalue_date = wd_column_info<kDatavalueDate, std::string>;

static const char kNil[] = "nil";
using col_nil = wd_column_info<kNil, std::string>;

static const char kDatavalueType[] = "datavalue_type";
using col_datavalue_type = wd_column_info<kDatavalueType, std::string>;

static const char kDatatype[] = "datatype";
using col_datatype = wd_column_info<kDatatype, std::string>;

static const char kCounter[] = "counter";
using col_counter = wd_column_info<kCounter, std::uint64_t>;

static const char kOrderHash[] = "order_hash";
using col_order_hash = wd_column_info<kOrderHash, std::uint64_t>;

template <typename tag> struct columns_info;
template <> struct columns_info<claims_tag_t> {
  using type =
      wd_column_pack<col_entity_id, col_claims_id, col_claims_type,
                     col_claims_rank, col_snaktype, col_property,
                     col_datavalue_string, col_datavalue_entity,
                     col_datavalue_date, col_datavalue_type, col_datatype>;
};
template <> struct columns_info<qualifiers_tag_t> {
  using type = wd_column_pack<col_claims_id, col_property, col_hash,
                              col_snaktype, col_qualifier_property,
                              col_datavalue_string, col_datavalue_entity,
                              col_datavalue_date, col_nil, col_datavalue_type,
                              col_datatype, col_counter, col_order_hash>;
};
template <typename tag> using columns_info_t = typename columns_info<tag>::type;
} // namespace detail
} // namespace wd_migrate

#endif // !PARSER_WIKIDATAUMNS_H
