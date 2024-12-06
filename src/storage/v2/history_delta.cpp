#include "storage/v2/history_delta.hpp"
#include "query/db_accessor.hpp"
#include <cstring>

#include "utils/flag_validation.hpp"
#include "utils/settings.hpp"
#include <json/json.hpp>
#include "query/serialization/property_value.hpp"
namespace history_delta {

namespace {
enum class ObjectType : uint8_t { MAP, TEMPORAL_DATA };
}  // namespace

nlohmann::json SerializePropertyValueVector(const std::vector<storage::PropertyValue> &values);

nlohmann::json SerializePropertyValueMap(const std::map<std::string, storage::PropertyValue> &parameters);

nlohmann::json SerializePropertyValue(const storage::PropertyValue &property_value) {
  using Type = storage::PropertyValue::Type;
  switch (property_value.type()) {
    case Type::Null:
      return {};
    case Type::Bool:
      return property_value.ValueBool();
    case Type::Int:
      return property_value.ValueInt();
    case Type::Double:
      return property_value.ValueDouble();
    case Type::String:
      return property_value.ValueString();
    case Type::List:
      return SerializePropertyValueVector(property_value.ValueList());
    case Type::Map:
      return SerializePropertyValueMap(property_value.ValueMap());
    case Type::TemporalData:
      const auto temporal_data = property_value.ValueTemporalData();
      auto data = nlohmann::json::object();
      data.emplace("type", static_cast<uint64_t>(ObjectType::TEMPORAL_DATA));
      data.emplace("value", nlohmann::json::object({{"type", static_cast<uint64_t>(temporal_data.type)},
                                                    {"microseconds", temporal_data.microseconds}}));
      return data;
    default:
      return {};
  }
}

nlohmann::json SerializePropertyValueVector(const std::vector<storage::PropertyValue> &values) {
  nlohmann::json array = nlohmann::json::array();
  for (const auto &value : values) {
    array.push_back(SerializePropertyValue(value));
  }
  return array;
}

nlohmann::json SerializePropertyValueMap(const std::map<std::string, storage::PropertyValue> &parameters) {
  nlohmann::json data = nlohmann::json::object();
  nlohmann::json data_value = nlohmann::json::object();
  data.emplace("type", static_cast<uint64_t>(ObjectType::MAP));

  for (const auto &[key, value] : parameters) {
    data_value[key] = SerializePropertyValue(value);
  }

  data.emplace("value", data_value);

  return data;
};


//help functions
 bool TemporalCheck(uint64_t object_ts,uint64_t object_te,uint64_t c_ts,uint64_t c_te,const query::TemporalQueryType& type){
  switch (type) {
    case query::TemporalQueryType::AS_OF:
      return object_ts<=c_ts & object_te>c_te;
    case query::TemporalQueryType::FROM_TO:
      return object_ts<c_te & object_te>c_ts;
    case query::TemporalQueryType::BETWEEN_AND:
      return object_ts<=c_te & object_te>c_ts;
    default:
      return false;
  }
}

std::vector<std::string> splits(const std::string &str, const std::string &pattern){
    std::vector<std::string> res;
    if (str == "")
        return res;

    std::string strs = str + pattern;
    size_t pos = strs.find(pattern);
    while (pos != strs.npos)
    {
      std::string temp = strs.substr(0, pos);
      res.push_back(temp);
      //Remove and splits the remaining string
      strs = strs.substr(pos + 1, strs.size());
      pos = strs.find(pattern);
    }
    return res;
}

std::vector<std::string> split(const std::string &s, const char delim) {
   std::vector<std::string> tokens;
   std::istringstream token_stream(s);

   while (!token_stream.eof()) {
     std::string x;
     std::getline(token_stream,x,delim);

     tokens.push_back(x);
   }

   return tokens;
}


int64_t swap64(const int64_t &v)
{
  return (v >> 56 & 0x00000000000000ff)
    | ((v & 0x00ff000000000000) >> 40 & 0x0000000000ffffff)
    | ((v & 0x0000ff0000000000) >> 24 & 0x000000ffffffffff)
    | ((v & 0x000000ff00000000) >> 8 & 0x00ffffffffffffff)
    | ((v & 0x00000000ff000000) << 8)
    | ((v & 0x0000000000ff0000) << 24)
    | ((v & 0x000000000000ff00) << 40)
    | (v << 56);
}

std::string uint_convert_to_string(const int64_t time){
    auto size=sizeof(time);
    char *buffer = (char *)std::malloc(size);
    auto reversed_time=swap64(time);
    std::memcpy(buffer, &reversed_time, size);
    std::string start_str(buffer,size);
    std::free(buffer);
    return start_str;
}

std::string formatPrefix(const std::string& segmentPrefix, const storage::Gid& gid, const uint64_t tt_start, const uint64_t tt_end, const storage::TemporalPeriod vt)
{
   std::string result=segmentPrefix;
   constexpr std::string_view delimiter(":");

   const std::string gid_str = std::to_string(gid.AsUint());
   const std::string tt_start_str = uint_convert_to_string(static_cast<int64_t>(-tt_start));
   const std::string tt_end_str = uint_convert_to_string(static_cast<int64_t>(-tt_end));
   const std::string vt_start_str = uint_convert_to_string(vt.first.get_microseconds());
   const std::string vt_end_str = uint_convert_to_string(vt.second.get_microseconds());

   result.reserve(delimiter.size() * 4 + tt_start_str.size() + tt_end_str.size() + vt_start_str.size() + vt_end_str.size() + segmentPrefix.size() + gid_str.size());

   //SEGMENT GID : TT_s : TT_e : VT_s : VT_e
   result.append(std::to_string(gid.AsUint()));
   result.append(delimiter);
   result.append(tt_start_str);
   result.append(delimiter);
   result.append(tt_end_str);
   result.append(delimiter);
   result.append(vt_start_str);
   result.append(delimiter);
   result.append(vt_end_str);

   return result;
}

std::string formatPartialPrefix(const std::string& segmentPrefix, const storage::Gid& gid, const std::optional<uint64_t> tt, bool flip_tt = true)
{
   std::string prefix=segmentPrefix;
   constexpr std::string_view delimiter(":");

   const std::string gid_str = std::to_string(gid.AsUint());

   if (tt) {
     const std::string tt_start_str = uint_convert_to_string(static_cast<int64_t>(flip_tt ? -*tt : *tt));

     prefix.reserve(delimiter.size() * 1 + tt_start_str.size()  + segmentPrefix.size() + gid_str.size());

     //SEGMENT GID : TT_s : TT_e : VT_s : VT_e
     prefix.append(gid_str);
     prefix.append(delimiter);
     prefix.append(tt_start_str);
   }else {
     prefix.reserve(segmentPrefix.size() + gid_str.size());

     prefix.append(gid_str);
   }

   return prefix;
}

std::string formatVT(const storage::TemporalPeriod& vt) {
   std::string result;
   constexpr std::string_view delimiter(":");

   const std::string vt_start_str = uint_convert_to_string(vt.first.get_microseconds());
   const std::string vt_end_str = uint_convert_to_string(vt.second.get_microseconds());

   result.reserve(delimiter.size() * 1 + vt_start_str.size() + vt_start_str.size());

   result.append(vt_start_str);
   result.append(delimiter);
   result.append(vt_end_str);

   return result;
}

storage::TemporalPeriod parseFormattedVT(const std::string& formatted_vt) {
   utils::VTDateTime vt_start = utils::VTDateTime::min();
   utils::VTDateTime vt_end = utils::VTDateTime::max();

   std::vector<std::string> res_split = split(formatted_vt, ':');

   const auto vt_ts = swap64(*reinterpret_cast<int64_t *>(&res_split[4]));
   const auto vt_te = swap64(*reinterpret_cast<int64_t *>(&res_split[5]));

   vt_start = utils::VTDateTime(vt_ts);
   vt_end = utils::VTDateTime(vt_te);

   return {vt_start, vt_end};
}

std::tuple<uint64_t,int64_t,int64_t, storage::TemporalPeriod> string_convert_to_uint(std::string res){
    constexpr size_t size64=sizeof(int64_t);
    const size_t length=res.length();

    std::vector<std::string> res_split = split(res,':');
   //Res split:
   // 0   Segment Prefix
   // 1   Gid
   // 2   TT start
   // 3   TT end
   // 4   VT start
   // 5   VT end
   // count = 6

    static_assert(!res_split.empty());

    //1.get gid
    const auto gid_uint = static_cast<uint64_t>(std::stoi(res_split[1]));

   //2.get TT
   const auto tt_ts = swap64(*reinterpret_cast<int64_t *>(&res_split[2]));
   const auto tt_te = swap64(*reinterpret_cast<int64_t *>(&res_split[3]));

   //3.get VT
   utils::VTDateTime vt_start = utils::VTDateTime::min();
   utils::VTDateTime vt_end = utils::VTDateTime::max();
   if (res_split.size() == 6) {
     const auto vt_ts = swap64(*reinterpret_cast<int64_t *>(&res_split[4]));
     const auto vt_te = swap64(*reinterpret_cast<int64_t *>(&res_split[5]));

     vt_start = utils::VTDateTime(vt_ts);
     vt_end = utils::VTDateTime(vt_te);
   }
   storage::TemporalPeriod vt(vt_start, vt_end);

   return std::make_tuple(gid_uint, tt_ts, tt_te, vt);

    // //get gid
    // const size_t pos = res.find(':');
    // std::string gid_str=res.substr(pos+1,length-2*size64-3-pos);//3:4 12 20-2*8
    // auto gid=static_cast<uint64_t>(std::stoi(gid_str));
    //
    // //get times
    // const std::string redo_str1=res.substr(length-size64);//12:
    // const std::string redo_str2=res.substr(length-2*size64-1,size64);//3:12
    //
    // char redo[size64];
    // char redo2[size64];
    // for(int i=0;i<size64;i++){
    //     redo[i]=redo_str1[i];
    //     redo2[i]=redo_str2[i];
    // }
    // auto ts = *reinterpret_cast<int64_t *>(redo2);// redo_str;
    // auto te = *reinterpret_cast<int64_t *>(redo);
    // ts=swap64(ts);
    // te=swap64(te);
    //
    // return std::make_tuple(gid,ts,te);
}

void combineVertex(nlohmann::json before_data,nlohmann::json &current_data){
  for (auto it = before_data.begin(); it != before_data.end(); ++it) {
    auto& it_key = it.key();
    auto it_value=it.value();
    auto it_iter=current_data.find(it_key);
    if(it_iter==current_data.end()){   //If the current data does not have this type, add it directly
      current_data.emplace(it_key,it_value);
    }else{
    //Otherwise, the value of json needs to be merged
      if(it_key=="SP"){
        auto now_prop_value = current_data["SP"];
        auto& before_prop_value=it_value;
        for(auto before_iter= before_prop_value.begin(); before_iter != before_prop_value.end(); ++before_iter){
          auto before_iter_key = before_iter.key();
          auto before_iter_value=before_iter.value();
          auto bb=now_prop_value.find(before_iter_key);
          if(bb==now_prop_value.end()){
            current_data["SP"].emplace(before_iter_key,before_iter_value);
          }
        }
      }
      else if(it_key=="L"){
        auto &current_info=current_data[it_key];
        for(const auto& before_label:before_data[it_key]){
          current_info.emplace_back(before_label);
        }  
      }
      else if(it_key=="VT") {
        auto &current_vt = current_data[it_key];
        for(const auto& before_vt:before_data[it_key]) {
          current_vt.emplace_back(before_vt);
        }
      }
    }
  }
}

// void combineVertexVT(nlohmann::json before_data, nlohmann::json &current_data, const storage::TemporalPeriod delta_vt, const storage::TemporalPeriod filter_vt, bool before_vt, utils::interval<std::string> prefixs) {
//    std::string this_prefix = formatVT(delta_vt);
//    prefixs.add(delta_vt.get_pair(), this_prefix);
//
//    if (before_vt) {
//
//    }
// }

void combineEdge(nlohmann::json before_data,nlohmann::json &current_data){
  for (auto it = before_data.begin(); it != before_data.end(); ++it) {
     auto edge_id = it.key();
     if(edge_id=="TT_TS" || edge_id=="TT_TE" ||edge_id=="Type" ||edge_id=="Fid"||edge_id=="Tid") continue;
     auto edge_jsons=it.value();
     auto it_iter=current_data.find(edge_id);
    if(it_iter==current_data.end()){//If the current data does not have information about this node, add it directly.
      current_data.emplace(edge_id,edge_jsons);
    }else{
      combineVertex(before_data[edge_id],current_data[edge_id]);
    }
  }
}


const std::string kDeltaPrefix = "D:";
const std::string kRecreatePrefix = "R:";

const std::string kVertexDeltaPrefix = "VD:";
const std::string kVertexAnchorPrefix = "VA:";
const std::string kEdgeDeltaPrefix = "ED:";
const std::string kEdgeAnchorPrefix = "EA:";
const std::string kVertexEdgePrefix = "VE:";

const std::string kVertexTimePrefix="VT:";
const std::string kEdgeTimePrefix="ET:";


HistoryDelta::HistoryDelta(const std::string &storage_directory) : storage_(storage_directory) {}

HistoryDelta::HistoryDelta(const std::string &storage_directory,bool realTimeFlag) : storage_(storage_directory) {
  realTimeFlagConstant=realTimeFlag;
}



std::pair<std::vector<nlohmann::json>,bool> HistoryDelta::GetEdgeInfo(uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type,uint64_t gid) {
  std::vector<nlohmann::json> history_Delta;
  bool anchor_flag=false;
  auto tmp_info=nlohmann::json::object();

  //1、Find the nearest record in VA(VE - anchor) segment
  std::string anchor_prefix=kEdgeAnchorPrefix+std::to_string(gid)+":"+uint_convert_to_string(static_cast<int64_t>(c_te));
  auto iter_begin=storage_.starts(anchor_prefix);
  auto iter_end=storage_.last(anchor_prefix);

  std::string prefixs=kEdgeDeltaPrefix+std::to_string(gid);
  auto vd_iter_begin=storage_.starts(prefixs);
  auto vd_iter_end=storage_.last(prefixs);//null
  bool need_combine=true;

  while(iter_begin!=iter_end){//1.2. found in VA, filter VD (VE - anchor) segment
    std::string key=iter_begin->first;
    std::vector<std::string> parts = split(key, ':');
    if(parts[1] != std::to_string(gid)){
      ++iter_begin;
      break;
    }

    anchor_flag=true;
    tmp_info=nlohmann::json::parse(iter_begin->second);

    int64_t va_ts=std::get<1>(string_convert_to_uint(key));
    if(va_ts>=c_te){
      va_ts=va_ts>0?-va_ts:va_ts;

      std::string va_ts_str=uint_convert_to_string(va_ts);
      std::string delta_prefix=kEdgeDeltaPrefix+std::to_string(gid)+":"+va_ts_str;

      vd_iter_begin=storage_.starts(delta_prefix);
      vd_iter_end=storage_.last(delta_prefix);
      break;
    }else  anchor_flag=false;
    ++iter_begin;
  }

  for(;vd_iter_begin!=vd_iter_end;++vd_iter_begin){
    auto [egde_gid,ts,te,vt]=string_convert_to_uint(vd_iter_begin->first);
    auto object_ts=static_cast<uint64_t>(-ts);//TT version start time
    auto object_te=static_cast<uint64_t>(-te);//TT version end time

    if(gid!=egde_gid)
      break;
    if(object_te<c_ts)
      break;

    auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node
    if(need_combine){
      combineVertex(tmp_info,current_info);
      tmp_info=current_info;
    }
    if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
      need_combine=false;
      history_Delta.emplace_back(current_info);
      if(type==query::TemporalQueryType::AS_OF) break;
    }
  } 
  return std::make_pair(history_Delta,anchor_flag);
}

