#pragma once
#include "common.hpp"
#include <deque>
#include <regex>
namespace snuw {
inline const std::vector<std::pair<std::string,std::string>> promptSlots={{"char","character_prompt"},{"scene","scene_prompt"},{"clothing","clothing_prompt"},{"style","style_prompt"},{"other","other_prompt"},{"free","prompt"}};
struct PromptSpan {size_t begin=0,end=0;std::wstring text;};
struct PromptDocument {std::wstring source;std::map<std::string,PromptSpan> slots;std::wstring freeText;std::vector<PromptSpan> unclassified;};
inline PromptDocument parsePromptDocument(const std::wstring& text,bool strict=true){
 if(text.size()>65536)throw std::runtime_error("프롬프트는 65,536자 이내로 입력하세요.");
 static const std::wregex tags(LR"(\[(/?)([a-z_]+)\])",std::regex::icase);
 struct Tag {size_t begin,end;bool close;std::string name;};std::vector<Tag> tokens;
 for(std::wsregex_iterator it(text.begin(),text.end(),tags),end;it!=end;++it){auto& m=*it;size_t begin=m.position();if(begin&&text[begin-1]==L'\\')continue;auto name=utf8(m[2].str());std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});tokens.push_back({begin,begin+size_t(m.length()),m[1].length()!=0,name});}
 auto known=[](const std::string& name){return std::any_of(promptSlots.begin(),promptSlots.end(),[&](auto& pair){return pair.first==name;});};
 bool active=strict||std::any_of(tokens.begin(),tokens.end(),[&](auto& t){return t.close&&known(t.name);});
 PromptDocument doc;doc.source=text;if(!active){doc.freeText=text;doc.unclassified.push_back({0,text.size(),text});return doc;}
 std::string opened;size_t contentBegin=0,cursor=0;std::set<std::string> unknown;
 for(auto& tag:tokens){
  if(opened=="free"&&!(tag.close&&tag.name=="free"))continue;
  if(!known(tag.name)){if(tag.close&&unknown.count(tag.name))throw std::runtime_error("알 수 없는 프롬프트 항목입니다.");if(!tag.close)unknown.insert(tag.name);continue;}
  if(!tag.close){if(!opened.empty()||doc.slots.count(tag.name))throw std::runtime_error("프롬프트 항목이 중첩되거나 중복됐습니다.");auto free=text.substr(cursor,tag.begin-cursor);doc.freeText+=free;if(!free.empty())doc.unclassified.push_back({cursor,tag.begin,free});opened=tag.name;contentBegin=tag.end;}
  else {if(opened.empty()||opened!=tag.name)throw std::runtime_error("프롬프트 항목의 닫는 표시를 확인하세요.");doc.slots[opened]={contentBegin,tag.begin,text.substr(contentBegin,tag.begin-contentBegin)};opened.clear();cursor=tag.end;}
 }
 if(!opened.empty())throw std::runtime_error("프롬프트 항목의 닫는 표시가 없습니다.");auto tail=text.substr(cursor);doc.freeText+=tail;if(!tail.empty())doc.unclassified.push_back({cursor,text.size(),tail});return doc;
}
inline std::wstring legacyPromptText(const PromptDocument& doc){auto spans=doc.unclassified;if(doc.slots.count("free"))spans.push_back(doc.slots.at("free"));std::sort(spans.begin(),spans.end(),[](auto& a,auto& b){return a.begin<b.begin;});std::wstring result;for(auto& span:spans)result+=span.text;return result;}
inline std::wstring replacePromptSlot(const std::wstring& text,const std::string& name,const std::wstring& value){
 auto doc=parsePromptDocument(text);auto found=doc.slots.find(name);std::wstring result;
 if(found!=doc.slots.end())result=text.substr(0,found->second.begin)+value+text.substr(found->second.end);
 else result=text+(text.empty()?L"":L"\n")+L"["+wide(name)+L"]"+value+L"[/"+wide(name)+L"]";
 parsePromptDocument(result);return result;
}
inline std::wstring composePromptDocument(const json& drafts){
 auto free=wide(str(drafts,"prompt"));bool details=false;for(auto& pair:promptSlots)if(pair.first!="free"&&!str(drafts,pair.second).empty())details=true;
 if(!details)return free;
 std::wstring result=free.empty()?L"":L"[free]"+free+L"[/free]";
 for(auto& [name,key]:promptSlots)if(name!="free"&&!str(drafts,key).empty()){if(!result.empty())result+=L"\n";result+=L"["+wide(name)+L"]"+wide(str(drafts,key))+L"[/"+wide(name)+L"]";}
 parsePromptDocument(result);return result;
}
inline std::wstring promptDocumentView(const json& state,int tab){
 auto drafts=state.value("drafts",json::object());if(tab>0){static const std::vector<std::string> keys={"prompt","character_prompt","clothing_prompt","scene_prompt","style_prompt","other_prompt"};return wide(str(drafts,keys.at(tab)));}
 auto source=wide(str(drafts,"prompt"));if(state.value("_prompt_linked",false))return source;
 try{if(!parsePromptDocument(source,false).slots.empty())return source;return composePromptDocument(drafts);}catch(...){return source;}
}
inline void applyPromptEdit(json& state,const std::string& key,const std::wstring& value){
 auto previous=state.value("drafts",json::object());bool linked=state.value("_prompt_linked",false);auto previousError=str(state,"_prompt_error");
 if(!linked&&state.value("_prompt_document_version",0)==0&&!state.contains("_prompt_legacy_backup"))state["_prompt_legacy_backup"]={{"drafts",previous},{"linked",false},{"error",previousError},{"document_version",0}};
 state["drafts"][key]=utf8(value); // Invalid drafts must remain recoverable, too.
 try{
  std::wstring source;
  if(key=="prompt")source=value;
  else {
   if(!previousError.empty())throw std::runtime_error("통합 프롬프트의 항목 구분을 먼저 수정하세요.");
   auto old=wide(str(previous,"prompt"));auto parsed=parsePromptDocument(old,false);
   if(linked||!parsed.slots.empty()){
    auto slot=std::find_if(promptSlots.begin(),promptSlots.end(),[&](auto& row){return row.second==key;});if(slot==promptSlots.end())throw std::runtime_error("알 수 없는 프롬프트 항목입니다.");
    if(parsed.slots.empty())source=composePromptDocument(state["drafts"]);else source=replacePromptSlot(old,slot->first,value);
   }else source=composePromptDocument(state["drafts"]);
  }
  auto parsed=parsePromptDocument(source,key=="prompt"&&(state.value("_prompt_document_version",0)==1||!previousError.empty()));if(!parsed.slots.empty())state["_prompt_document_version"]=1;for(auto& [slot,field]:promptSlots)if(slot!="free"){auto found=parsed.slots.find(slot);state["drafts"][field]=found==parsed.slots.end()?"":utf8(found->second.text);}
  state["drafts"]["prompt"]=utf8(source);state["_prompt_linked"]=true;state.erase("_prompt_error");
 }catch(const std::exception& e){state["_prompt_linked"]=true;state["_prompt_error"]=e.what();throw;}
}
inline json wirePromptDrafts(const json& state){
 auto drafts=state.value("drafts",json::object());if(!state.value("_prompt_linked",false))return drafts;
 if(!str(state,"_prompt_error").empty())throw std::runtime_error(str(state,"_prompt_error"));
 auto doc=parsePromptDocument(wide(str(drafts,"prompt")),state.value("_prompt_document_version",0)==1);
 auto free=legacyPromptText(doc);
 drafts["prompt"]=utf8(free);
 for(auto& [name,key]:promptSlots)if(name!="free")drafts[key]=doc.slots.count(name)?utf8(doc.slots.at(name).text):"";
 return drafts;
}
inline json promptSnapshot(const json& state){return json{{"drafts",state.value("drafts",json::object())},{"linked",state.value("_prompt_linked",false)},{"error",str(state,"_prompt_error")},{"document_version",state.value("_prompt_document_version",0)}};}
inline void restorePromptSnapshot(json& state,const json& snapshot){state["drafts"]=snapshot["drafts"];state["_prompt_linked"]=snapshot["linked"];state["_prompt_document_version"]=snapshot.value("document_version",0);if(str(snapshot,"error").empty())state.erase("_prompt_error");else state["_prompt_error"]=snapshot["error"];}
inline json legacyPromptExport(const json& state){auto copy=state;copy["drafts"]=wirePromptDrafts(state);copy["project"]["prompt_sections"]=copy["drafts"];copy["_prompt_linked"]=false;copy["_prompt_document_version"]=0;copy.erase("_prompt_error");copy.erase("_prompt_legacy_backup");return copy;}
class PromptHistory {
 struct Entry{json data;size_t bytes;};std::deque<Entry> past,future;std::string scope;
 void trim(){size_t bytes=0;for(auto& e:past)bytes+=e.bytes;for(auto& e:future)bytes+=e.bytes;while(past.size()+future.size()>100||bytes>4*1024*1024){auto& list=past.empty()?future:past;if(list.empty())break;bytes-=list.front().bytes;list.pop_front();}}
public:
 void bind(const std::string& key){if(scope!=key){scope=key;past.clear();future.clear();}}
 void record(const json& snapshot){future.clear();if(past.empty()||past.back().data!=snapshot)past.push_back({snapshot,snapshot.dump().size()});trim();}
 bool restore(json& state,bool redo){auto& from=redo?future:past;auto& to=redo?past:future;if(from.empty())return false;auto current=promptSnapshot(state);to.push_back({current,current.dump().size()});auto previous=from.back().data;from.pop_back();restorePromptSnapshot(state,previous);trim();return true;}
};
}
