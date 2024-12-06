#pragma once

#include <mutex>
#include <optional>
#include <vector>
#include "utils/visitor.hpp"
#include <list>

#include "kvstore/kvstore.hpp"
#include "utils/settings.hpp"
#include "storage/v2/name_id_mapper.hpp"
#include "storage/v2/delta.hpp"
#include <json/json.hpp>
#include <utils/temporal_filter.hpp>

namespace history_delta {

using kv_key = std::pair<uint64_t,uint64_t>;

struct HistoryContext{
  std::map<uint64_t,std::vector<nlohmann::json>> fiter_history_datas_; //data filtered from the KVStore
  std::map<uint64_t,nlohmann::json> fiter_history_delete; //data deleted from the KVStore
  std::map<kv_key,storage::Vertex*> all_vertex_; //historical data map (gid,ts) -> Vertex    历史数据+现有数据的集合map gid,transaction_ts vertex
  uint64_t c_ts; //TT start constraint
  uint64_t c_te; //TT end constraint
  utils::TemporalQueryType types; //type of constraint
  utils::TemporalFilter vt;
};

struct HistoryContextOnce{
    std::vector<storage::LabelId> labels;
    std::vector<kv_key> labels_tt;
    std::map<storage::PropertyId, std::tuple<storage::PropertyValue,uint64_t,uint64_t>> props_tt;
    std::vector<storage::LabelId> remove_labels;
};

class HistoryDelta final {
 public:

   explicit HistoryDelta(const std::string &storage_directory);

   explicit HistoryDelta(const std::string &storage_directory,bool realTimeFlag);

  void GetDelta(const std::string &gid_name) const;

  std::pair<std::vector<nlohmann::json>,bool> GetVertexInfo(storage::Gid gid,uint64_t c_ts,uint64_t c_te,utils::TemporalQueryType type);
  std::pair<std::vector<nlohmann::json>,bool> GetVertexInfo(storage::Gid gid,uint64_t c_ts,uint64_t c_te,utils::TemporalQueryType type, const utils::TemporalFilter& filter);

  std::pair<std::vector<nlohmann::json>,bool> GetEdgeInfo(uint64_t c_ts,uint64_t c_te,utils::TemporalQueryType type,uint64_t gid);
  std::pair<std::vector<nlohmann::json>,bool> GetEdgeInfo(storage::Gid gid, uint64_t c_ts,uint64_t c_te,utils::TemporalQueryType type, const utils::TemporalFilter& filter);

  std::vector<nlohmann::json> GetDeleteEdgeInfo(uint64_t c_ts,uint64_t c_te,utils::TemporalQueryType type,uint64_t gid);

   std::vector<nlohmann::json> GetDeleteEdgeInfo(uint64_t c_ts, uint64_t c_te, utils::TemporalQueryType type,
                                                 uint64_t vertex_gid, const utils::TemporalFilter &vt_filter);

   void GetTimeTableAll();
  void SaveDeltaAll();
  void SaveAnchorAll(std::map<std::string, std::string> &value);

  void SaveDelta(storage::Delta& delta,storage::NameIdMapper &name_id_mapper);
  void SaveVertexAnchor(storage::Gid gid,const uint64_t start,std::vector<storage::LabelId> &labels,std::map<storage::PropertyId, storage::PropertyValue> &maybe_properties,storage::NameIdMapper &name_id_mapper);
  void SaveEdgeAnchor(storage::Gid gid,const uint64_t start,std::map<storage::PropertyId, storage::PropertyValue> &maybe_properties,storage::NameIdMapper &name_id_mapper);
  void SaveVertexAnchor(storage::Gid gid,const uint64_t start,nlohmann::json data);
  void SaveEdgeAnchor(storage::Gid gid,const uint64_t start,nlohmann::json data);
  void SaveEdgeAnchorAll(uint64_t tid,std::map<std::string, std::string>& data);

  bool HasDeltas() const;

  static std::string getPrefix(storage::Gid gid,const uint64_t start,bool vertex);

  bool RemoveOldHistory(const std::chrono::milliseconds &retention_period);

 private:
  bool realTimeFlagConstant=false;

  //hash index used to store objects from min_ts max_te
  std::map<uint64_t,kv_key> vertex_time_table_;//Store the vertex id, the historical start time, and the historical end time
  std::map<uint64_t,kv_key> edge_time_table_;//Store the edge id, historical start time, and historical end time

  //hash index stores for the current transaction only
  std::map<uint64_t,kv_key> vertex_time_tmp_;//Store the vertex id, the historical start time, and the historical end time
  std::map<uint64_t,kv_key> edge_time_tmp_;//Store the edge id, historical start time, and historical end time

  //gid,delta-num: <json>
  std::map<uint64_t,std::pair<int,nlohmann::json>> vertex_anchor_;
  std::list<std::map<std::string, std::string>> edge_anchor_;

  kvstore::KVStore storage_;

  std::map<std::tuple<std::string,uint64_t,uint64_t>, nlohmann::json> gid_delta_delta_;//prefix,gid,commit_te
  std::map<std::string, nlohmann::json> gid_delta_;//save the same actions of one transaction
};
}  // namespace history_delta