std::pair<std::vector<nlohmann::json>,bool> HistoryDelta::GetEdgeInfo(storage::Gid gid, uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type, const query::TemporalFilter& vt_filter) {
  std::vector<nlohmann::json> history_Delta;
  bool anchor_flag=false;
  auto tmp_info=nlohmann::json::object();

  //1、Find the nearest record in VA(VE - anchor) segment
  std::string anchor_prefix= formatPartialPrefix(kEdgeAnchorPrefix, gid, c_te, false);
    //kEdgeAnchorPrefix+std::to_string(gid)+":"+uint_convert_to_string(static_cast<int64_t>(c_te));

  auto iter_begin=storage_.starts(anchor_prefix);
  auto iter_end=storage_.last(anchor_prefix);

  std::string gid_str = std::to_string(gid.AsUint());

  std::string prefixs=formatPartialPrefix(kEdgeDeltaPrefix, gid, std::nullopt, false);
    //kEdgeDeltaPrefix+std::to_string(gid);
  auto vd_iter_begin=storage_.starts(prefixs);
  auto vd_iter_end=storage_.last(prefixs);//null
  bool need_combine=true;

  while(iter_begin!=iter_end){//1.2. found in VA, filter VD (VE - anchor) segment
    std::string key=iter_begin->first;
    std::vector<std::string> parts = split(key, ':');
    if(parts[1] != gid_str){
      ++iter_begin;
      break;
    }

    anchor_flag=true;
    tmp_info=nlohmann::json::parse(iter_begin->second);

    int64_t va_ts=std::get<1>(string_convert_to_uint(key));
    if(va_ts>=c_te){
      std::string delta_prefix= formatPartialPrefix(kEdgeDeltaPrefix, gid, va_ts, va_ts>0);
        //kEdgeDeltaPrefix+std::to_string(gid)+":"+va_ts_str;

      vd_iter_begin=storage_.starts(delta_prefix);
      vd_iter_end=storage_.last(delta_prefix);
      break;
    }
    anchor_flag=false;
    ++iter_begin;
  }

  for(;vd_iter_begin!=vd_iter_end;++vd_iter_begin){
    auto [egde_gid,ts,te,vt]=string_convert_to_uint(vd_iter_begin->first);
    auto object_ts=static_cast<uint64_t>(-ts);//TT version start time
    auto object_te=static_cast<uint64_t>(-te);//TT version end time

    if(gid.AsUint()!=egde_gid)
      break;
    if(object_te<c_ts)
      break;
    if(!vt_filter.matches(vt.first, vt.second))
      break;

    auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node
    if(need_combine){
      combineVertex(tmp_info,current_info);
      tmp_info=current_info;
    }
    if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
      need_combine=false;
      history_Delta.emplace_back(current_info);
      if(type==query::TemporalQueryType::AS_OF) break;
    }
  }
  return std::make_pair(history_Delta,anchor_flag);
}


