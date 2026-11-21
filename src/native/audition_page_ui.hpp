// App members. Audition screen (page 16): buttons for what used to live only in the popup menu.
int auditionScene=-1;
void auditionPick(const std::string& runId,int index){auto& ledger=auditions();if(auditionRunning||auditionInFlight)throw std::runtime_error("현재 후보를 마친 뒤 목록을 바꿔 주세요.");auditionRun=runId;auditionSelected=index;if(str(ledger.task(auditionRun,auditionSelected),"state")=="completed"){auditionPreview();focusPreview=true;}else status=L"오디션 선택 · 재개 또는 진행 확인을 누르세요.";}
void auditionTogglePick(){auto& ledger=auditions();if(auditionRun.empty()||auditionSelected<0)throw std::runtime_error("먼저 후보를 선택하세요.");std::pair<std::string,int> key{auditionRun,auditionSelected};auto found=std::find(auditionPickSet.begin(),auditionPickSet.end(),key);if(found!=auditionPickSet.end()){auditionPickSet.erase(found);status=L"비교 목록 "+std::to_wstring(auditionPickSet.size())+L"개";return;}if(auditionPickSet.size()>=4)throw std::runtime_error("한 번에 최대 4개까지 비교할 수 있습니다.");if(!auditionPickSet.empty()&&auditionPickSet[0].first!=auditionRun)throw std::runtime_error("같은 오디션의 후보만 함께 비교할 수 있습니다.");if(str(ledger.task(auditionRun,auditionSelected),"state")!="completed")throw std::runtime_error("완료된 후보만 비교할 수 있습니다.");auditionPickSet.push_back(key);status=L"비교 목록 "+std::to_wstring(auditionPickSet.size())+L"개 · 2개 이상이면 비교를 시작하세요.";}
void auditionStartPickSet(){if(auditionPickSet.size()<2)throw std::runtime_error("비교할 후보를 2개 이상 추가하세요.");auto& ledger=auditions();auto runId=auditionPickSet[0].first;auto& run=ledger.run(runId);if(str(run,"mode")!=str(store->settings,"content_mode","standard"))throw std::runtime_error("후보를 만든 모드로 돌아가세요.");int base=auditionPickSet[0].second;std::vector<fs::path> files;for(auto& [id,index]:auditionPickSet){if(id!=runId)throw std::runtime_error("같은 오디션의 후보만 함께 비교할 수 있습니다.");auto& task=ledger.task(id,index);if(str(task,"state")!="completed")throw std::runtime_error("완료된 후보만 비교할 수 있습니다.");if(index!=base&&ledger.task(id,base).value("scene",-1)!=task.value("scene",-2)&&!sameStyleStudy(run,base,index))throw std::runtime_error("같은 장면 또는 같은 화풍 비교의 후보만 묶을 수 있습니다.");files.push_back(fs::path(wide(str(task,"path"))));}beginAuditionComparison(files);status=std::to_wstring(files.size())+L"개 후보를 비교 중입니다.";}
void auditionPaint(Rect c){
 Rect area{c.x+10,78,c.w-20,std::max(80.f,c.h-74)};float x=area.x,y=area.y,bottom=area.y+area.h;
 if(welcome||str(store->settings,"api_key").empty()){label(L"로그인 후 이용하세요.",{x,y,area.w,26},true,0xe5e5e7,14);return;}
 auto& ledger=auditions();auto& runs=ledger.data["runs"];
 std::string runId=auditionRun;if(runId.empty()&&!runs.empty())runId=str(runs.back(),"id");
 json* run=nullptr;for(auto& item:runs)if(str(item,"id")==runId)run=&item;
 bool picked=auditionSelected>=0&&!auditionRun.empty();
 label(std::wstring(L"후보 만들기·비교   ·   ")+(auditionRunning?L"생성 진행 중":auditionRun.empty()?L"선택된 오디션 없음":L"대기 · 일시정지"),{x,y,area.w,26},true,0xe5e5e7,14);y+=34;
 float bx=x,by=y;
 flowButton(area,bx,by,L"새 오디션",92,[this]{auditionMenu(1);},true,!auditionRunning);
 flowButton(area,bx,by,L"재개",64,[this]{auditionMenu(2);},false,!auditionRun.empty()&&!auditionRunning);
 flowButton(area,bx,by,L"일시정지",84,[this]{auditionMenu(3);},false,auditionRunning);
 flowButton(area,bx,by,L"진행 확인",84,[this]{auditionMenu(4);},false,true);
 flowButton(area,bx,by,L"전체 메뉴",84,[this]{auditionMenu();},false,true);
 y=by+38;
 label(L"선택 후보",{x,y+3,70,22},false,0x99999f,11);bx=x+74;by=y;
 for(int stars=1;stars<=5;stars++)flowButton(area,bx,by,L"★"+std::to_wstring(stars),46,[this,stars]{auditionMenu(10+stars);},false,picked);
 y=by+36;bx=x;by=y;
 flowButton(area,bx,by,L"선정본 저장",92,[this]{auditionMenu(20);},false,picked);
 flowButton(area,bx,by,L"기준 A 지정",92,[this]{auditionMenu(21);},false,picked);
 flowButton(area,bx,by,L"B와 비교",78,[this]{auditionMenu(22);},false,picked&&!auditionCompareReference.empty());
 flowButton(area,bx,by,L"비교 종료",78,[this]{auditionMenu(23);},false,true);
 flowButton(area,bx,by,L"→ 캐릭터 참조",104,[this]{auditionMenu(24);},false,picked);
 flowButton(area,bx,by,L"→ 화풍 참조",96,[this]{auditionMenu(25);},false,picked);
 bool inSet=picked&&std::find(auditionPickSet.begin(),auditionPickSet.end(),std::make_pair(auditionRun,auditionSelected))!=auditionPickSet.end();
 flowButton(area,bx,by,inSet?L"비교 목록에서 빼기":L"비교 목록에 담기",132,[this]{auditionTogglePick();},false,picked);
 flowButton(area,bx,by,L"담은 "+std::to_wstring(auditionPickSet.size())+L"개 비교",112,[this]{auditionStartPickSet();},true,auditionPickSet.size()>=2);
 flowButton(area,bx,by,L"목록 비우기",92,[this]{auditionPickSet.clear();status=L"비교 목록을 비웠습니다.";},false,!auditionPickSet.empty());
 y=by+42;
 if(!run){label(L"아직 오디션이 없습니다.\n'새 오디션'을 누르면 현재 페이지의 컷마다 후보를 만듭니다.",{x,y,area.w,48},false,0xb0b0b7,12);}
 else{
  auto& scenes=(*run)["scenes"];auto& tasks=(*run)["tasks"];int scene=auditionScene;
  if(scene<0||scene>=int(scenes.size())){scene=0;if(picked&&runId==auditionRun&&auditionSelected<int(tasks.size()))scene=std::max(0,tasks[auditionSelected].value("scene",0));}
  label(L"장면 (컷)",{x,y,area.w,20},false,0x99999f,11);y+=22;bx=x;by=y;
  for(int si=0;si<int(scenes.size());si++){auto name=wide(str(scenes[si],"name"));if(name.size()>9)name=name.substr(0,9)+L"…";flowButton(area,bx,by,name,96,[this,si]{auditionScene=si;InvalidateRect(hwnd,nullptr,FALSE);},si==scene,true);}
  y=by+38;label(L"후보 (클릭하면 미리보기)",{x,y,area.w,20},false,0x99999f,11);y+=22;
  float cardW=104,cardH=52,cx=x,cy=y;
  for(int i=0;i<int(tasks.size());i++){auto& item=tasks[i];if(item.value("scene",-1)!=scene)continue;if(cx>x&&cx+cardW>x+area.w){cx=x;cy+=cardH+6;}if(cy+cardH>bottom-50)break;
   Rect r{cx,cy,cardW,cardH};bool sel=runId==auditionRun&&i==auditionSelected,ref=auditionCompareReferenceRun==runId&&auditionCompareReferenceIndex==i;auto state=str(item,"state");std::wstring text=state=="completed"?L"완료":state=="pending"?L"대기":state=="running"?L"진행":state=="uncertain"?L"확인 필요":L"실패";
   int setPos=-1;for(size_t k=0;k<auditionPickSet.size();k++)if(auditionPickSet[k].first==runId&&auditionPickSet[k].second==i)setPos=int(k);renderer.box(r,sel?0x34343b:0x242426,sel?0xb5c2ff:setPos>=0?0xe0b86a:ref?0x8fd69a:0x333338,4);label(std::wstring(ref?L"A · ":L"")+L"후보 "+std::to_wstring(item.value("candidate",0)+1),{r.x+8,r.y+4,r.w-16,22},sel,0xe5e5e7,12);label(text+L" · ★"+std::to_wstring(item.value("stars",0))+(setPos>=0?L" · 비교"+std::to_wstring(setPos+1):std::wstring()),{r.x+8,r.y+26,r.w-16,20},false,0xb0b0b7,11);
   buttons.push_back({r,L"",[this,runId,i]{auditionPick(runId,i);},false});cx+=cardW+6;}
 }
 label(status,{x,bottom-44,area.w,22},false,0xb6a4a4,11);label(L"후보를 고른 뒤 별점·선정본 저장·비교를 바로 누를 수 있습니다.",{x,bottom-20,area.w,18},false,0x77777f,10);
}
