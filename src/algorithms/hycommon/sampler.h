#pragma once

#include <memory>
#include <queue>
#include <vector>

#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>

#include "all_column_combinations.h"
#include "hyfd/hyfd_config.h"
#include "options/thread_number/type.h"
#include "types.h"
#include "util/position_list_index.h"

namespace algos::hy {

class Sampler {
private:
    class Efficiency;
    double efficiency_threshold_ = hyfd::HyFDConfig::kEfficiencyThreshold;

    PLIsPtr plis_;
    RowsPtr compressed_records_;
    std::priority_queue<Efficiency> efficiency_queue_;
    std::unique_ptr<AllColumnCombinations> agree_sets_;
    config::ThreadNumType threads_num_;
    std::unique_ptr<boost::asio::thread_pool> pool_;


    void ProcessComparisonSuggestions(IdPairs const& comparison_suggestions);
    void SortClustersSeq();
    void SortClustersParallel();
    void SortClusters();
    void InitializeEfficiencyQueueSeq();
    void InitializeEfficiencyQueueParallel();
    void InitializeEfficiencyQueueImpl();
    void InitializeEfficiencyQueue();

    void Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
               size_t second_record_id);
    template <typename F>
    void RunWindowImpl(Efficiency& efficiency, util::PositionListIndex const& pli, F store_match);
    std::vector<boost::dynamic_bitset<>> RunWindowRet(Efficiency& efficiency,
                                                      util::PositionListIndex const& pli);
    void RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli);

public:
    Sampler(PLIsPtr plis, RowsPtr pli_records, config::ThreadNumType threads = 1);

    Sampler(Sampler const& other) = delete;
    Sampler(Sampler&& other) = delete;
    Sampler& operator=(Sampler const& other) = delete;
    Sampler& operator=(Sampler&& other) = delete;
    ~Sampler();

    ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hy
