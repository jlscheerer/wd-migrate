#ifndef PARSER_WIKIDATA_COLUMNS_H
#define PARSER_WIKIDATA_COLUMNS_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include "../utils/date.h"

static const char kClaimId[] = "claim_id";
static const char kPropety[] = "property";
static const char kHash[] = "hash";
static const char kSnaktype[] = "snaktype";
static const char kQualifierProperty[] = "qualifier_property";
static const char kDatavalueString[] = "datavalue_string";
static const char kDatavalueEntity[] = "datavalue_entity";
static const char kDatavalueDate[] = "datavalue_date";
static const char kNil[] = "nil";
static const char kDatavalueType[] = "datavalue_type";
static const char kDatatype[] = "datatype";
static const char kCounter[] = "counter";
static const char kOrderHash[] = "order_hash";

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

#endif // !PARSER_WIKIDATA_COLUMNS_H
