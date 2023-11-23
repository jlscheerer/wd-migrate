#ifndef PARSER_QUALIFIERS_PARSER_H
#define PARSER_QUALIFIERS_PARSER_H

#include <cstdint>
#include <string_view>
#include <unordered_map>

#include "../fast-cpp-csv-parser/csv.h"
#include "../utils/progress_indicator.h"
#include "wikidata_columns.h"
#include "wikidata_parser.h"

namespace wd_migrate {
namespace detail {
using qualifiers_columns =
    wd_column_pack<wd_column_info<kClaimId, std::string>,
                   wd_column_info<kPropety, std::string>,
                   wd_column_info<kHash, std::string>,
                   wd_column_info<kSnaktype, std::string>,
                   wd_column_info<kQualifierProperty, std::string>,
                   wd_column_info<kDatavalueString, std::string>,
                   wd_column_info<kDatavalueEntity, std::string>,
                   wd_column_info<kDatavalueDate, std::string>,
                   wd_column_info<kNil, std::string>,
                   wd_column_info<kDatavalueType, std::string>,
                   wd_column_info<kDatatype, std::string>,
                   wd_column_info<kCounter, std::uint64_t>,
                   wd_column_info<kOrderHash, std::uint64_t>>;
} // namespace detail

template <typename handler>
using qualifiers_parser =
    detail::wikidata_parser_impl<detail::wd_primitives_parser, handler,
                                 detail::qualifiers_columns>;
} // namespace wd_migrate

#endif // !PARSER_QUALIFIERS_PARSER_H
