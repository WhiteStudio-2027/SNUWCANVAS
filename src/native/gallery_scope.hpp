#pragma once
#include "api.hpp"
#include "gallery_files.hpp"
namespace snuw {
inline json galleryViewConfiguration(const json& settings,const json& channels,bool requestedAll,bool allowHardcore=false,const std::string& activeChannel=""){
 json cfg=json::object();for(auto key:{"output","content_mode","_local_account_scope","_local_workspace_root"})if(settings.contains(key))cfg[key]=settings[key];
 bool all=requestedAll&&storageId(str(cfg,"_local_account_scope"),64)&&!str(cfg,"_local_workspace_root").empty();cfg["_gallery_all"]=all;cfg["_gallery_allow_hardcore"]=allowHardcore;cfg["_gallery_active_channel"]=activeChannel.empty()?str(settings,"channel_id"):activeChannel;cfg["_gallery_channels"]=json::array();
 if(channels.is_array())for(auto& channel:channels)cfg["_gallery_channels"].push_back({{"id",str(channel,"id")},{"name",str(channel,"name")},{"output",str(channel.value("state",json::object()).value("settings",json::object()),"output")}});
 if(all){std::set<std::string> roots;auto add=[&](const std::string& value){if(!value.empty())roots.insert(value);};add(str(cfg,"output"));add(utf8((fs::path(wide(str(cfg,"_local_workspace_root")))/"generated").wstring()));json groups=json::array();
  if(channels.is_array())for(auto& channel:channels){auto state=channel.value("state",json::object());add(str(state.value("settings",json::object()),"output"));auto id=str(channel,"id");if(storageId(id,32))groups.push_back({{"channel",id},{"bindings",state.value("_gallery_migrations",json::array())}});}
  cfg["_gallery_roots"]=roots;cfg["_gallery_migration_groups"]=groups;
 }return cfg;
}
inline bool galleryContains(const fs::path& root,const fs::path& file){std::error_code ec;auto base=fs::weakly_canonical(root,ec);if(ec)return false;auto path=fs::weakly_canonical(file,ec);if(ec)return false;auto relative=path.lexically_relative(base);return !relative.empty()&&!relative.is_absolute()&&*relative.begin()!=L"..";}
struct GalleryImageChannel {std::string id;std::wstring name;bool owned=true;};
inline GalleryImageChannel galleryImageChannel(const json& cfg,const fs::path& file,const std::string& migratedChannel="",const fs::path& knownMetadata={}){
 auto catalog=cfg.value("_gallery_channels",json::array());auto scope=str(cfg,"_local_account_scope");
 auto named=[&](const std::string& id,const std::string& fallback=""){for(auto& row:catalog)if(str(row,"id")==id)return GalleryImageChannel{id,wide(str(row,"name","채널"))};return GalleryImageChannel{id,wide(fallback.empty()?"보관 채널 · "+id.substr(0,8):fallback.substr(0,120))};};
 if(!migratedChannel.empty())return named(migratedChannel);
 std::vector<std::wstring> parts;for(auto& part:file)parts.push_back(part.wstring());
 for(size_t i=0;i+5<parts.size();i++)if(parts[i]==L"data"&&storageId(utf8(parts[i+1]),32)&&storageId(utf8(parts[i+2]),32)&&(parts[i+3]==L"safe"||parts[i+3]==L"hardcore")&&parts[i+4]==L"images"){
  if(!scope.empty()&&utf8(parts[i+1])!=scope.substr(0,32))return {"",L"",false};return named(utf8(parts[i+2]));
 }
 json metadata=json::object();try{metadata=readJson(knownMetadata.empty()?imageMetadataPath(file):knownMetadata).value("settings",json::object());}catch(...){}
 auto owner=str(metadata,"_local_account_scope");if(!scope.empty()&&!owner.empty()&&owner!=scope)return {"",L"",false};auto id=str(metadata,"channel_id");if(storageId(id,32))return named(id,str(metadata,"channel_name"));
 for(auto& part:parts)for(auto& row:catalog)if(storageId(str(row,"id"),32)&&utf8(part)==str(row,"id"))return named(str(row,"id"));
 bool legacy=false;fs::path logical;for(auto& part:file.parent_path()){auto name=part.wstring();if(name==L"Standard"||name==L"Hardcore"||name==L"safe"||name==L"hardcore"){legacy=true;continue;}if(name!=L"images")logical/=part;}
 if(legacy){auto name=logical.filename().wstring();std::string key="folder:"+sha256(utf8(logical.lexically_normal().wstring()));return {key,name.empty()?L"이전 채널":name};}
 size_t longest=0;std::string match;for(auto& row:catalog){auto output=str(row,"output");if(!output.empty()&&output.size()>longest&&galleryContains(wide(output),file)){longest=output.size();match=str(row,"id");}}
 if(!match.empty())return named(match);auto active=str(cfg,"_gallery_active_channel");if(!active.empty())return named(active);return {"legacy",L"기존 이미지"};
}
inline std::vector<fs::path> galleryScanRoots(const json& cfg,bool all){
 std::vector<fs::path> roots;auto configured=all?cfg.value("_gallery_roots",json::array()):json::array({str(cfg,"output")});
 if(!configured.is_array())throw std::runtime_error("갤러리 조회 경로를 확인하세요.");
 for(auto& value:configured){if(!value.is_string()||value.get<std::string>().empty())continue;std::error_code ec;auto path=fs::weakly_canonical(wide(value.get<std::string>()),ec);if(ec||!fs::is_directory(path,ec)||!localOutputAllowed(cfg,path))continue;bool duplicate=false;for(auto& root:roots)if(galleryContains(root,path)){duplicate=true;break;}if(duplicate)continue;roots.erase(std::remove_if(roots.begin(),roots.end(),[&](auto& root){return galleryContains(path,root);}),roots.end());roots.push_back(path);}
 return roots;
}
inline fs::path galleryDeletionRoot(const json& cfg,const json& view,const fs::path& file){
 if(!flag(view,"_gallery_all"))return fs::path(wide(str(cfg,"output")));
 if(!localImageVisible(cfg,file))throw std::runtime_error("현재 계정의 이미지만 삭제할 수 있습니다.");auto roots=galleryScanRoots(view,true);bool owned=false;for(auto& root:roots)if(galleryContains(root,file))owned=true;if(!owned)throw std::runtime_error("현재 갤러리 조회 범위의 이미지만 삭제할 수 있습니다.");
 for(auto parent=file.parent_path();!parent.empty()&&parent!=parent.parent_path();parent=parent.parent_path()){bool inside=false;for(auto& root:roots)if(galleryContains(root,parent))inside=true;if(!inside)break;if(fs::is_regular_file(parent/".snuw-output.json")||fs::is_regular_file(parent/".snuw-layout-migration.json"))return parent;}
 return file.parent_path();
}
}
