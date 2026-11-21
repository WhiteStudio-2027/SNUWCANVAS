// App members: live server inventory, scoped replies, explicit per-file settings.
void installedSetting(const std::string& key,const json& value){static const std::set<std::string> allowed={"steps","cfg","sampler","scheduler","clip_skip","lora_strength","lora_clip_strength"};if(allowed.count(key))modelSetting(store->settings,key,value);}
std::string inventoryBinding;bool inventoryBusy=false;ULONGLONG inventoryNext=0;uint64_t inventorySerial=0;
void inventoryTick(){if(welcome||!hwnd||!IsWindowVisible(hwnd))return;auto binding=store->workspaceScope+"|"+str(store->settings,"api")+"|"+sha256(str(store->settings,"api_key"));if(binding!=inventoryBinding){if(!inventoryBinding.empty())capabilities=json::object();inventoryBinding=binding;inventoryBusy=false;inventoryNext=0;++inventorySerial;}if(!engine->busy&&!inventoryBusy&&GetTickCount64()>=inventoryNext&&!str(store->settings,"api_key").empty())capabilitiesRequest();}
void selectedAssetChanged(const std::string& key){
 sync();auto& cfg=store->settings;auto inventory=capabilities.value("installed_models",json::object());
 if(!inventory.contains(key)||!inventory[key].is_array())return;
 for(auto& row:inventory[key])if(row.is_object()&&str(row,"name")==str(cfg,key)){
  auto family=str(row,"family","unknown");
  if(key=="diffusion_model"&&family=="flux2"){
   auto previous=modelId(cfg);auto id=previous;if(id!="flux2_klein"&&id!="flux2_klein_base")cfg["workflow_profile"]="문장·실사 · FLUX 빠르게";
   applyModelSettings(cfg,previous!=modelId(cfg)&&flag(cfg,"model_auto_settings",true));
  }
  if(key=="checkpoint"&&family=="pixel"){
   auto previous=modelId(cfg);cfg["workflow_profile"]="픽셀아트";applyModelSettings(cfg,previous!=modelId(cfg)&&flag(cfg,"model_auto_settings",true));
  }
  if(key=="checkpoint"&&(family=="illustrious"||family=="animagine"||family=="sdxl")){
   auto previous=modelId(cfg);cfg["workflow_profile"]=family=="illustrious"?"애니 · Illustrious XL":"기존 SDXL";
   applyModelSettings(cfg,previous!=modelId(cfg)&&flag(cfg,"model_auto_settings",true));
  }
  auto recommended=row.value("recommended",json::object());if(flag(cfg,"model_auto_settings",true))for(auto& [name,value]:recommended.items())installedSetting(name,value);
  auto trigger=str(row,"trigger");status=family=="unknown"?L"설치 파일 선택 완료 · 계열 미확인: 생성 방식을 직접 확인하세요.":L"설치 파일 선택 완료";
  if(!trigger.empty())status+=L" · LoRA 트리거: "+wide(trigger);
  store->save();rebuild(false);return;
 }
 store->save();
}
void showLoraTrigger(){auto rows=capabilities.value("installed_models",json::object()).value("lora",json::array());for(auto& row:rows)if(str(row,"name")==str(store->settings,"lora")){auto trigger=str(row,"trigger");MessageBoxW(hwnd,wide(trigger.empty()?"등록된 트리거가 없습니다. 제작자 안내를 확인하세요.":"긍정 프롬프트에 추가할 트리거:\n"+trigger).c_str(),L"선택한 LoRA",MB_OK);return;}status=L"LoRA를 선택하고 서버 모델 목록을 갱신하세요.";}

void applyInstalledRecommendations(){auto inventory=capabilities.value("installed_models",json::object());for(auto key:{"checkpoint","lora"}){auto rows=inventory.value(key,json::array());if(!rows.is_array())continue;for(auto& row:rows)if(str(row,"name")==str(store->settings,key)){auto recommended=row.value("recommended",json::object());for(auto& [name,value]:recommended.items())installedSetting(name,value);}}}
