#include <stdint.h>
#include <windows.h>
#include <WinBase.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define internal_func static  // the function marked by static keyword are local and inaccessible from other files
#define local_persist static
#define global_variable static

global_variable bool g_appRunning;
global_variable BITMAPINFO g_bitmapInfo;
global_variable void *g_bitmapBuffer;
global_variable int g_bitmapWidth;
global_variable int g_bitmapHeight;
global_variable int g_bytePerPixel;

internal_func void renderGradient(float offset) { 
    uint8 *pixelBitmapBytes = (uint8 *)g_bitmapBuffer;
    int pixelRowSize = g_bitmapWidth * g_bytePerPixel;  // how many bytes are in a pixel row
    for (int y = 0; y < g_bitmapHeight; y++) {
        // uint32 *pixelRow = (uint32 *) pixelBitmapBytes; // get a row of pixels (each pixel is 32 bits)
        uint32 *pixel = (uint32 *)pixelBitmapBytes;  // get a row of pixels (each pixel is 32 bits)

        for (int x = 0; x < g_bitmapWidth; x++) {
            uint8 blue = 0xff;
            uint8 green = (((float)y / g_bitmapHeight) * offset * 255);
            uint8 red = (((float)y / g_bitmapHeight) * (1 - offset) * 255);

            *pixel = (red << 16) | (green << 8) | blue;
            *pixel++;
        }
        pixelBitmapBytes += pixelRowSize;
    }
}

// DIB = Device Independent Bitmap
internal_func void win32ResizeDIBSection(int width, int height) {
    // free the bitmap buffer memory page(s)
    if (g_bitmapBuffer) {
        VirtualFree(g_bitmapBuffer, 0, MEM_RELEASE);
    }

    g_bitmapWidth = width;
    g_bitmapHeight = height;

    g_bitmapInfo.bmiHeader.biSize = sizeof(g_bitmapInfo.bmiHeader);
    g_bitmapInfo.bmiHeader.biWidth = g_bitmapWidth;
    // we make this negative because we want to our coordinate system (back buffer) start at top-left corner
    g_bitmapInfo.bmiHeader.biHeight = -g_bitmapHeight;
    g_bitmapInfo.bmiHeader.biPlanes = 1;
    g_bitmapInfo.bmiHeader.biBitCount = 32;  // 4 byte int values for rgba
    g_bitmapInfo.bmiHeader.biCompression = BI_RGB;
    g_bitmapInfo.bmiHeader.biSizeImage = 0;
    g_bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    g_bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    g_bitmapInfo.bmiHeader.biClrUsed = 0;
    g_bitmapInfo.bmiHeader.biClrImportant = 0;

    g_bytePerPixel = 4;  // 4 RGBp where p=padding required to make it aligned ti 32/64 bit processors
    int bufferSize = (width * height) * g_bytePerPixel;
    // we ask windows kernel to give us page(s) of memory
    g_bitmapBuffer = VirtualAlloc(
        0,  // start of the memory page
        bufferSize,
        MEM_COMMIT,     // we need the memory now and want to write to it
        PAGE_READWRITE  // access type to our page
    );
}

internal_func void win32UpdateWindow(HDC deviceContext, RECT windowRect, int x, int y, int width, int height) {
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    StretchDIBits(
        deviceContext,
        // original mapping from source buffer to destination window canvas
        /*
         x, y, width, height, // window canvas
        x, y, width, height, // source back buffer
        */

        // TODO: i don't know why casey did this yet!
        0, 0, g_bitmapWidth, g_bitmapHeight,
        0, 0, windowWidth, windowHeight,

        g_bitmapBuffer,
        &g_bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

// we register a callback to windows for when a change happens to our window, it will let us know
LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT _result = 0;

    switch (message) {
        case WM_PAINT: {
            RECT canvas;
            PAINTSTRUCT paintStruct;  // we store the new information about our window in this struct
            GetClientRect(window, &canvas);
            HDC deviceContext = BeginPaint(window, &paintStruct);  // this will give us a handle so we can paint in the window
            LONG _x = paintStruct.rcPaint.left;
            LONG _y = paintStruct.rcPaint.top;
            LONG _width = paintStruct.rcPaint.right - paintStruct.rcPaint.left;
            LONG _height = paintStruct.rcPaint.bottom - paintStruct.rcPaint.top;
            win32UpdateWindow(deviceContext, canvas, _x, _y, _width, _height);
            EndPaint(window, &paintStruct);
        } break;

        case WM_SIZE: {
            RECT canvas;
            GetClientRect(window, &canvas);
            LONG _width = canvas.right - canvas.left;
            LONG _height = canvas.bottom - canvas.top;
            win32ResizeDIBSection(_width, _height);
            OutputDebugString("WM_SIZE\n");
        } break;

        case WM_DESTROY: {
            // TODO: handle this by showing a message to user
            g_appRunning = false;
            OutputDebugString("WM_DESTROY\n");
        } break;

        case WM_CLOSE: {
            // PostQuitMessage(0); // tell windows that we want to close the window and application
            g_appRunning = false;  // TODO: handle this by re-creating the window
            OutputDebugString("WM_CLOSE\n");
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugString("WM_ACTIVATEAPP\n");
        } break;

        default: {
            // we call to windows to handle any message that we are not listening for
            _result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }
    return _result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC | CS_HREDRAW;  // we can have our own Device Class (DC) resource from Windows kernel
    windowClass.lpfnWndProc = mainWindowCallback;
    windowClass.hInstance = instance;  // or we can use GetModuleName(0) if we don't have a hInstance passed to us
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    // we register our window class configuration to windows kernel
    ATOM windowResult = RegisterClass(&windowClass);
    if (windowResult) {
        HWND windowHandle = CreateWindowEx(
            0,
            windowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,  // x
            CW_USEDEFAULT,  // y
            CW_USEDEFAULT,  // width
            CW_USEDEFAULT,  // height
            0,
            0,
            instance,
            0);
        if (windowHandle) {
            g_appRunning = true;
            float animationOffset = 0.0f;
            float offsetValue = 0.0050f;
            // we read the messages sent by kernel or other apps to our window
            // these messages are stored in a message queue
            MSG messageFromWInKernel;
            while (g_appRunning) {
                // PeekMessage does not block when there are not messages!
                while (PeekMessage(&messageFromWInKernel, 0, 0, 0, PM_REMOVE)) {
                    if (messageFromWInKernel.message == WM_QUIT) {
                        g_appRunning = false;
                    }
                    TranslateMessage(&messageFromWInKernel);  // more windows crap!
                    DispatchMessage(&messageFromWInKernel);   // again, more more windows crap!
                }

                // game loop apparently!
                animationOffset += offsetValue;
                if (animationOffset > 1.0) {
                    animationOffset = 1.0f;
                    offsetValue = -offsetValue;
                }
                if (animationOffset < 0.0f) {
                    animationOffset = 0.0f;
                    offsetValue = -offsetValue;
                }

                renderGradient(animationOffset);

                HDC deviceContext = GetDC(windowHandle);
                RECT windowRect;
                GetClientRect(windowHandle, &windowRect);
                int windowWidth = windowRect.right - windowRect.left;
                int windowHeight = windowRect.bottom - windowRect.top;
                win32UpdateWindow(deviceContext, windowRect, 0, 0, windowWidth, windowHeight);
                ReleaseDC(windowHandle, deviceContext);
            }
        } else {
            // TODO: Failure! need logging
        }
    } else {
        // TODO: Failure! need logging
    }
    return 0;
}