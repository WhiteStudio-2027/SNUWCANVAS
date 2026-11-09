// App members. Promotion is explicit; neither project sync nor training starts here.
json auditionPromotionCandidate(bool sheetOnly=false){
 save();if(auditionRunning||auditionInFlight||engine->locked(store->active)||learningBusy)throw std::runtime_error("현재 작업을 마치거나 일시정지한 뒤 진행하세요.");
 auto& ledger=auditions();if(auditionRun.empty()||auditionSelected<0)throw std::runtime_error("평가한 후보를 선정본으로 저장하세요.");
 auto& run=ledger.run(auditionRun);auto& item=ledger.task(auditionRun,auditionSelected);
 if(run.contains("reference_sheet")&&!referenceSheetApproved(run,auditionSelected))throw std::runtime_error("시트의 각도별 외형을 먼저 확인하세요.");
 if(sheetOnly&&!run.contains("reference_sheet"))throw std::runtime_error("참조 시트 후보를 선택하세요.");
 if(!sheetOnly&&(!run.contains("selected")||!run["selected"].is_number_integer()||run["selected"].get<int>()!=auditionSelected||str(item,"state")!="completed"||item.value("stars",0)<1))throw std::runtime_error("평가한 후보를 선정본으로 저장하세요.");
 if(str(run,"server")!=sha256(str(store->settings,"api"))||str(run,"mode")!=str(store->settings,"content_mode","standard"))throw std::runtime_error("후보를 만든 서버와 모드로 돌아가세요.");
 if(sha256(run["scenes"].dump())!=str(run,"snapshot_hash"))throw std::runtime_error("오디션 기준 설정이 변경되었습니다.");
 auto hash=str(item,"sha256");if(hash.size()!=64||hash.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("이전 후보에는 파일 지문이 없습니다. 새 오디션 후보를 생성해 주세요.");
 auto path=fs::path(wide(str(item,"path")));std::error_code ec;auto size=fs::file_size(path,ec);
 if(ec||!size||size>25*1024*1024||!imagePath(path)||!localImageVisible(store->settings,path)||(!hardcore()&&imageIsHardcore(store->settings,path)))throw std::runtime_error("현재 계정과 모드의 후보 이미지 파일을 확인하세요.");
 json source={{"path",str(item,"path")},{"sha256",hash},{"run",auditionRun},{"candidate",auditionSelected},{"scene",item.value("scene",0)},{"seed",item.at("seed")},{"mode",str(run,"mode")},{"snapshot_hash",str(run,"snapshot_hash")}};if(run.contains("reference_sheet")){source["sheet_angle"]=str(run["scenes"][auditionSelected],"angle");source["inferred_details"]=run["scenes"][auditionSelected].value("hypothesis",false);source["profile_content_version"]=str(run["reference_sheet"],"content_version");source["front_source_sha256"]=str(run["reference_sheet"],"source_sha256");source["appearance_reviewed"]=true;}return source;
}
void promoteAuditionReference(const std::string& role,bool sheetOnly=false){
 if(role!="character"&&role!="style")throw std::runtime_error("참조 용도를 확인하세요.");auto source=auditionPromotionCandidate(sheetOnly);
 if(modelId(store->settings)=="pixel_art")throw std::runtime_error("픽셀아트는 참조 이미지와 함께 사용할 수 없습니다.");
 auto& refs=store->state["project"]["references"];if(!refs.is_array())refs=json::array();
 if(refs.size()>=10)throw std::runtime_error("참조 이미지는 최대 10장입니다.");int other=0;for(auto& row:refs){if(str(row,"path")==str(source,"path")||str(row,"audition_sha256")==str(source,"sha256"))throw std::runtime_error("이미 참조에 추가한 후보입니다.");other+=str(row,"role")!="pose";}
 if(str(store->settings,"workflow_profile").find("FLUX")==std::string::npos&&other>=3)throw std::runtime_error("캐릭터·의상·화풍 참조는 합계 3장까지 가능합니다.");
 auto previous=store->state;refs.push_back({{"path",source["path"]},{"role",role},{"weight",0.6},{"end",0.8},{"audition_sha256",source["sha256"]},{"audition_source",source}});
 try{store->save();}catch(...){store->state=previous;throw;}refIndex=int(refs.size())-1;page=1;formScroll=0;rebuild(false);status=L"선정본을 참조에 추가했습니다.";
}
void promoteAuditionTraining(const std::string& profile,bool sheetOnly=false){
 if(!developer())throw std::runtime_error("학습 이미지 추가는 개발자 권한이 필요합니다.");auto source=auditionPromotionCandidate(sheetOnly);const json* target=nullptr;for(auto& row:learningProfiles)if(str(row,"id")==profile)target=&row;
 if(!target||!boardId(profile)||textEntityKind(str(*target,"kind")))throw std::runtime_error("현재 서버의 캐릭터 또는 화풍 프로필을 선택하세요.");
 store->settings["profile_target"]=profile+" | "+str(*target,"name");store->save();page=7;consistencyView=2;formScroll=0;rebuild(false);
 // The normal upload worker checks the exact bytes before sending. Auto-train is deliberately disabled.
 auto path=fs::path(wide(str(source,"path")));startReferenceTransfer({path},{},{{path,str(source,"sha256")}},false,"");
}
void auditionTrainingMenu(){
 if(!developer())throw std::runtime_error("학습 이미지 추가는 개발자 권한이 필요합니다.");auditionPromotionCandidate();if(learningProfiles.empty())throw std::runtime_error("일관성에서 프로필 목록을 먼저 새로고침하세요.");
 HMENU menu=CreatePopupMenu();std::vector<std::string> ids;for(auto& row:learningProfiles)if(!textEntityKind(str(row,"kind"))&&boardId(str(row,"id"))){ids.push_back(str(row,"id"));AppendMenuW(menu,MF_STRING,ids.size(),wide(str(row,"name")).c_str());}
 POINT point;GetCursorPos(&point);int choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,point.x,point.y,0,hwnd,nullptr);DestroyMenu(menu);
 if(choice>0&&size_t(choice)<=ids.size()&&MessageBoxW(hwnd,L"선정본 한 장을 선택한 프로필에 업로드할까요?\n학습은 시작하지 않습니다. 여러 각도의 검토된 이미지를 모은 뒤 학습하세요.",L"학습 이미지 추가",MB_YESNO|MB_ICONQUESTION)==IDYES)promoteAuditionTraining(ids[choice-1]);
}