void HistoryDelta::GetTimeTableAll(){
  for(auto it=storage_.starts(kVertexTimePrefix);it!=storage_.last(kVertexTimePrefix);++it){
    auto gid=static_cast<uint64_t>(std::stoi(it->first.substr(3)));
    std::vector<std::string> split_info=split(it->second,':');
    auto min_ts=static_cast<uint64_t>(std::stoi(split_info[0]));
    auto max_te=static_cast<uint64_t>(std::stoi(split_info[1]));
    vertex_time_table_[gid]=std::make_pair(min_ts,max_te);
  }

  //save edge time table
  for(auto it=storage_.starts(kEdgeTimePrefix);it!=storage_.last(kEdgeTimePrefix);++it){
    auto gid=static_cast<uint64_t>(std::stoi(it->first.substr(3)));
    std::vector<std::string> split_info=split(it->second,':');
    auto min_ts=static_cast<uint64_t>(std::stoi(split_info[0]));
    auto max_te=static_cast<uint64_t>(std::stoi(split_info[1]));
    edge_time_table_[gid]=std::make_pair(min_ts,max_te);
  }
}

std::pair<std::vector<nlohmann::json>,bool> HistoryDelta::GetVertexInfo(storage::Gid gid,uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type){
    std::vector<nlohmann::json> history_Delta;
    bool anchor_flag=false;
    uint64_t vertx_gid=gid.AsUint();//vertex Gid
    auto tmp_info=nlohmann::json::object();
    std::string anchor_prefix=kVertexAnchorPrefix+std::to_string(vertx_gid)+":"+uint_convert_to_string(static_cast<int64_t>(c_te));
    auto iter_begin=storage_.starts(anchor_prefix);//seek the first record that meets the time condition and is greater or equal than the current time
    auto iter_end=storage_.last(anchor_prefix);//null

    //1.1. Not found in VA, find data from latest VD
    std::string prefixs=kVertexDeltaPrefix+std::to_string(vertx_gid)+":"+uint_convert_to_string(static_cast<int64_t>(-c_te));
    auto vd_iter_begin=storage_.starts(prefixs);
    auto vd_iter_end=storage_.last(prefixs);//null
    bool need_combine=true;

    if(iter_begin!=iter_end){//1.2. Found in VA, filter VD data segment
        std::string key=iter_begin->first;
        std::vector<std::string> parts = split(key, ':');
        if(parts[1] != std::to_string(vertx_gid)){
            anchor_flag=false;
        }else{
            anchor_flag=true;

            int64_t va_ts=(std::get<1>(string_convert_to_uint(key)));
            if(va_ts>=c_te){
                tmp_info=nlohmann::json::parse(iter_begin->second);
                va_ts=va_ts>0?-va_ts:va_ts;

                std::string va_ts_str=uint_convert_to_string(va_ts);
                std::string delta_prefix=kVertexDeltaPrefix+std::to_string(vertx_gid)+":"+va_ts_str;

                vd_iter_begin=storage_.starts(delta_prefix);
                vd_iter_end=storage_.last(delta_prefix);//null
            }
        }
    }

    //2、get delta data
    for(;vd_iter_begin!=vd_iter_end;++vd_iter_begin){
        auto [gid,ts,te,vt]=string_convert_to_uint(vd_iter_begin->first);
        auto object_ts=static_cast<uint64_t>(-ts);//TT version start time
        auto object_te=static_cast<uint64_t>(-te);//TT version end time

        if(gid!=vertx_gid)
          break;
        if(object_te<c_ts)
          break;

        auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node
        if(need_combine){
            combineVertex(tmp_info,current_info);
            tmp_info=current_info;
        }
        if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
            need_combine=false;
            history_Delta.emplace_back(current_info);
            if(type==query::TemporalQueryType::AS_OF) break;
        }
    }
    return std::make_pair(history_Delta,anchor_flag);
}

