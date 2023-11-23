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

template <typename derived, const char *field_name>
struct wd_column_parser : wd_datavalue_type_parser<derived> {
  template <typename columns_type>
  static auto parse_row(const columns_type &columns) {
    return derived::parse(columns.template get_field<field_name>());
  }
};

template <typename derived>
using wd_datavalue_string_parser = wd_column_parser<derived, kDatavalueString>;

template <typename derived>
using wd_datavalue_entity_parser = wd_column_parser<derived, kDatavalueEntity>;

struct wd_fallback_parser {
public:
  template <typename columns_type>
  static auto can_parse(const columns_type &columns) -> bool {
    return true;
  }
  template <typename columns_type>
  static auto parse(const columns_type &columns) -> void {
    std::cerr << "Unexpected datavalue_type encountered."
              << std::string(20, ' ') << std::endl;
    std::cerr << "datavalue_type: "
              << columns.template get_field<kDatavalueType>() << std::endl;
    std::exit(-1);
  }
};

struct wd_string_parser
    : public detail::wd_datavalue_string_parser<wd_string_parser> {
public:
  static const inline std::string kTypeIdentifier = "string";
  static auto parse(const std::string &str) -> void {
    // NOTE "parsing" the string type is trivial.
  }
};

struct wd_entity_parser
    : public detail::wd_datavalue_entity_parser<wd_entity_parser> {
public:
  static const inline std::string kTypeIdentifier = "wikibase-entityid";
  static auto parse(const std::string &entity_id) -> void {
    // NOTE "parsing" the entity_id is trivial.
  }
};

struct wd_text_parser
    : public detail::wd_datavalue_string_parser<wd_text_parser> {
public:
  static const inline std::string kTypeIdentifier = "monolingualtext";

  static auto parse(const std::string &text_str) -> void {
    if (text_str == "novalue") {
      // TODO(jlscheerer) Increase a counter for this event.
      return;
    }
    std::smatch text_match;
    if (!std::regex_match(text_str, text_match, text_regex)) {
      std::cerr << "Unexpected text string encountered." << std::endl;
      std::cerr << "text_str: " << text_str << std::endl;
      std::exit(-1);
    }
    // std::string text(text_match[1].str()), language(text_match[2].str());
    // std::cout << text << " " << language << std::endl;
  }

private:
  // {"text"=>"The Arms of George Washington", "language"=>"en"}
  static const inline std::regex text_regex =
      std::regex("^\\{\"text\"=>\"(.*?)\", \"language\"=>\"([^\"]*?)\"\\}$");
};

struct wd_time_parser
    : public detail::wd_datavalue_string_parser<wd_time_parser> {
public:
  static const inline std::string kTypeIdentifier = "time";

  static auto parse(const std::string &time_str) {
    if (time_str == "novalue") {
      // TODO(jlscheerer) Increase a counter for this event.
      return;
    }
    std::smatch time_match;
    if (!std::regex_match(time_str, time_match, time_regex)) {
      std::cerr << "Unexpected time string encountered." << std::endl;
      std::cerr << "time_str: " << time_str << std::endl;
      std::exit(-1);
    }

    // NOTE For calendarmodel we have (Q1985727 = "Gregorian Calendar", >99%)
    //      and (Q1985786 = "Julian Calendar")
    std::string time(time_match[1].str()), calendarmodel(time_match[6].str());

    // NOTE >99.9% of values have timezone = 0. The others are 1 and 60.
    std::uint64_t timezone(std::stoull(time_match[2].str())),
        before(std::stoull(time_match[3].str())),
        after(std::stoull(time_match[4].str())),
        precision(std::stoull(time_match[5].str()));
    // std::cout << time_str << std::endl;
    // std::cout << time << ' ' << timezone << ' ' << before << ' ' << after <<
    // '
    // '
    //           << precision << ' ' << calendarmodel << std::endl;
  }

private:
  // {"time"=>"+2023-09-13T00:00:00Z", "timezone"=>0, "before"=>0, "after"=>0,
  // "precision"=>11,
  // "calendarmodel"=>"http://www.wikidata.org/entity/Q1985727"}
  static const inline std::regex time_regex = std::regex(
      "^\\{\"time\"=>\"([^\"]*?)\", \"timezone\"=>(\\d+), \"before\"=>(\\d+), "
      "\"after\"=>(\\d+), \"precision\"=>(\\d+).*, "
      "\"calendarmodel\"=>\"http://www.wikidata.org/entity/([^\"]*?)\"\\}$");
};

