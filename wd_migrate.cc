#include <ios>
#include <iostream>

#include "fast-cpp-csv-parser/csv.h"
#include "handler/csv_handler.h"
#include "handler/wikidata_handler.h"
#include "parser/qualifiers_parser.h"
#include "utils/progress_indicator.h"

auto main(int argc, char **argv) -> int {
  using namespace wd_migrate;
  if (argc <= 2) {
    std::cout << "usage: " << argv[0] << " <filename> <output>" << std::endl;
    return -1;
  }
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  constexpr stats_handler_options options{.print_illegal_values = false};
  auto handler = stacked_handler(
      stats_handler<options>(), quantity_scale_handler(), csv_handler(argv[2]));
  qualifiers_parser<decltype(handler)> parser;
  parser.parse(argv[1], &handler);

  handler.summary();
  return 0;
}