std::pair<std::vector<nlohmann::json>,bool> HistoryDelta::GetVertexInfo(storage::Gid gid,uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type, const query::TemporalFilter& vt_filter){
  std::vector<nlohmann::json> history_Delta;
  bool anchor_flag=false, multiple_vts = false, tmp_vt = false;

  uint64_t vertx_gid=gid.AsUint();//vertex Gid
  auto tmp_info=nlohmann::json::object();
  std::string anchor_prefix= formatPartialPrefix(kVertexAnchorPrefix,gid, c_te, false);
    //kVertexAnchorPrefix+std::to_string(vertx_gid)+":"+uint_convert_to_string(static_cast<int64_t>(c_te));
  auto iter_begin=storage_.starts(anchor_prefix);//seek the first record that meets the time condition and is greater or equal than the current time
  auto iter_end=storage_.last(anchor_prefix);//null

  //1.1. Not found in VA, find data from latest VD
  std::string prefixs= formatPartialPrefix(kVertexDeltaPrefix,gid, c_te, true);
    //kVertexDeltaPrefix+std::to_string(vertx_gid)+":"+uint_convert_to_string(static_cast<int64_t>(-c_te));
  auto vd_iter_begin=storage_.starts(prefixs);
  auto vd_iter_end=storage_.last(prefixs);//null
  bool need_combine=true;

  if(iter_begin!=iter_end){//1.2. Found in VA, filter VD data segment
      std::string key=iter_begin->first;
      std::vector<std::string> parts = split(key, ':');
      if(parts[1] != std::to_string(vertx_gid)){
          anchor_flag=false;
      }else{
          anchor_flag=true;

          int64_t va_ts=(std::get<1>(string_convert_to_uint(key)));
          if(va_ts>=c_te){
              tmp_info=nlohmann::json::parse(iter_begin->second);

              std::string va_ts_str=uint_convert_to_string(va_ts);
              std::string delta_prefix= formatPartialPrefix(kVertexDeltaPrefix,gid, va_ts, va_ts>0);
                //kVertexDeltaPrefix+std::to_string(vertx_gid)+":"+va_ts_str;

              vd_iter_begin=storage_.starts(delta_prefix);
              vd_iter_end=storage_.last(delta_prefix);//null
          }
      }
  }

  //2、get delta data
  for(;vd_iter_begin!=vd_iter_end; ++vd_iter_begin){
    auto [gid,ts,te,vt] = string_convert_to_uint(vd_iter_begin->first);
    auto object_ts = static_cast<uint64_t>(-ts);//TT version start time
    auto object_te = static_cast<uint64_t>(-te);//TT version end time

    if(gid!=vertx_gid)
      break;
    if(object_te<c_ts)
      break;
    if(!vt_filter.matches(vt.first, vt.second))
      break;

    multiple_vts |= !vt_filter.get_period().included(vt);

    auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node

    if(need_combine){
        combineVertex(tmp_info,current_info);
        tmp_info=current_info;
    }

    // if (need_combine && multiple_vts) {
    //   combineVertexVT(tmp_info,current_info,vt,vt_filter.get_period(), tmp_vt);
    //   tmp_info=current_info;
    //   tmp_vt = true;
    // }

    if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
        need_combine=false;
        history_Delta.emplace_back(current_info);
        if(type==query::TemporalQueryType::AS_OF) break;
    }
  }
  return std::make_pair(history_Delta,anchor_flag);
}

