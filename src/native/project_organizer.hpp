#pragma once
#include "common.hpp"
namespace snuw {
inline void prepareProjectOrganizer(json& doc){
 if(!doc.contains("folders"))doc["folders"]=json::array();
 if(!doc["folders"].is_array())throw std::runtime_error("프로젝트 폴더 형식 오류");
 doc["manual_order"]=true;
}
inline bool projectFolderExists(const json& doc,const std::string& key){if(key.empty())return true;if(doc.contains("folders"))for(auto& folder:doc["folders"])if(str(folder,"id")==key)return true;return false;}
inline std::string projectFolder(const json& doc,const json& project){auto folder=str(project,"folder_id");return projectFolderExists(doc,folder)?folder:"";}
inline std::string addProjectFolder(json& doc,const std::string& name){if(name.empty())throw std::runtime_error("폴더 이름을 입력하세요.");prepareProjectOrganizer(doc);auto key=id();doc["folders"].push_back({{"id",key},{"name",name},{"collapsed",false}});return key;}
inline void moveProject(json& doc,const std::string& key,const std::string& folder,const std::string& before=""){
 prepareProjectOrganizer(doc);if(!projectFolderExists(doc,folder))throw std::runtime_error("대상 폴더를 찾을 수 없습니다.");if(key==before)return;
 auto& rows=doc["channels"];auto source=std::find_if(rows.begin(),rows.end(),[&](auto& row){return str(row,"id")==key;});if(source==rows.end())throw std::runtime_error("이동할 프로젝트를 찾을 수 없습니다.");
 if(!before.empty()&&std::none_of(rows.begin(),rows.end(),[&](auto& row){return str(row,"id")==before&&projectFolder(doc,row)==folder;}))throw std::runtime_error("정렬 위치를 찾을 수 없습니다.");
 json moved=*source;moved["folder_id"]=folder;rows.erase(source);auto target=rows.end();if(!before.empty())target=std::find_if(rows.begin(),rows.end(),[&](auto& row){return str(row,"id")==before;});else{for(auto it=rows.begin();it!=rows.end();++it)if(projectFolder(doc,*it)==folder)target=std::next(it);}rows.insert(target,std::move(moved));
}
inline void moveProjectFolder(json& doc,const std::string& key,const std::string& before=""){
 prepareProjectOrganizer(doc);if(key==before)return;auto& folders=doc["folders"];auto source=std::find_if(folders.begin(),folders.end(),[&](auto& row){return str(row,"id")==key;});if(source==folders.end())return;
 if(!before.empty()&&std::none_of(folders.begin(),folders.end(),[&](auto& row){return str(row,"id")==before;}))return;auto moved=*source;folders.erase(source);auto target=std::find_if(folders.begin(),folders.end(),[&](auto& row){return str(row,"id")==before;});folders.insert(target,std::move(moved));
}
inline void removeProjectFolder(json& doc,const std::string& key){prepareProjectOrganizer(doc);for(auto& row:doc["channels"])if(str(row,"folder_id")==key)row["folder_id"]="";auto& folders=doc["folders"];folders.erase(std::remove_if(folders.begin(),folders.end(),[&](auto& row){return str(row,"id")==key;}),folders.end());}
struct ProjectSidebarRow {std::string key,folder;bool isFolder=false,pinned=false;std::wstring name;Rect rect;};
inline std::vector<ProjectSidebarRow> projectSidebarRows(const json& doc,std::wstring query){
 std::transform(query.begin(),query.end(),query.begin(),::towlower);auto matches=[&](std::wstring name){std::transform(name.begin(),name.end(),name.begin(),::towlower);return name.find(query)!=std::wstring::npos;};std::vector<ProjectSidebarRow> result;
 auto append=[&](const std::string& folder,bool all){for(auto& row:doc["channels"])if(!flag(row,"closed")&&projectFolder(doc,row)==folder&&(all||matches(wide(str(row,"name")))))result.push_back({str(row,"id"),folder,false,flag(row,"pinned"),wide(str(row,"name")),{}});};
 if(doc.contains("folders"))for(auto& folder:doc["folders"]){auto key=str(folder,"id");auto start=result.size();result.push_back({key,key,true,false,wide(str(folder,"name")),{}});if(query.empty()){if(!flag(folder,"collapsed"))append(key,true);}else{append(key,matches(wide(str(folder,"name"))));if(result.size()==start+1&&!matches(wide(str(folder,"name"))))result.pop_back();}}
 append("",false);return result;
}
}
