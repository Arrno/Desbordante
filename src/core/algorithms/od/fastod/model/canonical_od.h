#pragma once

#include <memory>

#include "algorithms/od/fastod/hashing/hashing.h"
#include "algorithms/od/fastod/od_ordering.h"
#include "algorithms/od/fastod/storage/partition_cache.h"
#include "attribute_pair.h"

namespace algos::fastod {

template <od::Ordering Ordering>
class CanonicalOD {
private:
    AttributeSet context_;
    AttributePair ap_;

public:
    CanonicalOD() noexcept = default;
    CanonicalOD(AttributeSet const& context, model::ColumnIndex left, model::ColumnIndex right);

    bool IsValid(std::shared_ptr<DataFrame> data, PartitionCache& cache) const;
    std::string ToString() const;

    friend bool operator==(CanonicalOD<od::Ordering::ascending> const& x,
                           CanonicalOD<od::Ordering::ascending> const& y);
    friend bool operator!=(CanonicalOD<od::Ordering::ascending> const& x,
                           CanonicalOD<od::Ordering::ascending> const& y);
    friend bool operator<(CanonicalOD<od::Ordering::ascending> const& x,
                          CanonicalOD<od::Ordering::ascending> const& y);
    friend bool operator==(CanonicalOD<od::Ordering::descending> const& x,
                           CanonicalOD<od::Ordering::descending> const& y);
    friend bool operator!=(CanonicalOD<od::Ordering::descending> const& x,
                           CanonicalOD<od::Ordering::descending> const& y);
    friend bool operator<(CanonicalOD<od::Ordering::descending> const& x,
                          CanonicalOD<od::Ordering::descending> const& y);

    friend struct std::hash<CanonicalOD<Ordering>>;
};

using AscCanonicalOD = CanonicalOD<od::Ordering::ascending>;
using DescCanonicalOD = CanonicalOD<od::Ordering::descending>;

class SimpleCanonicalOD {
private:
    AttributeSet context_;
    model::ColumnIndex right_;

public:
    SimpleCanonicalOD();
    SimpleCanonicalOD(AttributeSet const& context, model::ColumnIndex right);

    bool IsValid(std::shared_ptr<DataFrame> data, PartitionCache& cache) const;
    std::string ToString() const;

    friend bool operator==(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator!=(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator<(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);

    friend struct std::hash<SimpleCanonicalOD>;
};

}  // namespace algos::fastod

namespace std {

template <algos::od::Ordering Ordering>
struct hash<algos::fastod::CanonicalOD<Ordering>> {
    size_t operator()(algos::fastod::CanonicalOD<Ordering> const& od) const noexcept {
        size_t const context_hash = hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const ap_hash = hash<algos::fastod::AttributePair>{}(od.ap_);

        return algos::fastod::hashing::CombineHashes(context_hash, ap_hash);
    }
};

template <>
struct hash<algos::fastod::SimpleCanonicalOD> {
    size_t operator()(algos::fastod::SimpleCanonicalOD const& od) const noexcept {
        size_t const context_hash = hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const right_hash = hash<model::ColumnIndex>{}(od.right_);

        return algos::fastod::hashing::CombineHashes(context_hash, right_hash);
    }
};

}  // namespace std
