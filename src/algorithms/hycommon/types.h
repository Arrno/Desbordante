#include <memory>
#include <utility>
#include <vector>
#ifdef SIMD
#include <immintrin.h>

#include <boost/align/aligned_allocator.hpp>
#endif

namespace util {

class PositionListIndex;

}

namespace algos::hy {

// Row (or column) position in the table
using TablePos = unsigned int;
using ClusterId = unsigned int;

// Represents a relation as a list of position list indexes. i-th PLI is a PLI built on i-th column
// of the relation
using PLIs = std::vector<util::PositionListIndex*>;
using PLIsPtr = std::shared_ptr<PLIs>;
#ifdef SIMD
using Row = std::vector<TablePos, boost::alignment::aligned_allocator<TablePos, alignof(__m256i)>>;
#else
using Row = std::vector<TablePos>;
#endif
// Represents a relation as a list of rows where each row is a list of row values
using Rows = std::vector<Row>;
// Represents a relation as a list of column where each column is a list of column values
using Columns = std::vector<std::vector<TablePos>>;
using RowsPtr = std::shared_ptr<Rows>;
// Pair of row numbers
using IdPairs = std::vector<std::pair<TablePos, TablePos>>;

}  // namespace algos::hy
