#include <windows.h>
#include <iostream>


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
            // for debugging and fun purposes
            static DWORD _operation = WHITENESS;
            if (_operation == WHITENESS){
                _operation = BLACKNESS;
            }else{
                _operation = WHITENESS;
            } //end fun :(
            PatBlt(deviceContext,_x, _y, _width, _height, _operation); // fills a rectangle with white color! 
            EndPaint(window, &paintStruct);
        } break;

        case WM_SIZE:{
            OutputDebugString("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:{
            OutputDebugString("WM_DESTROY\n");
        } break;
        
        case WM_CLOSE:{
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
           // we read the messages sent by kernel or other apps to our window 
           // these messages are stored in a message queue 
           MSG messageFromWInKernel;
           while(true)
           {
                BOOL messageResult = GetMessage(&messageFromWInKernel, 0, 0, 0);
                if(messageResult > 0){
                    TranslateMessage(&messageFromWInKernel); // more windows crap! 
                    DispatchMessage(&messageFromWInKernel); // again, more more windows crap!  
                }
                else{
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