std::pair<std::vector< std::tuple< std::map<storage::PropertyId,storage::PropertyValue>,uint64_t,uint64_t> >,bool> getDeadInfo2(query::VertexAccessor current_vertex_, const uint64_t c_ts, const uint64_t c_te,query::TemporalQueryType types_){
  std::vector<std::tuple< std::map<storage::PropertyId,storage::PropertyValue>,uint64_t,uint64_t>> res;
  storage::Delta* vertex_deltas=current_vertex_.getDeltas();
  auto need_deleted_flag=true;

  if(vertex_deltas==nullptr){
    return std::make_pair(res,need_deleted_flag);
  }

  auto maybe_properties=current_vertex_.impl_.getProperties();
  bool delta_is_edge=false;

  while (vertex_deltas != nullptr) {
    delta_is_edge=false;
    switch (vertex_deltas->action) {
      case storage::Delta::Action::ADD_OUT_EDGE:
      case storage::Delta::Action::REMOVE_OUT_EDGE: 
      case storage::Delta::Action::ADD_IN_EDGE:
      case storage::Delta::Action::REMOVE_IN_EDGE:{
        delta_is_edge=true;
        break;
      }
      case storage::Delta::Action::SET_PROPERTY:{
        auto property_value=vertex_deltas->property.value;
        auto property_key=vertex_deltas->property.key;
        if(property_value.type()!=storage::PropertyValue::Type::Null) {
          maybe_properties[property_key]=property_value;
        }else{
          maybe_properties[property_key]=storage::PropertyValue("NULL");
        }
      }
      default:break;
    }
    auto transaction_ts=vertex_deltas->transaction_st;
    auto transaction_te=vertex_deltas->commit_timestamp!=0?vertex_deltas->commit_timestamp:std::numeric_limits<uint64_t>::max();

    //Skip uncommitted nodes - Nodes built at the beginning
    if(transaction_ts> transaction_te &&  delta_is_edge && transaction_te!=std::numeric_limits<uint64_t>::max()) {
      vertex_deltas = vertex_deltas->next.load(std::memory_order_acquire);    
      continue;
    }//The data of the vertices is required. delta is the edge.

    if(TemporalCheck(transaction_ts,transaction_te,c_ts,c_te,types_)){//&TemporalCheck(tmp_ts,tmp_te,c_ts,c_te,types)
      res.emplace_back(maybe_properties,transaction_ts,transaction_te);
      if(types_==query::TemporalQueryType::AS_OF) {//If it is a time point, it will be returned directly and there is no need to traverse the records of delted history.
        need_deleted_flag=false;
        break;
      }
    }
    // Move to the next delta.
    vertex_deltas = vertex_deltas->next.load(std::memory_order_acquire);    
  }
  return std::make_pair(res,need_deleted_flag);
}

