#include <windows.h>
#include <iostream>
#include "WinBase.h"

#define internal_func static // the function marked by static keyword are local and inaccessible from other files
#define local_persist static
#define global_variable static


global_variable bool g_appRunning;
global_variable BITMAPINFO g_bitmapInfo;
global_variable void *g_bitmapBuffer;
global_variable HBITMAP g_bitmapHandle;
global_variable HDC g_bitmapDeviceContext;


// DIB = Device Independent Bitmap 
internal_func void win32ResizeDIBSection(int width, int height){
    //TODO: bullet proof this. maybe allocate first and then free? 

    //free the previous DI bitmap 
    if(g_bitmapHandle){
        // if we had a bitmap
        DeleteObject(g_bitmapHandle);
    }

    if(!g_bitmapDeviceContext){
        g_bitmapDeviceContext = CreateCompatibleDC(0);
    }

    g_bitmapInfo.bmiHeader.biSize = sizeof(g_bitmapInfo.bmiHeader);
    g_bitmapInfo.bmiHeader.biWidth = width;
    g_bitmapInfo.bmiHeader.biHeight = height;
    g_bitmapInfo.bmiHeader.biPlanes = 1;
    g_bitmapInfo.bmiHeader.biBitCount = 32; // 4 byte int values for rgba
    g_bitmapInfo.bmiHeader.biCompression = BI_RGB;
    g_bitmapInfo.bmiHeader.biSizeImage = 0;
    g_bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    g_bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    g_bitmapInfo.bmiHeader.biClrUsed = 0;
    g_bitmapInfo.bmiHeader.biClrImportant = 0;


    
    
    g_bitmapHandle = CreateDIBSection(
        g_bitmapDeviceContext, 
        &g_bitmapInfo,
        DIB_RGB_COLORS,
        &g_bitmapBuffer,
        0,
        0
    );
}

internal_func void win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height){
    StretchDIBits(
        deviceContext,
        x, y, width, height, // window canvas
        x, y, width, height, // source back buffer   
        g_bitmapBuffer,
        &g_bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// we register a callback to windows for when a change happens to our window, it will let us know 
LRESULT CALLBACK mainWindowCallback(HWND window,UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT _result = 0;

    switch(message){
        case WM_PAINT:{
            PAINTSTRUCT paintStruct; // we store the new information about our window in this struct
            HDC deviceContext = BeginPaint(window, &paintStruct); // this will give us a handle so we can paint in the window
            LONG _x = paintStruct.rcPaint.left;
            LONG _y = paintStruct.rcPaint.top;
            LONG _width = paintStruct.rcPaint.right - paintStruct.rcPaint.left;
            LONG _height = paintStruct.rcPaint.bottom - paintStruct.rcPaint.top;
            win32UpdateWindow(deviceContext, _x, _y, _width, _height);
            EndPaint(window, &paintStruct);
        } break;

        case WM_SIZE:{
            RECT canvas;
            GetClientRect(window, &canvas);
            LONG _width = canvas.right - canvas.left;
            LONG _height = canvas.bottom - canvas.top;
            win32ResizeDIBSection(_width, _height);
            OutputDebugString("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:{
            // TODO: handle this by showing a message to user
            g_appRunning = false;
            OutputDebugString("WM_DESTROY\n");
        } break;
        
        case WM_CLOSE:{
            //PostQuitMessage(0); // tell windows that we want to close the window and application
            g_appRunning = false; // TODO: handle this by re-creating the window
            OutputDebugString("WM_CLOSE\n");
        } break;
        
        case WM_ACTIVATEAPP:{
            OutputDebugString("WM_ACTIVATEAPP\n");
        } break;
        
        default:{
            // we call to windows to handle any message that we are not listening for
            _result = DefWindowProc(window, message, wParam, lParam); 
        } break;
    }
    return _result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    
    WNDCLASS windowClass = {};

    // TODO: check if HREDRAW or VREDRAW still matter! 

    windowClass.style = CS_OWNDC | CS_HREDRAW; // we can have our own Device Class (DC) resource from Windows kernel 
    windowClass.lpfnWndProc = mainWindowCallback;
    windowClass.hInstance = instance; // or we can use GetModuleName(0) if we don't have a hInstance passed to us
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    // we register our window class configuration to windows kernel 
    ATOM windowResult = RegisterClass(&windowClass);
    if(windowResult){
        HWND windowHandle = CreateWindowEx(
            0, 
            windowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, // x 
            CW_USEDEFAULT, // y
            CW_USEDEFAULT, // width
            CW_USEDEFAULT, // height
            0,
            0,
            instance,
            0
        );
        if(windowHandle){

            g_appRunning = true;
            // we read the messages sent by kernel or other apps to our window
            // these messages are stored in a message queue
            MSG messageFromWInKernel;
            while (g_appRunning)
            {
                BOOL messageResult = GetMessage(&messageFromWInKernel, 0, 0, 0);
                if(messageResult > 0){
                    TranslateMessage(&messageFromWInKernel); // more windows crap! 
                    DispatchMessage(&messageFromWInKernel); // again, more more windows crap!  
                }
                else{
                    OutputDebugString("Window is closed");
                    break;
                }
           }
        } 
        else{
             // TODO: Failure! need logging
        }
    }
    else {
        // TODO: Failure! need logging
    }
    return 0;
}