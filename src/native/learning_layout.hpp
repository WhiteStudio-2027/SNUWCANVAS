int consistencyView=0,performanceView=0;
struct LearningInput {std::string key;std::wstring title;Rect control,label;std::vector<std::wstring> choices;bool check=false;};
struct LearningAction {int id;std::wstring text;Rect rect;bool primary=false;};
struct LearningLayout {Rect clip;std::wstring title,hint;std::vector<LearningInput> inputs;std::vector<LearningAction> actions;float height=0,detailsY=0,selectedY=-1,referencesY=-1,referencesHeight=0,trainingY=-1;};
#include "learning_references.hpp"
int learningView()const{return page==7?consistencyView:performanceView;}
void setLearningView(int view){save();referenceScroll=0;selectedReferences.clear();if(page==7)consistencyView=view;else performanceView=view;formScroll=0;rebuild(false);if(page==7&&view==2&&hwnd&&IsWindowVisible(hwnd)&&!learningBusy&&!profileId(str(store->settings,"profile_target")).empty())loadReferenceProfile();}
LearningLayout learningLayout(Rect c,Rect g){
 LearningLayout out;out.clip={c.x+10,128,c.w-26,std::max(50.f,g.y-168)};float x=out.clip.x,w=out.clip.w,y=page==7?0.f:52.f;
 auto input=[&](const char* key,const wchar_t* title,std::vector<std::wstring> values=std::vector<std::wstring>{},bool check=false){out.inputs.push_back({key,title,{x,y+(check?0:21),check?22.f:w,check?22.f:((std::string(key)=="profile_description"||std::string(key)=="profile_caption"||std::string(key)=="caption_result"||std::string(key)=="caption_negative")?64.f:29.f)},{x+(check?30:0),y,check?w-30:w,check?24.f:19.f},values,check});y+=check?34:((std::string(key)=="profile_description"||std::string(key)=="profile_caption"||std::string(key)=="caption_result"||std::string(key)=="caption_negative")?96:61);};
 auto actions=[&](std::vector<std::tuple<int,std::wstring,bool>> items){y+=8;float cols=w>=420?3.f:w>=260?2.f:1.f,part=(w-8*(cols-1))/cols;for(size_t i=0;i<items.size();i++){auto [id,text,primary]=items[i];out.actions.push_back({id,text,{x+float(i%int(cols))*(part+8),y+float(i/int(cols))*36,part,29},primary});}y+=std::ceil(items.size()/cols)*36;};
 auto managed=profileChoices(false);auto styles=profileChoices(true);managed.insert(managed.end(),styles.begin()+1,styles.end());
 if(page==7){switch(consistencyView){
 case 0:out.title=L"사용할 캐릭터 선택";out.hint=L"등록한 캐릭터로 다음 이미지를 만듭니다.";
  input("consistency_enabled",L"일관성 사용",{},true);input("consistency_profile",L"캐릭터 / 대상",profileChoices(false));input("style_profile",L"스타일 (선택 사항)",profileChoices(true));actions({{1,L"새로고침",false},{2,L"저장",true},{3,L"새 캐릭터 등록",false}});break;
 case 1:out.title=L"새 캐릭터 등록";out.hint=L"이름과 항상 유지할 외형을 적어주세요.";
  input("profile_name",L"프로필 이름");input("profile_kind",L"종류",{L"character",L"product",L"brand",L"style"});input("profile_description",L"유지할 외형 (영문)");input("profile_pipeline",L"기반 모델",{L"서버 기본",L"sdxl",L"illustrious"});if(developer())input("profile_auto_train",L"이미지가 모이면 자동 학습",{},true);actions({{4,L"프로필 등록",true}});break;
 case 2:out.title=L"학습 이미지";
  input("profile_target",L"프로필",managed);
  actions({{5,L"이미지 추가",true},{6,L"새로고침",false},{23,L"선택 삭제",false},{28,L"학습 모델 삭제",false},{29,L"프로필 삭제",false}});
  {auto found=referenceProfiles.find(profileId(str(store->settings,"profile_target")));if(found!=referenceProfiles.end()&&str(found->second.value("training_setup",json::object()),"reason")=="pipeline_mismatch")actions({{30,L"서버 모델에 맞추기",true}});}
  input("profile_caption",L"공통 캡션 (선택)");out.inputs.back().control.h=40;y-=24;
  out.referencesY=y+4;out.referencesHeight=std::max(110.f,out.clip.h-y-82-(trainingStatusProfile==profileId(str(store->settings,"profile_target"))&&!trainingStatus.empty()?208.f:0.f));y+=out.referencesHeight+12;
  if(developer())actions({{7,L"학습 시작",true},{8,L"학습 중지",false},{27,L"추가 중지",false}});if(trainingStatusProfile==profileId(str(store->settings,"profile_target"))&&!trainingStatus.empty()){out.trainingY=y+8;y+=208;}break;
 default:out.title=L"자동 캡션 테스트";out.hint=L"로그인한 서버에서 이미지 설명을 만듭니다.";
actions({{18,L"모델 조회",false}});
  {std::vector<std::wstring> choices={L"모델을 조회하세요"};for(auto& row:captionModels)choices.push_back(wide(str(row,"name")));input("caption_model",L"비전 모델",choices);}
  input("caption_mode",L"용도",{L"prompt",L"training"});input("caption_format",L"출력 형식",{L"tags",L"sentence"});input("caption_image",L"테스트 이미지");actions({{19,L"이미지 선택",false},{20,L"자동 캡션",true},{22,L"요청 중지",false}});
  input("caption_result",L"긍정 프롬프트 (수정 가능)");input("caption_negative",L"부정 프롬프트 (생성용)");actions({{21,L"이미지 설명에 적용",true}});break;
 }}else{switch(performanceView){
 case 0:out.title=L"이미지를 평가하고 분석하기";out.hint=L"오른쪽에서 평가할 이미지를 선택하세요.";out.selectedY=y;out.actions.push_back({11,L"파일 선택",{x+w-106,y+8,100,28},false});y+=54;
  input("feedback_reason",L"좋거나 아쉬운 이유");input("profile_caption",L"이미지 특징 · 외부 예시 파일이면 입력");input("learning_vision_model",L"이미지 분석 모델 (선택 사항)",modelChoices(true));actions({{9,L"좋아요 +5",true},{10,L"나빠요 -5",false},{12,L"이미지 분석",false},{13,L"모델 목록 갱신",false},{14,L"분석 결과 보기",false}});break;
 case 1:out.title=L"게시물 성과 기록";out.hint=L"같은 게시물은 최신 누적 수치로 갱신됩니다.";out.selectedY=y;out.actions.push_back({11,L"파일 선택",{x+w-106,y+8,100,28},false});y+=54;
  input("performance_platform",L"플랫폼 이름");input("performance_post_id",L"게시물 ID");
  {float part=(w-16)/3;for(auto [key,title]:std::vector<std::pair<std::string,std::wstring>>{{"performance_impressions",L"노출 수"},{"performance_clicks",L"클릭 수"},{"performance_likes",L"좋아요 수"}}){int index=int(out.inputs.size())-2;out.inputs.push_back({key,title,{x+index*(part+8),y+21,part,29},{x+index*(part+8),y,part,19},{},false});}y+=61;}
  actions({{15,L"성과 기록 저장",true},{16,L"학습 결과 확인",false}});break;
 default:out.title=L"학습 반영과 결과";out.hint=L"이 채널의 평가와 성과를 다음 생성에 반영합니다.";
  input("learn_preferences",L"다음 이미지 생성에 학습 반영",{},true);actions({{17,L"학습 통계 갱신",true},{2,L"저장",false},{14,L"분석 결과 보기",false}});out.detailsY=y+12;y+=110+learnedInfo.value("features",json::array()).size()*28;break;
 }}
 out.height=y+22;return out;
}
float learningScrollMax(){auto layout=learningLayout(center(),card());return std::max(0.f,layout.height-layout.clip.h);}
void positionLearningControls(Rect c,Rect g){auto layout=learningLayout(c,g);formScroll=std::clamp(formScroll,0.f,std::max(0.f,layout.height-layout.clip.h));formArea=layout.clip;for(auto& f:fields)for(auto& spec:layout.inputs)if(f.key==spec.key){f.rect=spec.control;f.rect.y+=layout.clip.y-formScroll;}}
void learningAction(int action){switch(action){
 case 1:featureRequest("profiles","GET","/learning/profiles");break;
 case 2:save();learningNote=L"다음 생성에 적용할 설정을 저장했습니다.";break;
 case 3:setLearningView(1);break;
 case 4:createLearningProfile();break;
 case 5:uploadLearningReference();break;
 case 23:deleteSelectedReferences();break;
 case 28:manageLearningProfile("model");break;
 case 29:manageLearningProfile("profile");break;
 case 30:manageLearningProfile("align");break;
 case 27:referenceTransferCancel=true;break;
 case 6:loadReferenceProfile();break;
 case 7:save();featureRequest("train","POST","/learning/profiles/"+activeProfile()+"/train");break;
 case 8:if(trainingJob.empty())throw std::runtime_error("학습 상태를 먼저 확인하세요.");featureRequest("cancel","POST","/jobs/"+trainingJob+"/cancel");break;
 case 9:learningFeedback(5);break;case 10:learningFeedback(-5);break;
 case 11:{auto file=fileDialog(hwnd,false,L"이미지\0*.png;*.jpg;*.jpeg;*.webp\0\0");if(!file.empty()){select(file);learningNote=L"좋아요 / 나빠요로 선택한 이미지를 평가하세요.";}break;}
 case 12:learningAnalyze();break;case 13:featureRequest("models","GET","/ollama/models");break;
 case 14:if(learningAnalysisId.empty())throw std::runtime_error("이미지 분석 후 결과를 확인하세요.");featureRequest("analysis_result","GET","/jobs/"+learningAnalysisId);break;
 case 15:learningPerformance();break;case 16:setLearningView(2);break;
 case 18:captionStart(true);break;
 case 19:{auto file=fileDialog(hwnd,false,L"이미지\0*.png;*.jpg;*.jpeg;*.webp\0\0");if(!file.empty()){store->settings["caption_image"]=utf8(file);store->settings["caption_result"]="";store->settings["caption_negative"]="";rebuild(false);}break;}
 case 20:captionStart(false);break;
 case 21:save();if(str(store->settings,"caption_result").empty())throw std::runtime_error("먼저 캡션을 만드세요.");store->settings["profile_caption"]=store->settings["caption_result"];captionReference=captionResultFile.empty()?fs::path(wide(str(store->settings,"caption_image"))):captionResultFile;setLearningView(2);learningNote=L"캡션 적용";break;
 case 22:captionCancel=true;learningNote=L"캡션 요청 중지 중…";break;
 case 17:featureRequest("insights","GET","/learning/insights?channel_id="+queryText(store->active));break;
 }}
