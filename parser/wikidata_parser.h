#ifndef PARSER_WIKIDATA_PARSER_H
#define PARSER_WIKIDATA_PARSER_H

#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include "../fast-cpp-csv-parser/csv.h"
#include "../utils/progress_indicator.h"
#include "wikidata_columns.h"

namespace wd_migrate {
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

template <typename derived> struct wd_datavalue_type_parser {
public:
  template <typename columns_type>
  static auto can_parse(const columns_type &columns) -> bool {
    return derived::kTypeIdentifier ==
           columns.template get_field<kDatavalueType>();
  }
};

struct wd_fallback_parser {
public:
  template <typename columns_type>
  static auto can_parse(const columns_type &columns) -> bool {
    return true;
  }
  template <typename result_handler, typename columns_type>
  static auto parse(result_handler *handler, const columns_type &columns)
      -> void {
    std::cerr << "Unexpected datavalue_type encountered."
              << std::string(20, ' ') << std::endl;
    std::cerr << "datavalue_type: "
              << columns.template get_field<kDatavalueType>() << std::endl;
    std::exit(-1);
  }
};

struct wd_string_parser : public wd_datavalue_type_parser<wd_string_parser> {
public:
  static const inline std::string kTypeIdentifier = "string";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns)
      -> void {
    const std::string &str = columns.template get_field<kDatavalueString>();
    // NOTE "parsing" the string type is trivial.
    handler->handle(columns, wd_string_t{.value = str});
  }
};

struct wd_entity_parser : public wd_datavalue_type_parser<wd_entity_parser> {
public:
  static const inline std::string kTypeIdentifier = "wikibase-entityid";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns)
      -> void {
    const std::string &entity_id =
        columns.template get_field<kDatavalueEntity>();
    // NOTE "parsing" the entity_id is trivial.
    handler->handle(columns, wd_entity_id_t{.value = entity_id});
  }
};

struct wd_text_parser : public wd_datavalue_type_parser<wd_text_parser> {
public:
  static const inline std::string kTypeIdentifier = "monolingualtext";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns)
      -> void {
    const std::string &text_str =
        columns.template get_field<kDatavalueString>();
    if (text_str == "novalue") {
      handler->handle(columns, wd_novalue_t<wd_text_t>{});
      return;
    }
    std::smatch text_match;
    if (!std::regex_match(text_str, text_match, text_regex)) {
      std::cerr << "Unexpected text string encountered." << std::endl;
      std::cerr << "text_str: " << text_str << std::endl;
      std::exit(-1);
    }
    std::string text(text_match[1].str()), language(text_match[2].str());
    handler->handle(columns, wd_text_t{.text = text, .language = language});
  }

private:
  static const inline std::regex text_regex =
      std::regex("^\\{\"text\"=>\"(.*?)\", \"language\"=>\"([^\"]*?)\"\\}$");
};

struct wd_time_parser : public wd_datavalue_type_parser<wd_time_parser> {
public:
  static const inline std::string kTypeIdentifier = "time";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns) {
    const std::string &time_str =
        columns.template get_field<kDatavalueString>();
    if (time_str == "novalue") {
      handler->handle(columns, wd_novalue_t<wd_time_t>{});
      return;
    }
    std::smatch time_match;
    if (!std::regex_match(time_str, time_match, time_regex)) {
      std::cerr << "Unexpected time string encountered." << std::endl;
      std::cerr << "time_str: " << time_str << std::endl;
      std::exit(-1);
    }
    std::string time(time_match[1].str()), calendarmodel(time_match[6].str());
    std::uint64_t timezone(std::stoull(time_match[2].str())),
        before(std::stoull(time_match[3].str())),
        after(std::stoull(time_match[4].str())),
        precision(std::stoull(time_match[5].str()));
    handler->handle(columns, wd_time_t{.time = time,
                                       .calendermodel = calendarmodel,
                                       .timezone = timezone,
                                       .before = before,
                                       .after = after,
                                       .precision = precision});
  }

private:
  static const inline std::regex time_regex = std::regex(
      "^\\{\"time\"=>\"([^\"]*?)\", \"timezone\"=>(\\d+), \"before\"=>(\\d+), "
      "\"after\"=>(\\d+), \"precision\"=>(\\d+).*, "
      "\"calendarmodel\"=>\"http://www.wikidata.org/entity/([^\"]*?)\"\\}$");
};

