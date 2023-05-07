#pragma once

#include "hycommon/sampler.h"
#include "hycommon/types.h"
#include "options/thread_number/type.h"
#include "ucc/hyucc/structures/non_ucc_list.h"

namespace algos {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, config::ThreadNumType threads = 1)
        : sampler_(std::move(plis), std::move(pli_records), threads) {}

    NonUCCList GetNonUCCCandidates(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos
