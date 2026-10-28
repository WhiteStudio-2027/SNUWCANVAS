#pragma once
#include "common.hpp"
#include "output_layout.hpp"
namespace snuw {
inline std::vector<fs::path> galleryDeleteTargets(const fs::path& folder,const fs::path& selected){
 auto root=fs::weakly_canonical(folder),image=fs::absolute(selected).lexically_normal();
 auto inside=[&](const fs::path& path){auto relative=fs::weakly_canonical(path).lexically_relative(root);return !relative.empty()&&!relative.is_absolute()&&*relative.begin()!=L"..";};
 bool structured=fs::is_regular_file(root/L".snuw-output.json");
 if(!imagePath(image)||!fs::is_regular_file(image)||fs::is_symlink(image)||!inside(image)||(!structured&&fs::weakly_canonical(image.parent_path())!=root))throw std::runtime_error("현재 갤러리 폴더의 이미지만 삭제할 수 있습니다.");
 std::vector<fs::path> result{image};auto sidecar=image;sidecar.replace_extension(L".json");
 for(auto candidate:{sidecar,fs::path(image.wstring()+L".json"),imageMetadataPath(image)})if(fs::exists(candidate)){
  if(!fs::is_regular_file(candidate)||fs::is_symlink(candidate)||!inside(candidate))throw std::runtime_error("연결된 JSON 경로를 확인하세요.");
  if(std::find(result.begin(),result.end(),candidate)==result.end())result.push_back(candidate);
 }return result;
}
inline bool recycleGalleryFiles(HWND owner,const std::vector<fs::path>& targets){std::wstring files;for(auto& path:targets){files+=fs::absolute(path).wstring();files.push_back(L'\0');}files.push_back(L'\0');SHFILEOPSTRUCTW operation{};operation.hwnd=owner;operation.wFunc=FO_DELETE;operation.pFrom=files.c_str();operation.fFlags=FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;int code=SHFileOperationW(&operation);if(code)throw std::runtime_error("이미지 삭제에 실패했습니다. 파일 사용 여부와 권한을 확인하세요.");return !operation.fAnyOperationsAborted;}
}
