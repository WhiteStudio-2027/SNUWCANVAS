#pragma once
#include "gallery_migration.hpp"
#include <optional>
namespace snuw {
struct GalleryFileCheck {fs::file_time_type stamp;uintmax_t bytes=0;bool valid=false;};
struct GalleryScanResult {
 std::string context,migrationKey,error;uint64_t serial=0;bool changed=false,migrationWarning=false;
 std::vector<fs::path> paths;std::map<fs::path,GalleryFileCheck> checks;std::map<fs::path,fs::path> migrations;
};
inline GalleryScanResult collectGallery(const json& cfg,const PanelGalleryFilter& filter,const std::wstring& query,const json& bindings,const std::string& scope,const std::string& channel,const std::string& context,uint64_t serial,const std::map<fs::path,GalleryFileCheck>& previous,const std::string& migrationKey,const std::map<fs::path,fs::path>& migrations,const std::atomic_bool& cancel,std::atomic_size_t& examined){
 GalleryScanResult out;out.context=context;out.serial=serial;bool hc=str(cfg,"content_mode")=="hardcore";auto check=[&]{if(cancel)throw std::runtime_error("cancelled");};
 try{out.migrationKey=scope+channel+(hc?"hc":"safe")+bindings.dump();if(bindings.is_array())for(auto& binding:bindings){check();auto root=fs::path(wide(str(binding,"target")));if(root.root_name().wstring().starts_with(L"\\\\"))throw std::runtime_error("remote migration");std::error_code ec;auto stamp=fs::last_write_time(root/".snuw-layout-migration.json",ec);out.migrationKey+=ec?"missing":std::to_string(stamp.time_since_epoch().count());}out.migrations=out.migrationKey==migrationKey?migrations:migrationGalleryFiles(bindings,scope,channel,hc);}catch(...){check();out.migrationWarning=true;out.migrations.clear();}
 std::vector<fs::path> candidates;std::error_code ec;auto folder=fs::path(wide(str(cfg,"output")));fs::recursive_directory_iterator end;
 for(fs::recursive_directory_iterator it(folder,fs::directory_options::skip_permission_denied,ec);it!=end;it.increment(ec)){check();if(ec){ec.clear();continue;}if(it.depth()>32){it.disable_recursion_pending();continue;}auto attributes=GetFileAttributesW(it->path().c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_DIRECTORY)&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT)){it.disable_recursion_pending();continue;}if(it->is_regular_file(ec)&&imagePath(it->path())&&localImageVisible(cfg,it->path()))candidates.push_back(it->path());if(candidates.size()>100000)throw std::runtime_error("이미지 폴더에 파일이 너무 많습니다. 폴더를 나눈 뒤 다시 불러오세요.");}
 for(auto& [file,root]:out.migrations)candidates.push_back(file);if(candidates.size()>100000)throw std::runtime_error("이미지 폴더에 파일이 너무 많습니다. 폴더를 나눈 뒤 다시 불러오세요.");std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
 Com<IWICImagingFactory> wic;check();if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic))))throw std::runtime_error("이미지 목록 검사기를 준비하지 못했습니다.");
 for(auto& file:candidates){check();examined++;fs::directory_entry entry(file,ec);if(ec||!entry.is_regular_file(ec))continue;if(out.migrations.count(file)){try{migrationSafePath(file);}catch(...){continue;}}auto stamp=entry.last_write_time(ec);if(ec)continue;auto bytes=entry.file_size(ec);if(ec)continue;GalleryFileCheck checked{stamp,bytes,false};auto old=previous.find(file);
  if(old!=previous.end()&&old->second.stamp==stamp&&old->second.bytes==bytes)checked=old->second;else{out.changed|=old!=previous.end();Com<IWICBitmapDecoder> decoder;checked.valid=SUCCEEDED(wic->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder));}out.checks[file]=checked;
  if(!checked.valid||!filter.visible(filter.active.empty()?"":localImagePanel(cfg,file))||(!hc&&imageIsHardcore(cfg,file)))continue;auto name=file.filename().wstring();std::transform(name.begin(),name.end(),name.begin(),::towlower);if(name.find(query)!=std::wstring::npos)out.paths.push_back(file);
 }
 std::sort(out.paths.begin(),out.paths.end(),[&](auto& a,auto& b){auto x=out.checks.at(a).stamp,y=out.checks.at(b).stamp;return x==y?a.native()<b.native():x>y;});check();return out;
}
}