std::pair<std::vector< std::tuple< std::map<storage::PropertyId,storage::PropertyValue>,uint64_t,uint64_t, storage::TemporalPeriod> >,bool> getDeadInfo2(query::VertexAccessor current_vertex_, const uint64_t c_ts, const uint64_t c_te,query::TemporalQueryType types_, const query::TemporalFilter vt_filter){
  std::vector<std::tuple< std::map<storage::PropertyId,storage::PropertyValue>,uint64_t,uint64_t, storage::TemporalPeriod>> res;
  storage::Delta* vertex_deltas=current_vertex_.getDeltas();
  auto need_deleted_flag=true;

  if(vertex_deltas==nullptr){
    return std::make_pair(res,need_deleted_flag);
  }

  auto maybe_properties=current_vertex_.impl_.getProperties();
  bool delta_is_edge=false;


  while (vertex_deltas != nullptr) {
    storage::TemporalPeriod delta_vt = vertex_deltas->vt;

    if (!vt_filter.matches(vertex_deltas->vt.first, vertex_deltas->vt.second)) {
      vertex_deltas = vertex_deltas->next.load(std::memory_order_acquire);
      continue;
    }

    delta_is_edge=false;
    switch (vertex_deltas->action) {
      case storage::Delta::Action::ADD_OUT_EDGE:
      case storage::Delta::Action::REMOVE_OUT_EDGE:
      case storage::Delta::Action::ADD_IN_EDGE:
      case storage::Delta::Action::REMOVE_IN_EDGE:{
        delta_is_edge=true;
        break;
      }
      case storage::Delta::Action::SET_PROPERTY:{
        auto property_value=vertex_deltas->property.value;
        auto property_key=vertex_deltas->property.key;
        if(property_value.type()!=storage::PropertyValue::Type::Null) {
          maybe_properties[property_key]=property_value;
        }else{
          maybe_properties[property_key]=storage::PropertyValue("NULL");
        }
      }
      default:break;
    }
    auto transaction_ts=vertex_deltas->transaction_st;
    auto transaction_te=vertex_deltas->commit_timestamp!=0?vertex_deltas->commit_timestamp:std::numeric_limits<uint64_t>::max();

    //Skip uncommitted nodes - Nodes built at the beginning
    if(transaction_ts> transaction_te &&  delta_is_edge && transaction_te!=std::numeric_limits<uint64_t>::max()) {
      vertex_deltas = vertex_deltas->next.load(std::memory_order_acquire);
      continue;
    }//The data of the vertices is required. delta is the edge.

    if(TemporalCheck(transaction_ts,transaction_te,c_ts,c_te,types_)){//&TemporalCheck(tmp_ts,tmp_te,c_ts,c_te,types)
      res.emplace_back(maybe_properties,transaction_ts,transaction_te, delta_vt);
      if(types_==query::TemporalQueryType::AS_OF) {//If it is a time point, it will be returned directly and there is no need to traverse the records of delted history.
        need_deleted_flag=false;
        break;
      }
    }
    // Move to the next delta.
    vertex_deltas = vertex_deltas->next.load(std::memory_order_acquire);
  }
  return std::make_pair(res,need_deleted_flag);
}

