// Members of App. Prompt text is synchronized without rebuilding the editor.
PromptHistory promptHistory;
std::string promptDisplayed;
bool promptComposing=false,promptRebuildPending=false;
void bindPromptHistory(){promptHistory.bind(store->workspaceScope+"|"+store->active);}
void syncPromptEditor(){
 if(binding||promptComposing||page!=0||!positive)return;
 auto text=normalizedText(positive);if(text==promptDisplayed)return;
 bindPromptHistory();promptHistory.record(promptSnapshot(store->state));auto oldError=str(store->state,"_prompt_error");
 try{applyPromptEdit(store->state,sections.at(promptTab).first,wide(text));if(!oldError.empty()&&status==wide(oldError))status.clear();}
 catch(const std::exception& error){status=wide(error.what());}
 promptDisplayed=text;
}
void undoPrompt(bool redo=false){
 if(page!=0||!positive||promptComposing)return;syncPromptEditor();bindPromptHistory();
 if(!promptHistory.restore(store->state,redo))return;
 auto previous=binding;binding=true;auto value=promptDocumentView(store->state,promptTab);SetWindowTextW(positive,value.c_str());SendMessageW(positive,EM_SETSEL,-1,-1);binding=previous;
 promptDisplayed=normalizedText(positive);status=wide(str(store->state,"_prompt_error"));store->save();InvalidateRect(hwnd,nullptr,FALSE);
}
static LRESULT CALLBACK promptEditorProc(HWND edit,UINT message,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR data){
 auto* app=reinterpret_cast<App*>(data);
 if(message==WM_IME_STARTCOMPOSITION){app->promptComposing=true;app->hideEntitySuggestions();}
 if(message==WM_IME_ENDCOMPOSITION){app->promptComposing=false;SetTimer(app->hwnd,3,160,nullptr);if(app->promptRebuildPending)PostMessageW(app->hwnd,WM_APP+93,0,0);}
 if(message==WM_KEYDOWN&&app->entityKey(a))return 0;
 if(message==WM_CONTEXTMENU||message==WM_LBUTTONDOWN)app->hideEntitySuggestions();
 if(message==WM_KILLFOCUS&&(HWND)a!=app->entityList)app->hideEntitySuggestions();
 if(message==WM_NCDESTROY)RemoveWindowSubclass(edit,promptEditorProc,id);
 return DefSubclassProc(edit,message,a,b);
}
