#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "model/table/tuple_index.h"

namespace algos::order {

class SortedPartition {
public:
    using EquivalenceClass = boost::unordered_flat_set<model::TupleIndex>;
    using EquivalenceClasses = std::vector<EquivalenceClass>;
    using PartitionIndex = unsigned long;
    using HashProduct = std::unordered_map<PartitionIndex, SortedPartition::EquivalenceClasses>;

private:
    EquivalenceClasses sorted_partition;
    std::unordered_map<model::TupleIndex, PartitionIndex> hash_partition;
    unsigned long num_rows = 0;

    void BuildHashTable();
    HashProduct BuildHashProduct(SortedPartition const& other);

public:
    SortedPartition() = default;
    explicit SortedPartition(unsigned long num_rows) noexcept : num_rows(num_rows){};
    SortedPartition(EquivalenceClasses&& eq_classes, unsigned long num_rows)
        : sorted_partition(std::move(eq_classes)), num_rows(num_rows){};
    void Intersect(SortedPartition const& other);

    EquivalenceClasses const& GetEqClasses() const {
        return sorted_partition;
    }

    std::size_t Size() const {
        return sorted_partition.size();
    }
};

}  // namespace algos::order