struct wd_quantity_parser
    : public wd_datavalue_type_parser<wd_quantity_parser> {
public:
  static const inline std::string kTypeIdentifier = "quantity";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns)
      -> void {
    const std::string &quantity_str =
        columns.template get_field<kDatavalueString>();
    std::smatch quantity_match;
    if (quantity_str == "novalue") {
      handler->handle(columns, wd_novalue_t<wd_quantity_t>{});
      return;
    }
    if (!std::regex_match(quantity_str, quantity_match, quantity_regex)) {
      std::cerr << "Unexpected quantity string encountered." << std::endl;
      std::cerr << "quantity_str: " << quantity_str << std::endl;
      std::exit(-1);
    }
    std::string quantity(quantity_match[1].str()),
        unit(quantity_match[2].str()), upper_bound(quantity_match[4].str()),
        lower_bound(quantity_match[6].str());
    handler->handle(columns, wd_quantity_t{.quantity = quantity,
                                           .unit = unit,
                                           .lower_bound = lower_bound,
                                           .upper_bound = upper_bound});
  }

private:
  static const inline std::regex quantity_regex = std::regex(
      "\\{\"amount\"=>\"([^\"]*?)\", \"unit\"=>\"([^\"]*?)\"(, "
      "\"upperBound\"=>\"([^\"]*?)\")?(, \"lowerBound\"=>\"([^\"]*?)\")?\\}");
};

struct wd_coordinate_parser
    : public wd_datavalue_type_parser<wd_coordinate_parser> {
public:
  static const inline std::string kTypeIdentifier = "globecoordinate";
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns)
      -> void {
    const std::string &coordinate_str =
        columns.template get_field<kDatavalueString>();
    if (coordinate_str == "novalue") {
      handler->handle(columns, wd_novalue_t<wd_coordinate_t>{});
      return;
    }
    std::smatch coordinate_match;
    if (!std::regex_match(coordinate_str, coordinate_match, coordinate_regex)) {
      std::cerr << "Unexpected coordinate string encountered." << std::endl;
      std::cerr << "coordinate_str: " << coordinate_str << std::endl;
      std::exit(-1);
    }
    std::string latitude(coordinate_match[1].str()),
        longitude(coordinate_match[2].str()),
        altitude(coordinate_match[3].str()),
        precision(coordinate_match[4].str()), globe(coordinate_match[5].str());
    handler->handle(columns, wd_coordinate_t{.latitude = latitude,
                                             .longitude = longitude,
                                             .altitude = altitude,
                                             .precision = precision,
                                             .globe = globe});
  }

private:
  // {"latitude"=>38.70661, "longitude"=>-77.08723, "altitude"=>nil,
  // "precision"=>0.000277778, "globe"=>"http://www.wikidata.org/entity/Q2"}
  static const inline std::regex coordinate_regex =
      std::regex("\\{\"latitude\"=>([^,]*?), \"longitude\"=>([^,]*?), "
                 "\"altitude\"=>([^,]*?), \"precision\"=>([^,]*?), "
                 "\"globe\"=>\"([^\"]*?)\"\\}");
};

template <typename... parsers> struct wd_combined_parser;

template <> struct wd_combined_parser<> {
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns) {
    detail::wd_fallback_parser::parse(handler, columns);
  }
};

template <typename head, typename... tail>
struct wd_combined_parser<head, tail...> {
public:
  template <typename result_handler, typename columns_type>
  static auto parse_row(result_handler *handler, const columns_type &columns) {
    if (head::can_parse(columns)) {
      head::parse_row(handler, columns);
    } else {
      wd_combined_parser<tail...>::parse_row(handler, columns);
    }
  }
};

using wd_primitives_parser =
    wd_combined_parser<wd_string_parser, wd_entity_parser, wd_time_parser,
                       wd_coordinate_parser, wd_quantity_parser,
                       wd_text_parser>;

template <typename parser, typename result_handler, typename columns_type>
class wikidata_parser_impl {
public:
  auto parse(const std::string &filename, result_handler *handler) -> void {
    io::CSVReader<columns_type::size(), io::trim_chars<' '>,
                  io::no_quote_escape<'\t'>>
        reader(filename);
    utils::progress_indicator progress("parsing " + filename);
    progress.start();
    while (columns_.read_row(reader)) {
      parser::parse_row(handler, columns_);
      progress.update();
    }
    progress.done();
  }

protected:
  columns_type columns_;
};
} // namespace detail
} // namespace wd_migrate

#endif // !PARSER_WIKIDATA_PARSER_H
