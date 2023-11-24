#include <ios>
#include <iostream>
#include <string_view>

#include "fast-cpp-csv-parser/csv.h"
#include "handler/csv_handler.h"
#include "handler/wikidata_handler.h"
#include "parser/wikidata_columns.h"
#include "parser/wikidata_parser.h"
#include "utils/progress_indicator.h"

auto print_usage(const std::string_view binary) -> int {
  std::cerr << "usage: " << binary << " [claims|qualifiers] <filename> <output>"
            << std::endl;
  return -1;
}

template <typename tag, typename result_handler>
auto parse_wikidata(const std::string_view filename, result_handler &handler)
    -> void {
  wd_migrate::wikidata_parser<tag, result_handler> parser;
  parser.parse(std::string(filename), &handler);
  handler.summary();
}

auto main(int argc, char **argv) -> int {
  using namespace wd_migrate;
  if (argc <= 3) {
    return print_usage(argv[0]);
  }
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::string_view file_type(argv[1]);
  if (file_type == "claims") {
    auto handler = stats_handler</*print_illegal_values=*/true>();
    parse_wikidata<claims_tag_t>(argv[2], handler);
  } else if (file_type == "qualifiers") {
    auto handler = stacked_handler(
        stats_handler</*print_illegal_values=*/false>(),
        quantity_scale_handler(), csv_handler<qualifiers_tag_t>(argv[3]));
    parse_wikidata<qualifiers_tag_t>(argv[2], handler);
  } else {
    return print_usage(argv[0]);
  }
  return 0;
}
