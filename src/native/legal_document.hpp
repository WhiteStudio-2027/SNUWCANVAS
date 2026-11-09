// Included by identity.hpp inside namespace snuw. Plain native drawing only: no HTML, URLs or scripts are executed.
struct LegalBlock {std::vector<std::wstring> cells;bool heading=false,table=false;};
inline std::wstring legalTrim(std::wstring value){auto begin=value.find_first_not_of(L" \t\r"),end=value.find_last_not_of(L" \t\r");return begin==std::wstring::npos?L"":value.substr(begin,end-begin+1);}
inline std::vector<std::wstring> legalCells(std::wstring line){
 line=legalTrim(line);if(!line.empty()&&line.front()==L'|')line.erase(0,1);if(!line.empty()&&line.back()==L'|')line.pop_back();
 std::vector<std::wstring> out;std::wstring cell;bool escape=false;for(auto c:line){if(c==L'|'&&!escape){out.push_back(legalTrim(cell));cell.clear();}else{if(escape&&c!=L'|')cell+=L'\\';if(c==L'\\'&&!escape){escape=true;continue;}cell+=c;}escape=false;}if(escape)cell+=L'\\';out.push_back(legalTrim(cell));return out;
}
inline std::vector<LegalBlock> legalBlocks(const std::string& raw){
 if(raw.empty()||raw.size()>128*1024)throw std::runtime_error("동의 문서 크기를 확인하세요.");auto text=wide(raw);std::vector<LegalBlock> blocks;std::wstring paragraph;size_t lines=0;
 auto flush=[&]{if(!paragraph.empty()){blocks.push_back({{paragraph},false,false});paragraph.clear();}};
 for(size_t begin=0;begin<=text.size();){auto end=text.find(L'\n',begin);if(end==std::wstring::npos)end=text.size();auto line=legalTrim(text.substr(begin,end-begin));if(++lines>4096)throw std::runtime_error("동의 문서 행 수를 확인하세요.");for(auto& c:line)if(c<32&&c!=L'\t')c=L' ';
  if(line.empty())flush();else if(line.find(L'|')!=std::wstring::npos){flush();auto cells=legalCells(line);bool separator=cells.size()>1;for(auto cell:cells)separator&=!cell.empty()&&cell.find(L'-')!=std::wstring::npos&&cell.find_first_not_of(L"-: ")==std::wstring::npos;
   if(separator){if(!blocks.empty()&&blocks.back().table)blocks.back().heading=true;}else if(cells.size()>1&&cells.size()<=8)blocks.push_back({cells,false,true});else blocks.push_back({{line},false,false});
  }else if(line.front()==L'#'){flush();size_t count=line.find_first_not_of(L'#');if(count!=std::wstring::npos&&count<=6&&line[count]==L' ')blocks.push_back({{legalTrim(line.substr(count))},true,false});else blocks.push_back({{line},false,false});
  }else if(line.rfind(L"- ",0)==0||line.rfind(L"* ",0)==0){flush();blocks.push_back({{L"• "+line.substr(2)},false,false});}
  else{if(!paragraph.empty())paragraph+=L" ";paragraph+=line;}if(end==text.size())break;begin=end+1;
 }flush();if(blocks.empty())throw std::runtime_error("동의 문서가 비어 있습니다.");return blocks;
}
struct LegalView {
 struct Row {LegalBlock block;int y=0,height=0;bool stacked=false;std::vector<int> heights;};
 std::wstring accessibleText;HWND window=nullptr;HFONT normal=nullptr,bold=nullptr;std::vector<Row> rows;int scroll=0,total=0,width=0;float scale=1;
 explicit LegalView(const std::string& text):accessibleText(wide(text)){for(auto& block:legalBlocks(text))rows.push_back({block});}
 ~LegalView(){if(normal)DeleteObject(normal);if(bold)DeleteObject(bold);}
 int px(float value)const{return std::max(1,int(value*scale));}
 void fonts(){if(normal)DeleteObject(normal);if(bold)DeleteObject(bold);scale=GetDpiForWindow(window)/96.f;normal=CreateFontW(-px(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");bold=CreateFontW(-px(15),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");}
 void layout(){RECT bounds;GetClientRect(window,&bounds);width=bounds.right;int available=std::max(px(40),width-px(24)),y=px(12);HDC dc=GetDC(window);
  for(auto& row:rows){row.y=y;row.stacked=row.block.table&&available<int(row.block.cells.size())*px(180);row.heights.clear();row.height=0;SelectObject(dc,row.block.heading?bold:normal);int cellWidth=row.stacked?available:available/int(row.block.cells.size());
   for(auto& cell:row.block.cells){RECT rect{0,0,std::max(px(20),cellWidth-px(16)),0};DrawTextW(dc,cell.c_str(),int(cell.size()),&rect,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);int height=std::max(px(22),int(rect.bottom))+px(14);row.heights.push_back(height);row.height=row.stacked?row.height+height:std::max(row.height,height);}y+=row.height+(row.block.table?0:px(10));
  }ReleaseDC(window,dc);total=y+px(12);scroll=std::clamp(scroll,0,std::max(0,total-int(bounds.bottom)));SCROLLINFO info{sizeof info,SIF_RANGE|SIF_PAGE|SIF_POS,0,std::max(0,total-1),UINT(bounds.bottom),scroll,0};SetScrollInfo(window,SB_VERT,&info,TRUE);InvalidateRect(window,nullptr,FALSE);
 }
 void move(int value){RECT rect;GetClientRect(window,&rect);scroll=std::clamp(value,0,std::max(0,total-int(rect.bottom)));SetScrollPos(window,SB_VERT,scroll,TRUE);InvalidateRect(window,nullptr,FALSE);}
 void draw(HDC dc){RECT bounds;GetClientRect(window,&bounds);HBRUSH background=CreateSolidBrush(RGB(22,22,24));FillRect(dc,&bounds,background);DeleteObject(background);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(238,238,242));HPEN pen=CreatePen(PS_SOLID,1,RGB(74,74,80));auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));int available=std::max(px(40),width-px(24));
  for(auto& row:rows){int top=row.y-scroll;if(top+row.height<0||top>bounds.bottom)continue;SelectObject(dc,row.block.heading?bold:normal);int y=top;for(size_t i=0;i<row.block.cells.size();i++){int left=px(12)+(row.stacked?0:int(i)*available/int(row.block.cells.size())),right=px(12)+(row.stacked?available:int(i+1)*available/int(row.block.cells.size())),bottom=y+(row.stacked?row.heights[i]:row.height);RECT box{left,y,right,bottom};if(row.block.table)Rectangle(dc,box.left,box.top,box.right,box.bottom);RECT text{left+px(8),y+px(7),right-px(8),bottom-px(7)};DrawTextW(dc,row.block.cells[i].c_str(),int(row.block.cells[i].size()),&text,DT_WORDBREAK|DT_NOPREFIX);if(row.stacked)y=bottom;}}
  SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);
 }
 void paint(){PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);draw(dc);EndPaint(window,&ps);}
 static LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM a,LPARAM b){auto self=(LegalView*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);if(message==WM_NCCREATE){self=(LegalView*)((CREATESTRUCTW*)b)->lpCreateParams;self->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)self);}
  if(!self)return DefWindowProcW(hwnd,message,a,b);if(message==WM_CREATE){self->fonts();return 0;}if(message==WM_SIZE){self->layout();return 0;}if(message==WM_DPICHANGED_AFTERPARENT){self->fonts();self->layout();return 0;}if(message==WM_PAINT){self->paint();return 0;}if(message==WM_PRINTCLIENT){self->draw((HDC)a);return 0;}if(message==WM_ERASEBKGND)return 1;
  if(message==WM_MOUSEWHEEL){self->move(self->scroll-GET_WHEEL_DELTA_WPARAM(a)*self->px(72)/WHEEL_DELTA);return 0;}
  if(message==WM_VSCROLL){SCROLLINFO info{sizeof info,SIF_TRACKPOS};GetScrollInfo(hwnd,SB_VERT,&info);RECT rect;GetClientRect(hwnd,&rect);int next=self->scroll;switch(LOWORD(a)){case SB_LINEUP:next-=self->px(24);break;case SB_LINEDOWN:next+=self->px(24);break;case SB_PAGEUP:next-=rect.bottom;break;case SB_PAGEDOWN:next+=rect.bottom;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:next=info.nTrackPos;break;case SB_TOP:next=0;break;case SB_BOTTOM:next=self->total;break;}self->move(next);return 0;}
  if(message==WM_KEYDOWN){if(a==VK_HOME)self->move(0);else if(a==VK_END)self->move(self->total);else if(a==VK_UP||a==VK_DOWN)self->move(self->scroll+(a==VK_UP?-1:1)*self->px(24));else if(a==VK_PRIOR||a==VK_NEXT){RECT rect;GetClientRect(hwnd,&rect);self->move(self->scroll+(a==VK_PRIOR?-1:1)*rect.bottom);}else return DefWindowProcW(hwnd,message,a,b);return 0;}
  if(message==WM_GETDLGCODE)return DLGC_WANTARROWS;return DefWindowProcW(hwnd,message,a,b);
 }
 HWND create(HWND parent,HINSTANCE instance){static std::once_flag once;std::call_once(once,[&]{WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=L"SnuwLegalDocument";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("동의 문서 화면 초기화 실패");});auto result=CreateWindowExW(WS_EX_CLIENTEDGE,L"SnuwLegalDocument",accessibleText.c_str(),WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_TABSTOP,0,0,100,100,parent,nullptr,instance,this);if(!result)throw std::runtime_error("동의 문서 화면 표시 실패");return result;}
};
struct LegalDialog {
 LegalView document;std::wstring revision;HWND owner=nullptr,window=nullptr,heading=nullptr,required=nullptr,optional=nullptr,agree=nullptr,cancel=nullptr;bool accepted=false,hardware=false;HBRUSH background=CreateSolidBrush(RGB(26,26,28));
 LegalDialog(const std::string& text,const std::string& version):document(text),revision(L"이용동의 · 버전 "+wide(version)){}
 ~LegalDialog(){if(background)DeleteObject(background);}
 void layout(){RECT r;GetClientRect(window,&r);int w=r.right,h=r.bottom;auto p=[&](int n){return document.px(float(n));};int gap=p(16),width=std::max(p(100),w-gap*2);MoveWindow(heading,gap,p(12),width,p(48),TRUE);MoveWindow(document.window,gap,p(68),width,std::max(p(80),h-p(196)),TRUE);MoveWindow(required,gap,h-p(118),width,p(32),TRUE);MoveWindow(optional,gap,h-p(84),width,p(32),TRUE);MoveWindow(agree,std::max(gap,w-p(246)),h-p(42),p(136),p(30),TRUE);MoveWindow(cancel,std::max(gap,w-p(102)),h-p(42),p(86),p(30),TRUE);}
 static LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM a,LPARAM b){auto self=(LegalDialog*)GetWindowLongPtrW(hwnd,GWLP_USERDATA);if(message==WM_NCCREATE){self=(LegalDialog*)((CREATESTRUCTW*)b)->lpCreateParams;self->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,(LONG_PTR)self);}if(!self)return DefWindowProcW(hwnd,message,a,b);
  if(message==WM_CREATE){try{auto instance=GetModuleHandleW(nullptr);self->document.create(hwnd,instance);auto control=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int id){auto h=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,10,10,hwnd,(HMENU)(INT_PTR)id,instance,nullptr);if(!h)throw std::runtime_error("동의 화면 컨트롤 초기화 실패");SendMessageW(h,WM_SETFONT,(WPARAM)self->document.normal,TRUE);return h;};
   self->heading=control(L"STATIC",self->revision.c_str(),0,0);self->required=control(L"BUTTON",L"[필수] 위 이용·개인정보 안내에 동의합니다.",BS_AUTOCHECKBOX|BS_MULTILINE|WS_TABSTOP,101);self->optional=control(L"BUTTON",L"[선택] OS·CPU·RAM·GPU·VRAM 정보 제공",BS_AUTOCHECKBOX|BS_MULTILINE|WS_TABSTOP,102);self->agree=control(L"BUTTON",L"동의하고 계속",BS_PUSHBUTTON|WS_TABSTOP,IDOK);self->cancel=control(L"BUTTON",L"취소",BS_DEFPUSHBUTTON|WS_TABSTOP,IDCANCEL);EnableWindow(self->agree,FALSE);return 0;}catch(...){return -1;}
  }
  if(message==WM_SIZE&&self->heading){self->layout();return 0;}if(message==WM_GETMINMAXINFO){auto info=(MINMAXINFO*)b;info->ptMinTrackSize={self->document.px(460),self->document.px(440)};return 0;}
  if(message==WM_DPICHANGED){auto rect=(RECT*)b;SetWindowPos(hwnd,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(self->document.window,WM_DPICHANGED_AFTERPARENT,0,0);for(auto h:{self->heading,self->required,self->optional,self->agree,self->cancel})SendMessageW(h,WM_SETFONT,(WPARAM)self->document.normal,TRUE);self->layout();return 0;}
  if(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLORBTN){SetTextColor((HDC)a,RGB(238,238,242));SetBkColor((HDC)a,RGB(26,26,28));return (LRESULT)self->background;}
  if(message==WM_ERASEBKGND){RECT rect;GetClientRect(hwnd,&rect);FillRect((HDC)a,&rect,self->background);return 1;}
  if(message==WM_COMMAND){auto id=LOWORD(a);if(id==101){EnableWindow(self->agree,SendMessageW(self->required,BM_GETCHECK,0,0)==BST_CHECKED);return 0;}if(id==IDOK){if(SendMessageW(self->required,BM_GETCHECK,0,0)!=BST_CHECKED)return 0;self->accepted=true;self->hardware=SendMessageW(self->optional,BM_GETCHECK,0,0)==BST_CHECKED;DestroyWindow(hwnd);return 0;}if(id==IDCANCEL){DestroyWindow(hwnd);return 0;}}
  if(message==WM_CLOSE){DestroyWindow(hwnd);return 0;}return DefWindowProcW(hwnd,message,a,b);
 }
 HWND create(bool visible=true){static std::once_flag once;auto instance=GetModuleHandleW(nullptr);std::call_once(once,[&]{WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=L"SnuwLegalReview";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("동의 화면 초기화 실패");});
  RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);float dpi=GetDpiForSystem()/96.f;int w=std::min(int(820*dpi),int(work.right-work.left)),h=std::min(int(820*dpi),int(work.bottom-work.top));
  auto result=CreateWindowExW(WS_EX_CONTROLPARENT,L"SnuwLegalReview",L"SNUWCANVAS · 이용동의",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MAXIMIZEBOX,(work.left+work.right-w)/2,(work.top+work.bottom-h)/2,w,h,owner,nullptr,instance,this);if(!result)throw std::runtime_error("동의 화면 표시 실패");layout();if(visible){ShowWindow(result,SW_SHOWNORMAL);SetFocus(document.window);}return result;
 }
 bool run(){owner=GetActiveWindow();bool wasEnabled=owner&&IsWindowEnabled(owner);create();if(wasEnabled)EnableWindow(owner,FALSE);MSG message;while(IsWindow(window)){int result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(result==0)PostQuitMessage(int(message.wParam));if(IsWindow(window))DestroyWindow(window);break;}if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}if(wasEnabled&&IsWindow(owner)){EnableWindow(owner,TRUE);SetActiveWindow(owner);}return accepted;}

};