struct wd_quantity_parser
    : public detail::wd_datavalue_string_parser<wd_quantity_parser> {
public:
  static const inline std::string kTypeIdentifier = "quantity";
  static auto parse(const std::string &quantity_str) -> void {
    std::smatch quantity_match;
    if (quantity_str == "novalue") {
      // TODO(jlscheerer) Increase a counter for this event.
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
    // std::cout << quantity_str << std::endl;
    // std::cout << quantity << ' ' << unit << ' ' << upper_bound << ' '
    //           << lower_bound << std::endl;
  }

private:
  // {"amount"=>"+50", "unit"=>"http://www.wikidata.org/entity/Q39369"}
  // {"amount"=>"-3.54", "unit"=>"http://www.wikidata.org/entity/Q11573"}
  // {"amount"=>"+57613", "unit"=>"1"}
  // novalue | snaktype = somevalue
  static const inline std::regex quantity_regex = std::regex(
      "\\{\"amount\"=>\"([^\"]*?)\", \"unit\"=>\"([^\"]*?)\"(, "
      "\"upperBound\"=>\"([^\"]*?)\")?(, \"lowerBound\"=>\"([^\"]*?)\")?\\}");
};

struct wd_coordinate_parser
    : public detail::wd_datavalue_string_parser<wd_coordinate_parser> {
public:
  static const inline std::string kTypeIdentifier = "globecoordinate";
  static auto parse(const std::string &coordinate_str) -> void {
    if (coordinate_str == "novalue") {
      // TODO(jlscheerer) Increase a counter for this event.
      return;
    }
    std::smatch coordinate_match;
    if (!std::regex_match(coordinate_str, coordinate_match, coordinate_regex)) {
      std::cerr << "Unexpected coordinate string encountered." << std::endl;
      std::cerr << "coordinate_str: " << coordinate_str << std::endl;
      std::exit(-1);
    }
    // {"latitude"=>38.70661, "longitude"=>-77.08723, "altitude"=>nil,
    // "precision"=>0.000277778, "globe"=>"http://www.wikidata.org/entity/Q2"}
    std::string latitude(coordinate_match[1].str()),
        longitude(coordinate_match[2].str()),
        altitude(coordinate_match[3].str()),
        precision(coordinate_match[4].str()), globe(coordinate_match[5].str());

    // std::cout << coordinate_str << std::endl;
    // std::cout << latitude << ' ' << longitude << ' ' << precision << ' ' <<
    // globe
    //           << std::endl;
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
  template <typename columns_type>
  static auto parse_row(const columns_type &columns) {
    detail::wd_fallback_parser::parse(columns);
  }
};

template <typename head, typename... tail>
struct wd_combined_parser<head, tail...> {
public:
  template <typename columns_type>
  static auto parse_row(const columns_type &columns) {
    if (head::can_parse(columns)) {
      head::parse_row(columns);
    } else {
      wd_combined_parser<tail...>::parse_row(columns);
    }
  }
};

using wd_primitives_parser =
    wd_combined_parser<wd_string_parser, wd_entity_parser, wd_time_parser,
                       wd_coordinate_parser, wd_quantity_parser,
                       wd_text_parser>;

template <typename parser, typename columns_type> class wikidata_parser_impl {
public:
  wikidata_parser_impl(const std::string &filename) : filename_(filename) {}
  auto parse() -> void {
    io::CSVReader<columns_type::size(), io::trim_chars<' '>,
                  io::no_quote_escape<'\t'>>
        reader(filename_);
    utils::progress_indicator progress("parsing " + filename_);
    progress.start();
    while (columns_.read_row(reader)) {
      parser::parse_row(columns_);
      progress.update();
    }
    progress.done();
  }

protected:
  const std::string filename_;

  columns_type columns_;
};
} // namespace detail
} // namespace wd_migrate

#endif // !PARSER_WIKIDATA_PARSER_H
