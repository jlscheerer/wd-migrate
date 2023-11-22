#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>

#include "fast-cpp-csv-parser/csv.h"

static const std::string kDatavalueTypeString = "string";
static const std::string kDatavalueTypeEntityId = "wikibase-entityid";

static const std::string kDatavalueTypeTime = "time";
// {"time"=>"+2023-09-13T00:00:00Z", "timezone"=>0, "before"=>0, "after"=>0,
// "precision"=>11, "calendarmodel"=>"http://www.wikidata.org/entity/Q1985727"}
std::regex time_regex(
    "^\\{\"time\"=>\"([^\"]*?)\", \"timezone\"=>(\\d+), \"before\"=>(\\d+), "
    "\"after\"=>(\\d+), \"precision\"=>(\\d+).*, "
    "\"calendarmodel\"=>\"http://www.wikidata.org/entity/([^\"]*?)\"\\}$");

static const std::string kDatavalueTypeCoordinate = "globecoordinate";
// {"latitude"=>38.70661, "longitude"=>-77.08723, "altitude"=>nil,
// "precision"=>0.000277778, "globe"=>"http://www.wikidata.org/entity/Q2"}
static std::regex
    coordinate_regex("\\{\"latitude\"=>([^,]*?), \"longitude\"=>([^,]*?), "
                     "\"altitude\"=>([^,]*?), \"precision\"=>([^,]*?), "
                     "\"globe\"=>\"([^\"]*?)\"\\}");

static const std::string kDatavalueTypeQuantity = "quantity";
// {"amount"=>"+50", "unit"=>"http://www.wikidata.org/entity/Q39369"}
// {"amount"=>"-3.54", "unit"=>"http://www.wikidata.org/entity/Q11573"}
// {"amount"=>"+57613", "unit"=>"1"}
// novalue | snaktype = somevalue
static std::regex quantity_regex(
    "\\{\"amount\"=>\"([^\"]*?)\", \"unit\"=>\"([^\"]*?)\"(, "
    "\"upperBound\"=>\"([^\"]*?)\")?(, \"lowerBound\"=>\"([^\"]*?)\")?\\}");

static const std::string kDatavalueTypeText = "monolingualtext";
// {"text"=>"The Arms of George Washington", "language"=>"en"}
std::regex
    text_regex("^\\{\"text\"=>\"(.*?)\", \"language\"=>\"([^\"]*?)\"\\}$");

auto parse_time(const std::string &snaktype, const std::string &time_str)
    -> void {
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
  std::string time(time_match[1].str()), calendarmodel(time_match[6].str());
  std::uint64_t timezone(std::stoull(time_match[2].str())),
      before(std::stoull(time_match[3].str())),
      after(std::stoull(time_match[4].str())),
      precision(std::stoull(time_match[5].str()));
  // std::cout << time_str << std::endl;
  // std::cout << time << ' ' << timezone << ' ' << before << ' ' << after << '
  // '
  //           << precision << ' ' << calendarmodel << std::endl;
}

auto parse_coordinate(const std::string &coordinate_str) -> void {
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
      longitude(coordinate_match[2].str()), altitude(coordinate_match[3].str()),
      precision(coordinate_match[4].str()), globe(coordinate_match[5].str());

  // std::cout << coordinate_str << std::endl;
  // std::cout << latitude << ' ' << longitude << ' ' << precision << ' ' <<
  // globe
  //           << std::endl;
}

auto parse_quantity(const std::string &snaktype,
                    const std::string &quantity_str) -> void {
  std::smatch quantity_match;
  if (quantity_str == "novalue") {
    // TODO(jlscheerer) Increase a counter for this event.
    return;
  }
  if (!std::regex_match(quantity_str, quantity_match, quantity_regex)) {
    std::cerr << "Unexpected quantity string encountered." << std::endl;
    std::cerr << "quantity_str: " << quantity_str << " snaktype: " << snaktype
              << std::endl;
    std::exit(-1);
  }
  std::string quantity(quantity_match[1].str()), unit(quantity_match[2].str()),
      upper_bound(quantity_match[4].str()),
      lower_bound(quantity_match[6].str());
  // std::cout << quantity_str << std::endl;
  // std::cout << quantity << ' ' << unit << ' ' << upper_bound << ' '
  //           << lower_bound << std::endl;
}

auto parse_text(const std::string &text_str) -> void {
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

auto main() -> int {
  io::CSVReader<13, io::trim_chars<' '>, io::no_quote_escape<'\t'>> reader(
      "data/qualifiers1M.csv");
  std::string claim_id, property, hash, snaktype, qualifier_property,
      datavalue_string, datavalue_entity, datavalue_date, unused,
      datavalue_type, datatype;
  std::uint64_t counter, order_hash;

  while (reader.read_row(claim_id, property, hash, snaktype, qualifier_property,
                         datavalue_string, datavalue_entity, datavalue_date,
                         unused, datavalue_type, datatype, counter,
                         order_hash)) {
    if (datavalue_type == kDatavalueTypeString) {
      const std::string &str = datavalue_string;
    } else if (datavalue_type == kDatavalueTypeEntityId) {
      const std::string &entity_id = datavalue_entity;
    } else if (datavalue_type == kDatavalueTypeTime) {
      parse_time(snaktype, datavalue_string);
    } else if (datavalue_type == kDatavalueTypeCoordinate) {
      parse_coordinate(datavalue_string);
    } else if (datavalue_type == kDatavalueTypeQuantity) {
      parse_quantity(snaktype, datavalue_string);
    } else if (datavalue_type == kDatavalueTypeText) {
      parse_text(datavalue_string);
    } else {
      std::cerr << "Unexpected datavalue_type encountered." << std::endl;
      std::cerr << "datavalue_type: " << datavalue_type << std::endl;
      std::exit(-1);
    }
  }
  return 0;
}