std::vector<nlohmann::json> HistoryDelta::GetDeleteEdgeInfo(uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type,uint64_t vertex_gid){
    std::vector<nlohmann::json> history_Delta;

    //1. Find data in VE segment
    std::string prefixs=kVertexEdgePrefix+std::to_string(vertex_gid);
    auto vd_iter_begin=storage_.starts(prefixs);
    auto vd_iter_end=storage_.last(prefixs);//null
    bool need_combine=true;
    auto tmp_info=nlohmann::json::object();

    //2、get data
    for(;vd_iter_begin!=vd_iter_end;++vd_iter_begin){
        auto [gid,ts,te,vt]=string_convert_to_uint(vd_iter_begin->first);
        auto object_ts=static_cast<uint64_t>(-ts);//version start time
        auto object_te=static_cast<uint64_t>(-te);//version end time

        if(gid!=vertex_gid)
          break;
        if(object_te<c_ts)
          break;

        auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node
        if(need_combine){
            combineEdge(tmp_info,current_info);
            tmp_info=current_info;
        }
        if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
            need_combine=false;
            history_Delta.emplace_back(current_info);
            if(type==query::TemporalQueryType::AS_OF) break;
        }
    }

    return history_Delta;
}

std::vector<nlohmann::json> HistoryDelta::GetDeleteEdgeInfo(uint64_t c_ts,uint64_t c_te,query::TemporalQueryType type,uint64_t vertex_gid, const query::TemporalFilter& vt_filter){
  std::vector<nlohmann::json> history_Delta;

  //1. Find data in VE segment
  std::string prefixs=kVertexEdgePrefix+std::to_string(vertex_gid);
  auto vd_iter_begin=storage_.starts(prefixs);
  auto vd_iter_end=storage_.last(prefixs);//null
  bool need_combine=true;
  auto tmp_info=nlohmann::json::object();

  //2、get data
  for(;vd_iter_begin!=vd_iter_end;++vd_iter_begin){
    auto [gid,ts,te,vt]=string_convert_to_uint(vd_iter_begin->first);
    auto object_ts=static_cast<uint64_t>(-ts);//version start time
    auto object_te=static_cast<uint64_t>(-te);//version end time

    if(gid!=vertex_gid)
      break;
    if(object_te<c_ts)
      break;
    if(!vt_filter.matches(vt.first,vt.second))
      break;

    auto current_info=nlohmann::json::parse(vd_iter_begin->second);//data of current node
    if(need_combine){
      combineEdge(tmp_info,current_info);
      tmp_info=current_info;
    }
    if(TemporalCheck(object_ts,object_te,c_ts,c_te,type)){
      need_combine=false;
      history_Delta.emplace_back(current_info);
      if(type==query::TemporalQueryType::AS_OF) break;
    }
  }

  return history_Delta;
}

void HistoryDelta::SaveDeltaAll() {
  bool success = false;
  if(gid_delta_.empty()) return;
  std::map<std::string,std::string> gid_data_tmp;
  for(const auto& [key,value]:gid_delta_){
    gid_data_tmp[key]=value.dump();
  }
  success=storage_.PutMultiple(gid_data_tmp);
  if (!success) {
    std::cout<<"Couldn't save delta!"<<std::endl;
  }
  gid_delta_.clear();
}


void HistoryDelta::SaveAnchorAll(std::map<std::string, std::string> &value){
  bool success=storage_.PutMultiple(value);
  if (!success) {
    std::cout<<"Couldn't save delta!"<<std::endl;
  }
}


void HistoryDelta::SaveEdgeAnchorAll(uint64_t tid,std::map<std::string, std::string>& data){
  if(!data.empty())edge_anchor_.emplace_back(data);
}

