#pragma once
namespace snuw {
inline int profileEditorTest(const fs::path& folder){try{
 LoadLibraryW(L"Msftedit.dll");INITCOMMONCONTROLSEX init{sizeof init,ICC_STANDARD_CLASSES};InitCommonControlsEx(&init);
 auto store=std::make_shared<Store>(folder/"state");store->load();App app(store);auto parent=CreateWindowExW(0,L"STATIC",L"",0,0,0,800,700,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);if(!parent)throw std::runtime_error("Hidden parent failed");app.hwnd=parent;app.page=7;app.accountProfile={{"role","develop"}};
 std::string profile(32,'a');json row={{"id",profile},{"name","Original"},{"description","appearance"},{"edit_version",std::string(64,'b')},{"kind","character"},{"pipeline","sdxl"},{"revision",7},{"lora","kept.safetensors"}};
 store->settings["profile_name"]="registration draft";store->settings["profile_description"]="registration description";store->settings["profile_target"]=profile+" | Original";store->settings["consistency_profile"]=profile+" | Original";app.learningProfiles=json::array({row});app.openProfileEditor(row);
 auto layout=app.learningLayout(app.center(),app.card());bool save=false,cancel=false;for(auto& action:layout.actions){save|=action.id==32;cancel|=action.id==33;}if(!save||!cancel||app.consistencyView!=1||app.editingProfile!=profile)throw std::runtime_error("Editor actions missing");for(auto& field:layout.inputs)if(field.key=="profile_pipeline"||field.key=="profile_kind"||field.key=="profile_auto_train")throw std::runtime_error("Training identity exposed as editable");
 app.learningAction(33);if(!app.editingProfile.empty()||str(store->settings,"profile_name")!="registration draft")throw std::runtime_error("Cancel did not restore registration draft");
 app.openProfileEditor(row);auto changed=row;changed["name"]="Renamed";changed["description"]="new appearance";changed["edit_version"]=std::string(64,'c');app.learningReply({"learning_reply",store->active,"profile_updated",{{"reply",changed}}});
 if(!app.editingProfile.empty()||app.consistencyView!=2||str(app.learningProfiles[0],"name")!="Renamed"||str(store->settings,"consistency_profile")!=profile+" | Renamed"||str(app.learningProfiles[0],"lora")!="kept.safetensors")throw std::runtime_error("Saved profile did not refresh selections");
 app.openProfileEditor(row);auto edited=app.editingProfile;app.learningReply({"learning_reply",store->active,"profile_updated",{{"scope","another-account"},{"reply",changed}}});if(app.editingProfile!=edited)throw std::runtime_error("Late account response modified the editor");app.clearProfileEdit();
 app.clearControls();DestroyWindow(parent);app.hwnd=nullptr;atomicWrite(folder/"profile-editor-result.json",json{{"ok",true},{"cancel",true},{"selection_refresh",true},{"training_preserved",true},{"account_scope",true}}.dump());return 0;
 }catch(const std::exception& e){atomicWrite(folder/"profile-editor-result.json",json{{"ok",false},{"error",e.what()}}.dump());return 1;}}
}