void learningWorkspacePaint(Rect c,Rect g){
 auto layout=learningLayout(c,g);formScroll=std::clamp(formScroll,0.f,std::max(0.f,layout.height-layout.clip.h));
 auto names=page==7?std::vector<std::wstring>{L"적용",L"프로필",L"학습 이미지",L"캡션"}:std::vector<std::wstring>{L"이미지 평가",L"게시물 성과",L"학습 결과"};
 float part=(c.w-16)/float(names.size());for(int i=0;i<int(names.size());i++){Rect tab{c.x+8+i*part,92,part-4,27};renderer.box(tab,i==learningView()?0x343944:0x242426,0,3);label(names[i],{tab.x+7,tab.y,tab.w-14,27},false,i==learningView()?0xe1e6ff:0x99999f,12);buttons.push_back({tab,L"",[this,i]{setLearningView(i);},false,!learningBusy});}
 auto clip=layout.clip;float offset=clip.y-formScroll;
 size_t contentButtons=buttons.size();renderer.target->PushAxisAlignedClip(clip.d2d(),D2D1_ANTIALIAS_MODE_ALIASED);
 if(page!=7){label(layout.title,{clip.x,offset,clip.w,25},true,0xe9e9ed,15);label(layout.hint,{clip.x,offset+27,clip.w,21},false,0x9797a3,11);}
 if(layout.selectedY>=0){Rect r{clip.x,offset+layout.selectedY,clip.w,43};renderer.box(r,0x222326,0x383a41,4);label(selected.empty()?L"선택된 이미지 없음":selected.filename().wstring(),{r.x+10,r.y+2,std::max(20.f,r.w-128),21},true,0xd2d6e5,12);label(selected.empty()?L"라이브러리 또는 파일 선택으로 시작하세요.":L"오른쪽 미리보기의 이미지가 기록 대상입니다.",{r.x+10,r.y+23,std::max(20.f,r.w-128),17},false,0x9696a1,10);}
 for(auto& spec:layout.inputs){auto box=spec.control;box.y+=offset;if(!spec.check)renderer.box({box.x-1,box.y-1,box.w+2,box.h+2},0x161617,0x3b3e48,3);if(!hwnd){if(spec.check){if(flag(store->settings,spec.key))label(L"✓",box,true,0xb5c2ff,14);}else{auto value=wide(str(store->settings,spec.key));auto split=value.find(L" | ");if(split==32)value=value.substr(35);label(value,{box.x+8,box.y,box.w-16,box.h},false,0xd5d8e2,12);}}auto r=spec.label;r.y+=offset;label(spec.title,r,false,0xb4b7c3,12);}
 if(layout.referencesY>=0)paintReferenceList(layout,offset);
 if(layout.trainingY>=0){float y=offset+layout.trainingY;renderer.box({clip.x,y,clip.w,196},0x141415,0,4);
  label(wide(str(trainingStatus,"label")),{clip.x+10,y+5,clip.w-20,22},true,0xeeeeee,13);
  auto step=trainingStatus.value("step",0),total=trainingStatus.value("total_steps",0),seconds=trainingStatus.value("elapsed_seconds",0);
  std::wstring details=(total?std::to_wstring(step)+L" / "+std::to_wstring(total)+L" 스텝 · ":L"")+std::to_wstring(seconds/60)+L"분 "+std::to_wstring(seconds%60)+L"초";
  label(details,{clip.x+10,y+29,clip.w-20,20},false,0xdddddd,12);
  renderer.box({clip.x+10,y+53,clip.w-20,4},0x38383b,0,1);if(trainingStatus.contains("percent"))renderer.box({clip.x+10,y+53,(clip.w-20)*std::clamp(trainingStatus.value("percent",0.),0.,100.)/100,4},0xeeeeee,0,1);
  if(str(trainingStatus,"state")=="failed")label(wide(str(trainingStatus,"message")),{clip.x+10,y+62,clip.w-20,34},false,0xffb4b4,11);
  auto lines=trainingStatus.value("logs",json::array());int begin=std::max(0,int(lines.size())-6);float lineY=y+98;for(int i=begin;i<int(lines.size());i++){label(wide(lines[i].get<std::string>()),{clip.x+10,lineY,clip.w-20,15},false,0xdddddd,10);lineY+=15;}
 }

 if(layout.detailsY>0){float y=offset+layout.detailsY;renderer.box({clip.x,y,clip.w,82},0x23242a,0x363944,4);label(L"현재 채널의 학습 근거",{clip.x+12,y+5,clip.w-24,24},true);auto ctr=learnedInfo.value("ctr",json());std::wstring percent=L"-";if(ctr.is_number()){std::wostringstream value;value<<std::fixed<<std::setprecision(1)<<ctr.get<double>()*100<<L"%";percent=value.str();}label(L"평가 "+std::to_wstring(learnedInfo.value("samples",0))+L"건 · 클릭률 "+percent,{clip.x+12,y+30,clip.w-24,24},false,0xc7cfee,12);label(L"노출 "+std::to_wstring(learnedInfo.value("impressions",0LL))+L" · 클릭 "+std::to_wstring(learnedInfo.value("clicks",0LL)),{clip.x+12,y+54,clip.w-24,20},false,0x999da9,11);y+=98;for(auto& row:learnedInfo.value("features",json::array())){label(wide(str(row,"prompt"))+L"  ·  "+wide(str(row,"samples"))+L"건",{clip.x+8,y,clip.w-16,25},false,0xbac7ef,12);y+=28;}}
 size_t first=contentButtons;for(auto& action:layout.actions){auto r=action.rect;r.y+=offset;bool available=action.id==22?captionRunning:!learningBusy;if(action.id==8)available&=!trainingJob.empty();if(action.id==23)available&=!selectedReferences.empty();if(action.id==7){auto found=referenceProfiles.find(profileId(str(store->settings,"profile_target")));if(found!=referenceProfiles.end()&&found->second.contains("training_setup"))available&=found->second["training_setup"].value("ready",false);}if(action.id==27)available=referenceUploadRunning;if(action.id==14)available&=!learningAnalysisId.empty();if(action.id==15)available&=!selected.empty();if(action.id==12)available&=!selected.empty();button(r,action.text,[this,id=action.id]{learningAction(id);},action.primary,available);}
 // A scrolled action may never receive clicks outside the content viewport.
 for(size_t i=first;i<buttons.size();i++){auto& r=buttons[i].rect;float bottom=std::min(r.y+r.h,clip.y+clip.h);r.y=std::max(r.y,clip.y);r.h=std::max(0.f,bottom-r.y);if(r.h<=0){r={-10,-10,0,0};buttons[i].enabled=false;}}
 renderer.target->PopAxisAlignedClip();
 float maximum=std::max(0.f,layout.height-clip.h);if(maximum>0){float thumb=std::max(22.f,clip.h*clip.h/layout.height);renderer.box({clip.x+clip.w+5,clip.y,4,clip.h},0x24252a,0,1);renderer.box({clip.x+clip.w+5,clip.y+(clip.h-thumb)*formScroll/maximum,4,thumb},0x646b80,0,1);}
 renderer.line(c.x+10,g.y-31,c.x+c.w-10,g.y-31,0x34353c);label(learningBusy&&!referenceUploadRunning?L"서버 처리 중…":learningNote,{c.x+12,g.y-28,c.w-24,22},false,0xaab6dd,11);
}
