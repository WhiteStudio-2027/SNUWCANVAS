#pragma once
#include "gallery_scope.hpp"
#include "gallery_migration.hpp"
#include "gallery_layout.hpp"
#include <optional>
namespace snuw {
struct GalleryFileCheck {uint64_t stamp=0;uintmax_t bytes=0;bool valid=false;};
struct GalleryScanResult {
 std::string context,migrationKey,error;uint64_t serial=0;bool changed=false,migrationWarning=false;
 std::vector<fs::path> paths;std::vector<GalleryChannelGroup> groups;std::map<fs::path,GalleryFileCheck> checks;std::map<fs::path,fs::path> migrations;
};
inline GalleryScanResult collectGallery(const json& cfg,const PanelGalleryFilter& filter,const std::wstring& query,const json& bindings,const std::string& scope,const std::string& channel,const std::string& context,uint64_t serial,const std::map<fs::path,GalleryFileCheck>& previous,const std::string& migrationKey,const std::map<fs::path,fs::path>& migrations,const std::atomic_bool& cancel,std::atomic_size_t& examined){
 GalleryScanResult out;out.context=context;out.serial=serial;bool hc=str(cfg,"content_mode")=="hardcore",all=flag(cfg,"_gallery_all")&&storageId(scope,64)&&scope==str(cfg,"_local_account_scope");auto check=[&]{if(cancel)throw std::runtime_error("cancelled");};
 auto groups=all?cfg.value("_gallery_migration_groups",json::array()):json::array({{{"channel",channel},{"bindings",bindings}}});
 try{out.migrationKey=scope+(all?"all":channel)+(hc?"hc":"safe")+(all&&flag(cfg,"_gallery_allow_hardcore")?"both":"mode")+groups.dump();for(auto& group:groups)for(auto& binding:group.value("bindings",json::array())){check();auto root=fs::path(wide(str(binding,"target")));if(root.root_name().wstring().starts_with(L"\\\\"))throw std::runtime_error("remote migration");std::error_code ec;auto stamp=fs::last_write_time(root/".snuw-layout-migration.json",ec);out.migrationKey+=ec?"missing":std::to_string(stamp.time_since_epoch().count());}if(out.migrationKey==migrationKey)out.migrations=migrations;else for(auto& group:groups){auto files=migrationGalleryFiles(group.value("bindings",json::array()),scope,str(group,"channel"),hc||(all&&flag(cfg,"_gallery_allow_hardcore")));out.migrations.insert(files.begin(),files.end());}}catch(...){check();out.migrationWarning=true;out.migrations.clear();}
 std::vector<fs::path> candidates;std::error_code ec;
 for(auto& folder:galleryScanRoots(cfg,all)){
  std::vector<std::pair<fs::path,unsigned>> pending{{folder,0}};
  while(!pending.empty()){check();auto [directory,depth]=pending.back();pending.pop_back();WIN32_FIND_DATAW row{};HANDLE handle=FindFirstFileExW(extendedWindowsPath(directory/L"*").c_str(),FindExInfoBasic,&row,FindExSearchNameMatch,nullptr,0);
   if(handle==INVALID_HANDLE_VALUE){auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND||error==ERROR_ACCESS_DENIED)continue;throw std::runtime_error("이미지 폴더를 읽을 수 없습니다. 경로와 권한을 확인하세요.");}
   struct FindGuard{HANDLE handle;~FindGuard(){FindClose(handle);}} guard{handle};
   do{check();if(wcscmp(row.cFileName,L".")==0||wcscmp(row.cFileName,L"..")==0)continue;auto file=directory/row.cFileName;
    if(row.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(depth<32&&!(row.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))pending.push_back({file,depth+1});}
    else if(!(row.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)&&imagePath(file)&&localImageVisible(cfg,file))candidates.push_back(file);
    if(candidates.size()>100000)throw std::runtime_error("이미지 폴더에 파일이 너무 많습니다. 폴더를 나눈 뒤 다시 불러오세요.");
   }while(FindNextFileW(handle,&row));if(GetLastError()!=ERROR_NO_MORE_FILES)throw std::runtime_error("이미지 목록을 끝까지 읽지 못했습니다. 다시 불러오세요.");
  }
 }
 for(auto& [file,root]:out.migrations)candidates.push_back(file);if(candidates.size()>100000)throw std::runtime_error("이미지 폴더에 파일이 너무 많습니다. 폴더를 나눈 뒤 다시 불러오세요.");std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
 Com<IWICImagingFactory> wic;check();if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic))))throw std::runtime_error("이미지 목록 검사기를 준비하지 못했습니다.");
 std::map<fs::path,std::string> migrationChannels;std::map<fs::path,GalleryImageChannel> channels;std::map<fs::path,fs::path> metadataFolders;std::map<fs::path,bool> hardcoreFolders;
 for(auto& file:candidates){check();examined++;WIN32_FILE_ATTRIBUTE_DATA attributes{};if(!GetFileAttributesExW(extendedWindowsPath(file).c_str(),GetFileExInfoStandard,&attributes)||(attributes.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;std::string imported;if(out.migrations.count(file)){try{migrationSafePath(file);auto root=out.migrations.at(file);if(!migrationChannels.count(root))migrationChannels[root]=str(readJson(root/".snuw-layout-migration.json"),"channel");imported=migrationChannels[root];}catch(...){continue;}}auto parent=file.parent_path();if(!metadataFolders.count(parent))metadataFolders[parent]=imageMetadataPath(file).parent_path();auto metadata=metadataFolders.at(parent)/file.filename();metadata.replace_extension(L".json");auto channelInfo=galleryImageChannel(cfg,file,imported,metadata);if(!channelInfo.owned)continue;if(!all&&!str(cfg,"_gallery_active_channel").empty()&&channelInfo.id!=str(cfg,"_gallery_active_channel"))continue;uint64_t stamp=(uint64_t(attributes.ftLastWriteTime.dwHighDateTime)<<32)|attributes.ftLastWriteTime.dwLowDateTime;uintmax_t bytes=(uintmax_t(attributes.nFileSizeHigh)<<32)|attributes.nFileSizeLow;GalleryFileCheck checked{stamp,bytes,false};auto old=previous.find(file);
  if(old!=previous.end()&&old->second.stamp==stamp&&old->second.bytes==bytes)checked=old->second;else{out.changed|=old!=previous.end();Com<IWICBitmapDecoder> decoder;checked.valid=SUCCEEDED(wic->CreateDecoderFromFilename(extendedWindowsPath(file).c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder));}out.checks[file]=checked;
  if(!hardcoreFolders.count(parent)){try{hardcoreFolders[parent]=structuredHardcoreImage(file);}catch(...){hardcoreFolders[parent]=true;}}bool adult=hardcoreFolders.at(parent);if(!adult){try{adult=str(readJson(localImageMarker(cfg,file)),"content_mode")=="hardcore";}catch(...){}if(!adult)try{adult=str(readJson(metadata).value("settings",json::object()),"content_mode")=="hardcore";}catch(...){}}if(!checked.valid||(!all&&!filter.visible(filter.active.empty()?"":localImagePanel(cfg,file)))||(!all&&adult!=hc)||(all&&adult&&!flag(cfg,"_gallery_allow_hardcore")))continue;auto name=file.filename().wstring();std::transform(name.begin(),name.end(),name.begin(),::towlower);if(name.find(query)!=std::wstring::npos){out.paths.push_back(file);channels[file]=channelInfo;}
 }
 std::sort(out.paths.begin(),out.paths.end(),[&](auto& a,auto& b){if(all&&channels[a].id!=channels[b].id)return channels[a].name==channels[b].name?channels[a].id<channels[b].id:channels[a].name<channels[b].name;auto x=out.checks.at(a).stamp,y=out.checks.at(b).stamp;return x==y?a.native()<b.native():x>y;});for(size_t i=0;i<out.paths.size();i++){auto& info=channels.at(out.paths[i]);if(out.groups.empty()||out.groups.back().id!=info.id)out.groups.push_back({info.id,info.name,i,0});out.groups.back().count++;}check();return out;
}
}
