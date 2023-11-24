#ifndef PARSER_WIKIDATA_PARSER_H
#define PARSER_WIKIDATA_PARSER_H

#include <cstdlib>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <utility>

#include "../fast-cpp-csv-parser/csv.h"
#include "../utils/progress_indicator.h"
#include "wikidata_columns.h"

namespace wd_migrate {
namespace detail {

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
    if (str == "novalue" || str.size() == 0) {
      // TODO(jlscheerer) Investigate why this happens.
      handler->handle(columns, wd_novalue_t<wd_string_t>{});
      return;
    }
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
    if (entity_id.size() == 0) {
      handler->handle(columns, wd_novalue_t<wd_entity_id_t>{});
      return;
    }
    if (entity_id.size() < 2 || (entity_id[0] != 'P' && entity_id[0] != 'Q')) {
      handler->handle(columns, wd_invalid_t<wd_entity_id_t>{});
      return;
    }
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
    std::string time(time_match[1].str());
    std::optional<iso_time_t> iso8601 = parse_iso8601(time);
    if (!iso8601.has_value()) {
      handler->handle(columns, wd_invalid_t<wd_time_t>{});
      return;
    }

    std::string calendarmodel(time_match[6].str());
    std::uint64_t timezone(std::stoull(time_match[2].str())),
        before(std::stoull(time_match[3].str())),
        after(std::stoull(time_match[4].str())),
        precision(std::stoull(time_match[5].str()));

    handler->handle(columns, wd_time_t{.time = time,
                                       .iso8601 = *iso8601,
                                       .calendermodel = calendarmodel,
                                       .timezone = timezone,
                                       .before = before,
                                       .after = after,
                                       .precision = precision});
  }

private:
  static auto parse_iso8601(std::string &time) -> std::optional<iso_time_t> {
    // NOTE we convert +YYYY-00-00 to YYYY-01-01 to obtain a valid timestamp
    if (time[6] == '0' && time[7] == '0') {
      time[7] = '1';
    }
    if (time[9] == '0' && time[10] == '0') {
      time[10] = '1';
    }
    std::istringstream in{time};
    date::sys_time<std::chrono::milliseconds> tp;
    in >> date::parse("%FT%TZ", tp);
    if (in.fail()) {
      return std::nullopt;
    }
    return tp;
  }

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
        unit_str(quantity_match[2].str()), upper_bound(quantity_match[4].str()),
        lower_bound(quantity_match[6].str());

    if (quantity.size() == 0 || (quantity[0] != '+' && quantity[0] != '-')) {
      handler->handle(columns, wd_invalid_t<wd_quantity_t>{});
      return;
    }

    std::optional<std::string> unit = std::nullopt;
    if (unit_str != "1") {
      std::smatch quantity_unit_match;
      if (!std::regex_match(unit_str, quantity_unit_match,
                            quantity_unit_regex)) {
        std::cerr << "Unexpected quantity string encountered." << std::endl;
        std::cerr << "quantity_str: " << quantity_str << std::endl;
        std::exit(-1);
      }
      unit = quantity_unit_match[1];
    }
    handler->handle(columns, wd_quantity_t{.quantity = quantity,
                                           .unit = unit,
                                           .lower_bound = lower_bound,
                                           .upper_bound = upper_bound});
  }

private:
  static const inline std::regex quantity_regex = std::regex(
      "\\{\"amount\"=>\"([^\"]*?)\", \"unit\"=>\"([^\"]*?)\"(, "
      "\"upperBound\"=>\"([^\"]*?)\")?(, \"lowerBound\"=>\"([^\"]*?)\")?\\}");

  static const inline std::regex quantity_unit_regex =
      std::regex("^http://www.wikidata.org/entity/(.*)$");
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

template <typename tag, typename result_handler,
          typename parser = wd_primitives_parser>
class wikidata_parser_impl {
  using columns_type = columns_info_t<tag>;

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

template <typename tag, typename result_handler>
using wikidata_parser = detail::wikidata_parser_impl<tag, result_handler>;
} // namespace wd_migrate

#endif // !PARSER_WIKIDATA_PARSER_H
