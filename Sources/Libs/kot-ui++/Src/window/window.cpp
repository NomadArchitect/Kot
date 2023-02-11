#include <kot-ui++/window.h>

#include <kot-graphics/orb.h>

#include <kot/uisd/srvs/system.h>

#include <kot-ui++/component.h>

namespace UiWindow {

    void EventHandler(enum Window_Event EventType, uint64_t GP0, uint64_t GP1, uint64_t GP2, uint64_t GP3, uint64_t GP4){
        Window* Wid = (Window*)Sys_GetExternalDataThread();
        Wid->Handler(EventType, GP0, GP1, GP2, GP3, GP4);
        Sys_Event_Close();
    }

    Window::Window(char* title, uint32_t Width, uint32_t Height, uint32_t XPosition, uint32_t YPosition){
        // Setup event
        Sys_Event_Create(&WindowEvent);
        Sys_Createthread(Sys_GetProcess(), (uintptr_t)&EventHandler, PriviledgeApp, (uint64_t)this, &WindowHandlerThread);
        Sys_Event_Bind(WindowEvent, WindowHandlerThread, false);
        IsListeningEvents = false;

        // Setup window
        this->Wid = CreateWindow(WindowEvent, Window_Type_Default);
        ResizeWindow(this->Wid, Width + 2, Height + 2);
        WindowChangePosition(this->Wid, XPosition, YPosition);

        IsBorders = true;
        if(IsBorders){
            BordersCtx = CreateGraphicContext(&Wid->Framebuffer);
            framebuffer_t FramebufferWithoutBorder;
            FramebufferWithoutBorder.Bpp = Wid->Framebuffer.Bpp;
            FramebufferWithoutBorder.Btpp = Wid->Framebuffer.Btpp;
            FramebufferWithoutBorder.Width = Wid->Framebuffer.Width - 2;
            FramebufferWithoutBorder.Height = Wid->Framebuffer.Height - 2;
            FramebufferWithoutBorder.Pitch = Wid->Framebuffer.Pitch;
            FramebufferWithoutBorder.Buffer = (uintptr_t)((uint64_t)Wid->Framebuffer.Buffer + Wid->Framebuffer.Btpp + Wid->Framebuffer.Pitch);
            DrawBorders(WIN_BDCOLOR_ONBLUR);

            GraphicCtx = CreateGraphicContext(&FramebufferWithoutBorder);
            UiCtx = new Ui::UiContext(&FramebufferWithoutBorder);
        }else{
            GraphicCtx = CreateGraphicContext(&Wid->Framebuffer);
            UiCtx = new Ui::UiContext(&Wid->Framebuffer);
        }

        // Test

        /* auto imgtest = Ui::Picturebox("kotlogo.tga", Ui::ImageType::_TGA, { .Width = 256, .Height = 256 });
        this->SetContent(imgtest); */

        Titlebar = Ui::Titlebar(title, { .BackgroundColor = WIN_BGCOLOR_ONFOCUS, .ForegroundColor = WIN_TBCOLOR_ONBLUR });
        this->SetContent(Titlebar);
 
/*         auto wrapper = Ui::Box({ .Width = this->UiCtx->fb->Width, .Height = this->UiCtx->fb->Height - titlebar->GetStyle()->Height, .color = WIN_BGCOLOR_ONFOCUS });

        auto flexbox = UiLayout::Flexbox({}, Ui::Layout::HORIZONTAL);

        auto Box = Ui::Box({ .Width = 20, .Height = 20, .color = 0xFFFF00 });
        flexbox->AddChild(box);

        wrapper->AddChild(flexbox);

        this->SetContent(wrapper); */
        ChangeVisibilityWindow(this->Wid, true);
        IsListeningEvents = true;
    }

    void Window::DrawBorders(uint32_t Color){
        if(IsBorders){
            ctxDrawRect(BordersCtx, 0, 0, BordersCtx->Width - 1, BordersCtx->Height - 1, Color);
        }
    }

    void Window::SetContent(Ui::Component* content) {
        Ui::Component* windowCpnt = this->UiCtx->Cpnt;

        windowCpnt->AddChild(content);
        windowCpnt->Update();
    }

    void Window::Handler(enum Window_Event EventType, uint64_t GP0, uint64_t GP1, uint64_t GP2, uint64_t GP3, uint64_t GP4){
        if(IsListeningEvents){
            switch (EventType){
                case Window_Event_Focus:
                    HandlerFocus(GP0);
                    break;
                case Window_Event_Mouse:
                    HandlerMouse(GP0, GP1, GP2, GP3);
                    break;
                case Window_Event_Keyboard:
                    // TODO
                    break;
                default:
                    break;
            }
        }
    }

    void Window::HandlerFocus(bool IsFocus){
        if(IsFocus){
            DrawBorders(WIN_BDCOLOR_ONFOCUS);
            Titlebar->GetStyle()->BackgroundColor = WIN_TBCOLOR_ONFOCUS;
        }else{
            DrawBorders(WIN_BDCOLOR_ONBLUR);
            Titlebar->GetStyle()->BackgroundColor = WIN_TBCOLOR_ONBLUR;
        }

        Titlebar->Update();
        this->UiCtx->Cpnt->Update();
    }

    void Window::HandlerMouse(uint64_t PositionX, uint64_t PositionY, uint64_t ZValue, uint64_t Status){
        int64_t RelativePostionX = PositionX - Wid->Position.x;
        int64_t RelativePostionY = PositionY - Wid->Position.y; 

        if(RelativePostionX >= 0 && RelativePostionY >= 0 && RelativePostionX < Wid->Framebuffer.Width && RelativePostionY < Wid->Framebuffer.Height){
            Ui::Component* Component = (Ui::Component*)GetEventData(UiCtx->EventBuffer, RelativePostionX, RelativePostionY);
            if(Component){
                Component->MouseEvent(RelativePostionX, RelativePostionY, PositionX, PositionY, ZValue, Status);
            }
        }
    }

}