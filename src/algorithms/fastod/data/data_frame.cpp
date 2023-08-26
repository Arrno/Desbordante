#include <vector>
#include <string>
#include <utility>
#include <filesystem>

#include "parser/csv_parser.h"
#include "util/config/equal_nulls/type.h"
#include "model/column_layout_typed_relation_data.h"

#include "data_frame.h"
#include "schema_value.h"
#include "attribute_set.h"

using namespace algos::fastod;

DataFrame::DataFrame(std::vector<model::TypedColumnData> columns_data) noexcept : columns_data_(std::move(columns_data)) {
    assert(columns_data_.size() != 0);
    size_t numRows = columns_data_[0].GetNumRows();
    data_.reserve(numRows);
    for (size_t i = 0; i < numRows; ++i) {
        data_.emplace_back(columns_data_.size());
        for (size_t col = 0; col < columns_data_.size(); ++col) {
            data_.back()[col] = SchemaValue::FromTypedColumnData(columns_data_[col], i);
        }
    }
    ASIterator::MAX_COLS = columns_data_.size();
}

const SchemaValue& DataFrame::GetValue(int tuple_index, int attribute_index) const noexcept {
    return data_[tuple_index][attribute_index];
}

SchemaValue& DataFrame::GetValue(int tuple_index, int attribute_index) noexcept {
    return data_[tuple_index][attribute_index];
}

std::size_t DataFrame::GetColumnCount() const noexcept {
    return columns_data_.size();
}

std::size_t DataFrame::GetTupleCount() const noexcept {
    return columns_data_.size() > 0
        ? columns_data_.at(0).GetNumRows()
        : 0;
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path) {
    return FromCsv(path, ',', true, true);
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path,
                             char separator,
                             bool has_header,
                             util::config::EqNullsType is_null_equal_null) {
    CSVParser parser = CSVParser(path, separator, has_header);
    std::vector<model::TypedColumnData> columns_data = model::CreateTypedColumnData(parser, is_null_equal_null);

    return DataFrame(std::move(columns_data));
}