#include "local_build.hpp"
using namespace snuw;
int wmain(int argc,wchar_t** argv){
 if(argc==6&&std::wstring(argv[1])==L"--apply-local-build")return applyLocalBuild(argv[2],DWORD(std::stoul(argv[3])),utf8(argv[4]),utf8(argv[5]));
 if(argc==3&&std::wstring(argv[1])==L"--hold"){auto deadline=GetTickCount64()+20000;while(GetTickCount64()<deadline&&GetFileAttributesW(argv[2])==INVALID_FILE_ATTRIBUTES)Sleep(20);return 0;}
 auto root=exeDir();bool newer=STUB_NEW;auto mode=root/"fail-new";if(newer&&fs::exists(mode))return 1;
 auto state=root/L".updates/local/SNUWCANVAS.exe.status.json";if(fs::exists(state)){auto status=readJson(state);if(str(status,"state")=="awaiting_health"){status["state"]="healthy";atomicWrite(state,status.dump());}}
 atomicWrite(root/"booted.json",json{{"new",newer}}.dump());return 0;
}
