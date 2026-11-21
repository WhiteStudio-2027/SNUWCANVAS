// App members: everyday controls first; technical controls stay reachable in Details.
int detailSection=0;
std::wstring referenceRoleLabel(const std::string& role)const{return role=="clothing"?L"의상":role=="style"?L"화풍":role=="pose"?L"포즈":L"캐릭터";}
std::string referenceRoleId(const std::wstring& label)const{return label==L"의상"?"clothing":label==L"화풍"?"style":label==L"포즈"?"pose":label==L"캐릭터"?"character":utf8(label);}
void editReferenceTiming(){auto& refs=store->state["project"]["references"];if(refIndex<0||refIndex>=int(refs.size()))throw std::runtime_error("참조 화면에서 이미지를 먼저 선택하세요.");auto value=inputBox(hwnd,L"선택한 참조의 적용 구간 (0~1)",wide(str(refs[refIndex],"end","0.8")));if(value.empty())return;size_t used=0;double end=std::stod(value,&used);if(used!=value.size()||!std::isfinite(end)||end<0||end>1)throw std::runtime_error("0~1 사이 숫자를 입력하세요.");refs[refIndex]["end"]=end;store->save();status=L"선택한 참조의 적용 구간을 저장했습니다.";}
bool promptSectionsOpen()const{return flag(store->state,"_ui_prompt_sections")||promptTab!=0;}
bool promptNegativeOpen()const{return true;}
Rect promptMainRect()const{auto c=center(),g=card();float top=promptSectionsOpen()?202.f:140.f;float negativeSpace=promptNegativeOpen()?std::clamp((g.y-top)*.24f,68.f,140.f)+34:0;return {c.x+14,top,c.w-28,std::max(64.f,g.y-16-top-negativeSpace)};}
Rect promptNegativeRect()const{auto c=center(),g=card();if(!promptNegativeOpen())return {c.x+14,g.y-80,c.w-28,64};auto main=promptMainRect();float top=main.y+main.h+34;return {c.x+14,top,c.w-28,std::max(40.f,g.y-16-top)};}
Rect settingsFormArea(Rect c,Rect g)const{if(page==10)return {c.x+4,140,c.w-8,std::max(20.f,g.y-186)};float top=(page==2||page==4)?124.f:90.f,bottom=(page==2||page==4)?g.y-52:g.y-8;return {c.x+4,top,c.w-8,std::max(20.f,bottom-top)};}
void togglePromptSections(){if(promptComposing)return;sync();bool next=!promptSectionsOpen();if(!next)promptTab=0;store->state["_ui_prompt_sections"]=next;rebuild(false);store->save();}
void openDetails(int section=0){if(promptComposing)return;if(welcome)throw std::runtime_error("로그인 후 이용하세요.");save();detailSection=std::clamp(section,0,4);page=4;formScroll=0;rebuild(false);}
void chooseFriendlySize(int shape){sync();if(modelId(store->settings)=="pixel_art"){store->settings["pixel_width"]=shape==0?"64":"128";store->settings["pixel_height"]=shape==2?"64":"128";}else{store->settings["width"]=shape==0?"832":shape==1?"1024":"1216";store->settings["height"]=shape==0?"1216":shape==1?"1024":"832";}rebuild(false);store->save();}
void sizeShortcutsPaint(Rect c){float w=(c.w-24)/3;for(int i=0;i<3;i++)button({c.x+4+i*(w+8),84,w,28},i==0?L"세로":i==1?L"정사각":L"가로",[this,i]{chooseFriendlySize(i);});}
void promptToolsMenu(){if(promptComposing)return;HMENU menu=CreatePopupMenu();for(auto [id,title]:std::vector<std::pair<int,const wchar_t*>>{{1,L"프리셋 불러오기"},{2,L"프리셋 저장"},{3,L"텍스트 파일 불러오기"},{4,L"프롬프트 저장"},{5,L"제외 내용 파일 불러오기"},{6,L"표정 선택"}})AppendMenuW(menu,MF_STRING,id,title);POINT p;GetCursorPos(&p);int choice=TrackPopupMenu(menu,TPM_RETURNCMD,p.x,p.y,0,hwnd,nullptr);DestroyMenu(menu);if(choice==1)preset(false);else if(choice==2)preset(true);else if(choice==3)loadText();else if(choice==4)savePrompts();else if(choice==5){loadText(true);}else if(choice==6)emotion();}
void friendlySeedMenu(){HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING|(seedBusy?MF_GRAYED:0),1,L"현재 입력으로 추천받기");AppendMenuW(menu,MF_STRING|(seedChoices.empty()?MF_GRAYED:0),2,L"추천 결과 선택");AppendMenuW(menu,MF_STRING|(seedUndo.empty()?MF_GRAYED:0),3,L"원래 시드로 되돌리기");AppendMenuW(menu,MF_STRING,4,flag(store->settings,"seed_recommendations_enabled",true)?L"추천 기능 끄기":L"추천 기능 켜기");POINT p;GetCursorPos(&p);int choice=TrackPopupMenu(menu,TPM_RETURNCMD,p.x,p.y,0,hwnd,nullptr);DestroyMenu(menu);if(choice==1)startSeedRecommendations();else if(choice==2)chooseRecommendedSeed();else if(choice==3)undoRecommendedSeed();else if(choice==4){bool enabled=!flag(store->settings,"seed_recommendations_enabled",true);resetSeedRecommendations();store->settings["seed_recommendations_enabled"]=enabled;save();}}
void paintFriendlyPrompt(Rect c){
 float bw=(c.w-18)/2;
 button({c.x+6,84,bw,28},promptSectionsOpen()?L"통합 입력":L"항목별 입력",[this]{togglePromptSections();},promptSectionsOpen());
 button({c.x+12+bw,84,bw,28},L"입력 도구",[this]{promptToolsMenu();});
 if(promptSectionsOpen()){float part=c.w/3;for(int i=0;i<6;i++){Rect r{c.x+(i%3)*part,124+(i/3)*24.f,part-4,21};renderer.box(r,i==promptTab?0x34343b:0x242426,0,5);label(sections[i].second,{r.x+9,r.y,r.w-18,r.h},false,i==promptTab?0xe5e5e7:0x99999f,11);buttons.push_back({r,L"",[this,i]{if(promptComposing)return;sync();promptTab=i;rebuild(false);},false});}}
 auto main=promptMainRect();label(L"프롬프트",{main.x-4,main.y-25,main.w,20},true,0xccccd0,12);renderer.box({main.x-6,main.y-5,main.w+12,main.h+10},0x161617,0x333338,7);
 if(promptNegativeOpen()){auto negative=promptNegativeRect();label(L"제외할 내용",{negative.x-4,negative.y-25,negative.w,20},false,0xb0b0b7,11);renderer.box({negative.x-6,negative.y-5,negative.w+12,negative.h+10},0x161617,0x333338,7);}
}
std::vector<std::pair<std::string,std::wstring>> friendlySettingsList()const{
 if(page==2)return {{"width",L"이미지 너비"},{"height",L"이미지 높이"},{"count",L"만들 장수"}};
 if(page!=4)return {};
 if(detailSection==1)return {{"steps",L"생성 단계 (Steps)"},{"cfg",L"설명 반영 (CFG)"},{"sampler",L"샘플러"},{"scheduler",L"스케줄러"},{"denoise",L"원본 변경 강도"}};
 if(detailSection==2)return {{"checkpoint",L"모델 파일"},{"vae",L"색상 복원 (VAE)"},{"lora",L"추가 모델 (LoRA)"},{"lora_strength",L"추가 모델 강도"},{"lora_clip_strength",L"추가 설명 강도"},{"clip_skip",L"CLIP 레이어"},{"diffusion_model",L"확산 모델"},{"text_encoder",L"텍스트 인코더"},{"tile_size",L"VAE 타일 크기"},{"tile_overlap",L"VAE 타일 겹침"},{"node_overrides",L"노드 JSON"}};
 if(detailSection==3)return {{"ip_weight",L"참조 이미지 영향"},{"ip_end",L"참조 적용 구간"},{"pose_strength",L"포즈 영향"},{"pose_end",L"포즈 적용 구간"},{"initial_image",L"시작 이미지 파일"}};
 if(detailSection==4)return {{"seed",L"시드 (-1: 무작위)"},{"version",L"파일 이름 앞부분"},{"output",L"저장 폴더"}};
 return {};
}
void detailsPaint(Rect c,Rect g){
 float part=(c.w-28)/5;
 const wchar_t* titles[]={L"도구",L"생성",L"모델",L"참조",L"파일"};
 for(int i=0;i<5;i++)button({c.x+4+i*(part+5),84,part,28},titles[i],[this,i]{openDetails(i);},detailSection==i);
 if(detailSection!=0){float w=(c.w-28)/3;button({c.x+4,g.y-40,w,28},L"권장값 적용",[this]{restoreRecommendedSettings();});button({c.x+12+w,g.y-40,w,28},detailSection==2?L"LoRA 트리거":L"되돌리기",[this]{if(detailSection==2)showLoraTrigger();else undoRecommendedSettings();},false,detailSection==2||recommendedUndoAvailable());button({c.x+20+w*2,g.y-40,w,28},detailSection==4?L"폴더 선택":detailSection==3?L"참조 구간":L"모델 목록",[this]{if(detailSection==4){auto path=pickFolder(hwnd);if(!path.empty()){sync();store->settings["output"]=utf8(path);rebuild(false);save();scan();}}else if(detailSection==3)editReferenceTiming();else capabilitiesRequest();});return;}
 std::vector<std::pair<std::wstring,std::function<void()>>> actions={
  {L"이미지 보정·확대",[this]{setPage(5);}},{L"후보 만들기·비교",[this]{setPage(16);}},
  {L"페이지·컷 구성",[this]{setPage(17);}},{L"좋은 시드 추천",[this]{friendlySeedMenu();}},
  {L"진행 기록",[this]{setPage(3);}},{L"서버 이미지",[this]{setPage(9);}}};
 if(modelsSupported())actions.push_back({L"모델 추가·관리",[this]{setPage(14);}});
 if(developer()){actions.push_back({L"캐릭터·화풍 일관성",[this]{setPage(7);}});actions.push_back({L"내 이미지로 학습",[this]{setPage(8);}});actions.push_back({L"여러 각도 참조 시트",[this]{setPage(15);}});actions.push_back({L"서버 관리",[this]{setPage(11);if(!developerBusy)developerFetch();}});}
 float w=(c.w-24)/2;for(size_t i=0;i<actions.size();i++){float x=c.x+4+(i%2)*(w+16),y=138+(i/2)*58.f;button({x,y,w,46},actions[i].first,actions[i].second);}
 if(seedBusy||!seedChoices.empty()||!status.empty())label(seedBusy?L"시드를 추천받는 중…":status,{c.x+6,height-72,c.w-12,26},false,0xccccd0,11);
}
