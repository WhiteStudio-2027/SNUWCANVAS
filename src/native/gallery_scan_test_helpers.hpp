#pragma once
namespace snuw {
inline void settleGallery(App& app){auto deadline=GetTickCount64()+30000;while(app.galleryActive||app.galleryQueued){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}app.galleryTick();if(GetTickCount64()>deadline)throw std::runtime_error("Gallery scan did not settle");Sleep(1);}}
}
