#pragma once
#include "common.hpp"
#include "output_layout.hpp"
namespace snuw {
// Paths under a deep workspace can exceed MAX_PATH; std::filesystem in this toolchain then reports "missing".
// Existence/attribute checks therefore go through the same extended-path Win32 calls the gallery scan uses.
inline fs::path galleryPlainPath(const fs::path& path){auto value=extendedWindowsPath(path);if(value.rfind(L"\\\\?\\UNC\\",0)==0)value=L"\\\\"+value.substr(8);else if(value.rfind(L"\\\\?\\",0)==0)value=value.substr(4);return fs::path(value).lexically_normal();}
inline bool galleryFileInfo(const fs::path& path,WIN32_FILE_ATTRIBUTE_DATA& info){info={};return GetFileAttributesExW(extendedWindowsPath(path).c_str(),GetFileExInfoStandard,&info)!=0;}
inline bool galleryRegularFile(const fs::path& path){WIN32_FILE_ATTRIBUTE_DATA info;return galleryFileInfo(path,info)&&!(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));}
inline bool galleryAnyEntry(const fs::path& path){WIN32_FILE_ATTRIBUTE_DATA info;return galleryFileInfo(path,info);}
inline uintmax_t galleryFileSize(const fs::path& path){WIN32_FILE_ATTRIBUTE_DATA info;if(!galleryFileInfo(path,info))return 0;return (uintmax_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow;}
inline bool galleryInside(const fs::path& root,const fs::path& path){auto base=galleryPlainPath(root),target=galleryPlainPath(path);auto relative=target.lexically_relative(base);return !relative.empty()&&!relative.is_absolute()&&*relative.begin()!=L"..";}
inline bool gallerySameFolder(const fs::path& a,const fs::path& b){auto x=galleryPlainPath(a).wstring(),y=galleryPlainPath(b).wstring();while(!x.empty()&&x.back()==L'\\')x.pop_back();while(!y.empty()&&y.back()==L'\\')y.pop_back();std::transform(x.begin(),x.end(),x.begin(),::towlower);std::transform(y.begin(),y.end(),y.begin(),::towlower);return x==y;}
inline std::vector<fs::path> galleryDeleteTargets(const fs::path& folder,const fs::path& selected){
 auto root=galleryPlainPath(folder),image=galleryPlainPath(selected);
 bool structured=galleryRegularFile(root/L".snuw-output.json")||galleryRegularFile(root/L".snuw-layout-migration.json");
 if(!imagePath(image)||!galleryRegularFile(image)||!galleryInside(root,image)||(!structured&&!gallerySameFolder(image.parent_path(),root)))throw std::runtime_error("현재 갤러리 폴더의 이미지만 삭제할 수 있습니다.");
 std::vector<fs::path> result{image};auto sidecar=image;sidecar.replace_extension(L".json");
 for(auto candidate:{sidecar,fs::path(image.wstring()+L".json"),imageMetadataPath(image)})if(galleryAnyEntry(candidate)){
  if(!galleryRegularFile(candidate)||!galleryInside(root,candidate))throw std::runtime_error("연결된 JSON 경로를 확인하세요.");
  if(std::find(result.begin(),result.end(),candidate)==result.end())result.push_back(candidate);
 }return result;
}
inline bool recycleGalleryFiles(HWND owner,const std::vector<fs::path>& targets){
 bool tooLong=false;for(auto& path:targets)if(galleryPlainPath(path).wstring().size()>=MAX_PATH-2)tooLong=true;
 if(tooLong){ // the shell recycle API cannot take paths longer than MAX_PATH; delete only after an explicit confirmation
  if(MessageBoxW(owner,L"파일 경로가 너무 길어 휴지통으로 보낼 수 없습니다.\n완전히 삭제할까요? 복구할 수 없습니다.",L"갤러리 이미지 삭제",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES)return false;
  for(auto& path:targets){if(!DeleteFileW(extendedWindowsPath(path).c_str())&&GetLastError()!=ERROR_FILE_NOT_FOUND)throw std::runtime_error("이미지 삭제에 실패했습니다. 파일 사용 여부와 권한을 확인하세요.");}
  return true;
 }
 std::wstring files;for(auto& path:targets){files+=galleryPlainPath(path).wstring();files.push_back(L'\0');}files.push_back(L'\0');SHFILEOPSTRUCTW operation{};operation.hwnd=owner;operation.wFunc=FO_DELETE;operation.pFrom=files.c_str();operation.fFlags=FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;int code=SHFileOperationW(&operation);if(code)throw std::runtime_error("이미지 삭제에 실패했습니다. 파일 사용 여부와 권한을 확인하세요.");return !operation.fAnyOperationsAborted;
}
}