//storage::Gid gid,const std::optional<storage::Gid> to_gid,const uint64_t start,const uint64_t commit
void HistoryDelta::SaveDelta(storage::Delta& delta,storage::NameIdMapper &name_id_mapper) {
  uint64_t start = delta.transaction_st;
  uint64_t commit = delta.commit_timestamp;
  storage::Gid gid = delta.gid;
  std::optional<storage::Gid> to_gid = delta.to_gid;
  storage::TemporalPeriod vt = delta.vt;


  if(start>commit)
    return;

  bool edge_flag=false;

  //get delta infomation encode delta to string
  nlohmann::json data = nlohmann::json::object();

  switch (delta.action) {
    case storage::Delta::Action::RECREATE_OBJECT: {
      data= delta.add_info;
      if(!to_gid)data["R"]="R";//exclude edges
      break;
    }
    case storage::Delta::Action::SET_PROPERTY: {
      const std::string& property_name = name_id_mapper.IdToName(delta.property.key.AsUint());//delta.property.key.AsUint();//
      auto property_value = SerializePropertyValue(delta.property.value);//query::serialization::
      nlohmann::json data2 = nlohmann::json::object();
      data2[property_name] = property_value;

      data["SP"]=data2;
      break;
    }
    case storage::Delta::Action::ADD_LABEL:{
      auto add_label=std::vector<std::pair<std::string,std::string>>();
      add_label.emplace_back("AL",name_id_mapper.IdToName(delta.label.AsUint()));

      data["L"] = add_label;//"ADD:"+name_id_mapper.IdToName(delta.label.AsUint());
      break;
    }
    case storage::Delta::Action::REMOVE_LABEL: {
      auto remove_labels=std::vector<std::pair<std::string,std::string>>();
      // auto labels=std::vector<uint64_t>();
      // labels.emplace_back(delta.label.AsUint());
      remove_labels.emplace_back("RL",name_id_mapper.IdToName(delta.label.AsUint()));

      data["L"] = remove_labels;//"REMOVE:"+name_id_mapper.IdToName(delta.label.AsUint());
      break;
    }
    case storage::Delta::Action::ADD_OUT_EDGE:
    case storage::Delta::Action::ADD_IN_EDGE:{
      edge_flag=true;

      std::string edge_id=std::to_string((delta.vertex_edge.edge.ptr->gid).AsUint());
      nlohmann::json edge_data = nlohmann::json::object();

      edge_data["Type"]=delta.action==storage::Delta::Action::ADD_OUT_EDGE?"AOE":"AIE";//ADD IN EDGE
      edge_data["edgeType"]=name_id_mapper.IdToName((delta.vertex_edge.edge_type).AsUint());
      edge_data["edgeId"]=(delta.vertex_edge.edge.ptr->gid).AsUint();
      edge_data["fromGid"]=(delta.vertex_edge.edge.ptr->from_gid).AsUint();
      edge_data["toGid"]=(delta.vertex_edge.edge.ptr->to_gid).AsUint();
      data[edge_id]=edge_data;
      break;
    }
    default:
      return;
  }

  if(to_gid) {
    data["Fid"]=(delta.from_gid)->AsUint();
    data["Tid"]=(delta.to_gid)->AsUint();
  }

  std::string prefix=to_gid?kEdgeDeltaPrefix:(edge_flag?kVertexEdgePrefix:kVertexDeltaPrefix);

  //save hash index
  uint64_t vertex_gid=gid.AsUint();

  auto storeInTimeTable = [](auto& time_table,uint64_t gid, uint64_t start, uint64_t commit) {
    auto iter=time_table.find(gid);
    if(iter==time_table.end()){
      time_table[gid]=std::make_pair(start,commit);
    }else{
      auto &[tt_ts, tt_te]=time_table[gid];
      tt_te=commit;
      if(tt_ts==0){
        tt_ts=start;
      }
    }
  };

  if(prefix==kVertexDeltaPrefix){
    storeInTimeTable(vertex_time_table_, vertex_gid, start, commit);
  }

  // //save hash index
  if(prefix==kEdgeDeltaPrefix){
    storeInTimeTable(edge_time_table_, vertex_gid, start, commit);
  }

  std::string start_str=uint_convert_to_string(static_cast<int64_t>(-start));
  std::string commit_str=uint_convert_to_string(static_cast<int64_t>(-commit));

  //prefix + std::to_string(gid.AsUint()) +":"+(start_str)+":"+(commit_str); //std::to_string  std::to_string
  std::string put_key = formatPrefix(prefix, gid, start, commit, vt);

  // union something
  data["TT_TS"]=start;
  data["TT_TE"]=commit;

  data["VT_TS"]=vt.first.get_microseconds();
  data["VT_TE"]=vt.second.get_microseconds();

  auto iter =  gid_delta_.find(put_key);
  if(iter !=  gid_delta_.end()){ 
    auto before_value= gid_delta_[put_key];
    if(edge_flag){//VE
      combineEdge(before_value,data);
    }else{//ED+VD
      combineVertex(before_value,data);
    }
  }
   gid_delta_[put_key]=data;
}

std::string HistoryDelta::getPrefix(storage::Gid gid,const uint64_t start,bool vertex){
  std::string start_str=uint_convert_to_string(static_cast<int64_t>(start));
  std::string prefix=vertex?kVertexAnchorPrefix:kEdgeAnchorPrefix;
  std::string key=prefix + std::to_string(gid.AsUint()) +":"+(start_str)+":"+(start_str); //std::to_string  std::to_string
  return key;
}

bool HistoryDelta::HasDeltas() const { return storage_.begin(kDeltaPrefix) != storage_.end(kDeltaPrefix); }


bool HistoryDelta::RemoveOldHistory(const std::chrono::milliseconds &retention_period) {
  //TODO find old history and delete them
  auto now_time = std::chrono::system_clock::now();
  long now_time_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now_time.time_since_epoch()).count();
  int64_t clean_timestamp = now_time_milliseconds-retention_period.count() ;

  std::vector<std::string> delete_keys;
  for (auto it = storage_.begin(); it != storage_.end(); ++it) {
    auto [gid,ts,te,vt]=string_convert_to_uint(it->first);
    te=te>0?te:-te;
    if(te<=clean_timestamp){
      delete_keys.push_back(it->first);
    }
  }
  if (!storage_.DeleteMultiple(delete_keys)) {
    std::cout<<"Couldn't Remove Old History!\n";
  }
  return true;
}
}  // namespace history_delta
