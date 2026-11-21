#pragma once
#include "common.hpp"
namespace snuw {
// 통합 설정 패널이 표시하는 모든 항목의 정의. 값은 Store::settings 에 있고 기본값은 defaults() 가 가진다.
enum class SettingType{Toggle,Choice,Number,Text,Action,Info};
struct SettingDef {
 std::string key;            // settings 키 (Action/Info 는 식별자)
 std::wstring label,desc;    // 카드 제목, 설명
 int category=0;             // settingCategories() 색인
 std::wstring section;       // 카테고리 안의 단락 제목
 SettingType type=SettingType::Text;
 std::vector<std::pair<std::string,std::wstring>> options; // Choice: {저장값, 표시}
 bool integer=false;         // Number: 정수만 허용
 bool restart=false;         // 적용에 재시작이 필요함
 bool resettable=true;       // 개별 초기화 대상
};
struct SettingCategory {std::wstring name,hint;int icon;bool savesDefaults;};
inline const std::vector<SettingCategory>& settingCategories(){
 static const std::vector<SettingCategory> v={
  {L"일반",L"앱 동작과 업데이트",40,false},
  {L"생성 기본값",L"이미지 생성 기본 파라미터",41,true},
  {L"모델",L"체크포인트, VAE, LoRA",42,true},
  {L"참조·일관성",L"참조 이미지와 포즈 영향",43,true},
  {L"저장·파일",L"저장 폴더와 메타데이터",44,false},
  {L"화면·모션",L"애니메이션과 단축키",45,false},
  {L"연결·고급",L"서버, 초기화, 정보",46,false}};
 return v;}
inline const std::vector<SettingDef>& settingRegistry(){
 using T=SettingType;
 static const std::vector<SettingDef> v=[]{
  std::vector<SettingDef> r;
  auto add=[&](SettingDef d){r.push_back(std::move(d));};
  auto num=[&](const char* key,const wchar_t* label,const wchar_t* desc,int cat,const wchar_t* section,bool integer){SettingDef d;d.key=key;d.label=label;d.desc=desc;d.category=cat;d.section=section;d.type=T::Number;d.integer=integer;add(d);};
  auto text=[&](const char* key,const wchar_t* label,const wchar_t* desc,int cat,const wchar_t* section){SettingDef d;d.key=key;d.label=label;d.desc=desc;d.category=cat;d.section=section;d.type=T::Text;add(d);};
  auto toggle=[&](const char* key,const wchar_t* label,const wchar_t* desc,int cat,const wchar_t* section){SettingDef d;d.key=key;d.label=label;d.desc=desc;d.category=cat;d.section=section;d.type=T::Toggle;add(d);};
  auto action=[&](const char* key,const wchar_t* label,const wchar_t* desc,int cat,const wchar_t* section){SettingDef d;d.key=key;d.label=label;d.desc=desc;d.category=cat;d.section=section;d.type=T::Action;d.resettable=false;add(d);};
  auto info=[&](const char* key,const wchar_t* label,const wchar_t* desc,int cat,const wchar_t* section){SettingDef d;d.key=key;d.label=label;d.desc=desc;d.category=cat;d.section=section;d.type=T::Info;d.resettable=false;add(d);};
  // 0 일반
  action("content_mode",L"하드코어 모드",L"민감 콘텐츠 생성과 갤러리를 허용합니다. 변경하려면 인증이 필요합니다.",0,L"콘텐츠");
  toggle("auto_update",L"자동 업데이트 확인",L"시작할 때 새 빌드가 있는지 확인합니다.",0,L"업데이트");
  toggle("splash_minimum_enabled",L"시작 화면 최소 2초 표시",L"시작 화면을 최소 2초 동안 보여 줍니다.",0,L"시작");
  // 1 생성 기본값
  num("count",L"생성 장수",L"한 번에 만들 이미지 수",1,L"기본 파라미터",true);
  num("width",L"가로 크기",L"픽셀 단위 (모델 권장 832)",1,L"기본 파라미터",true);
  num("height",L"세로 크기",L"픽셀 단위 (모델 권장 1216)",1,L"기본 파라미터",true);
  text("seed",L"시드",L"-1이면 매번 무작위",1,L"기본 파라미터");
  num("steps",L"Steps",L"샘플링 단계 수",1,L"샘플링",true);
  num("cfg",L"CFG",L"프롬프트를 따르는 강도",1,L"샘플링",false);
  text("sampler",L"Sampler",L"예: euler_ancestral",1,L"샘플링");
  text("scheduler",L"Scheduler",L"예: normal, karras",1,L"샘플링");
  num("denoise",L"Denoise",L"1이면 완전히 새로 생성",1,L"샘플링",false);
  toggle("quality_tags",L"품질 태그 자동 추가",L"프롬프트 앞에 품질 태그를 붙입니다.",1,L"프롬프트");
  toggle("use_default_negative",L"기본 제외 프롬프트 사용",L"제외할 내용에 기본 문구를 함께 보냅니다.",1,L"프롬프트");
  toggle("enhance",L"프롬프트 보강",L"서버가 프롬프트를 다듬도록 요청합니다.",1,L"프롬프트");
  toggle("seed_recommendations_enabled",L"시드 추천 사용",L"좋았던 결과 기반의 시드 추천을 표시합니다.",1,L"프롬프트");
  // 2 모델
  text("checkpoint",L"체크포인트",L"'서버 기본'이면 서버가 정합니다.",2,L"기본 모델");
  text("vae",L"VAE",L"'서버 기본'이면 서버가 정합니다.",2,L"기본 모델");
  text("lora",L"LoRA",L"'사용 안 함'이면 적용하지 않습니다.",2,L"LoRA");
  num("lora_strength",L"LoRA 강도",L"모델 강도",2,L"LoRA",false);
  num("lora_clip_strength",L"LoRA CLIP 강도",L"텍스트 인코더 강도",2,L"LoRA",false);
  text("clip_skip",L"CLIP Skip",L"'서버 기본' 또는 정수",2,L"고급 모델");
  text("upscale_model",L"업스케일 모델",L"확대 보정에 쓸 모델",2,L"고급 모델");
  text("diffusion_model",L"Diffusion 모델",L"분리형 모델 사용 시",2,L"고급 모델");
  text("text_encoder",L"텍스트 인코더",L"분리형 모델 사용 시",2,L"고급 모델");
  // 3 참조·일관성
  num("ip_weight",L"참조 영향",L"참조 이미지가 결과에 미치는 세기",3,L"참조",false);
  num("ip_end",L"참조 적용 구간",L"참조를 적용할 단계의 끝 비율",3,L"참조",false);
  num("pose_strength",L"포즈 영향",L"포즈 참조의 세기",3,L"포즈",false);
  num("pose_end",L"포즈 적용 구간",L"포즈를 적용할 단계의 끝 비율",3,L"포즈",false);
  toggle("consistency_enabled",L"일관성 기능",L"캐릭터·화풍 프로필을 생성에 반영합니다.",3,L"일관성");
  toggle("learn_preferences",L"선호 학습",L"선택한 결과를 학습해 추천에 반영합니다.",3,L"일관성");
  // 4 저장·파일
  text("output",L"저장 폴더",L"생성 이미지를 저장할 폴더 경로",4,L"저장");
  toggle("save_metadata",L"메타데이터 저장",L"이미지에 프롬프트와 설정을 함께 기록합니다.",4,L"저장");
  info("_local_workspace_root",L"작업 공간 위치",L"이 계정의 설정과 프로젝트가 저장된 폴더",4,L"작업 공간");
  // 5 화면·모션
  {SettingDef d;d.key="motion_mode";d.label=L"모션 줄이기";d.desc=L"버튼·스크롤바·진행 바의 애니메이션. 시스템 설정을 따를 수도 있습니다.";d.category=5;d.section=L"애니메이션";d.type=T::Choice;d.options={{"system",L"시스템 따름"},{"on",L"항상 켬"},{"off",L"항상 끔"}};add(d);}
  info("_k_settings",L"설정 열기",L"Ctrl + ,",5,L"단축키");
  info("_k_save",L"저장",L"Ctrl + S",5,L"단축키");
  info("_k_panel",L"중앙 패널 열기/닫기",L"Ctrl + B",5,L"단축키");
  info("_k_new",L"새 프로젝트",L"Ctrl + N",5,L"단축키");
  info("_k_open",L"가져오기",L"Ctrl + O",5,L"단축키");
  // 6 연결·고급
  info("api",L"서버 주소",L"현재 연결된 서버",6,L"서버");
  action("server",L"서버 세부 설정",L"서버를 바꾸면 현재 계정에서 로그아웃됩니다.",6,L"서버");
  text("caption_api",L"캡션 API 주소",L"캡션 생성 서버 주소",6,L"서버");
  text("node_overrides",L"노드 덮어쓰기 (JSON)",L"고급 사용자용. 잘못 입력하면 생성이 실패할 수 있습니다.",6,L"고급");
  action("reset",L"설정 초기화",L"생성 기본값을 포함한 모든 설정을 처음 상태로 되돌립니다.",6,L"초기화");
  info("_about",L"White Studio",L"라이선스 저작권 copyright license 모든 권리 버전 version 정보 about",6,L"정보 및 라이선스");
  return r;}();
 return v;}
inline std::wstring settingFold(const std::wstring& s){std::wstring out;out.reserve(s.size());for(wchar_t c:s)out.push_back(c>=L'A'&&c<=L'Z'?wchar_t(c+32):c);return out;}
// 검색: 공백으로 나눈 모든 조각이 라벨·설명·키·단락 중 하나에 들어 있어야 한다.
inline bool settingMatches(const SettingDef& d,const std::wstring& query){if(query.empty())return true;auto hay=settingFold(d.label+L" "+d.desc+L" "+d.section+L" "+std::wstring(d.key.begin(),d.key.end()));for(auto& o:d.options)hay+=L" "+settingFold(o.second);size_t i=0;while(i<query.size()){while(i<query.size()&&query[i]==L' ')i++;size_t j=i;while(j<query.size()&&query[j]!=L' ')j++;if(j>i&&hay.find(settingFold(query.substr(i,j-i)))==std::wstring::npos)return false;i=j;}return true;}
inline std::string settingText(const json& v){return v.is_string()?v.get<std::string>():v.is_null()?std::string():v.dump();}
}
