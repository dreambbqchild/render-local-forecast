#include "FileSystem.h"
#include <wrl.h>

#include <filesystem>
#include <random>
#include <vector>

using namespace Microsoft::WRL;
using namespace std;
namespace fs = std::filesystem;

HRESULT LoadD2DBitmapFromFile(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, std::wstring path, ID2D1Bitmap** ppD2DBitmap)
{
    ComPtr<IWICBitmapDecoder> pDecoder;
    ComPtr<IWICBitmapFrameDecode> pSource;
    ComPtr<IWICFormatConverter> pConverter;
    ComPtr<IWICBitmap> pWicBitmap;
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    UINT width = 0, height = 0;

    HRESULT hr = pWICFactory->CreateDecoderFromFilename(path.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);

    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(0, &pSource);

    if (SUCCEEDED(hr))
        hr = pSource->GetSize(&width, &height);

    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateFormatConverter(&pConverter);

    if (SUCCEEDED(hr))
        hr = pConverter->Initialize(pSource.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
    
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, &pWicBitmap);

    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(pWicBitmap.Get(), D2D1::RenderTargetProperties(), &pRenderTarget);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter.Get(), ppD2DBitmap);

    return hr;
}

HRESULT LoadWxBitmapFromFile(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, std::wstring path, IWICBitmap** ppWICBitmap)
{
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    ComPtr<ID2D1Bitmap> pD2DBitmap;    

    HRESULT hr = pWICFactory->CreateBitmap(imageWidth, imageHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, ppWICBitmap);

    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(*ppWICBitmap, D2D1::RenderTargetProperties(), &pRenderTarget);

    if (SUCCEEDED(hr))
        hr = LoadD2DBitmapFromFile(pWICFactory, pD2DFactory, path, &pD2DBitmap);

    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(53 / 255.0f, 56 / 255.0f, 62 / 255.0f));
        pRenderTarget->DrawBitmap(pD2DBitmap.Get(), D2D1::RectF(0, 40, 1060, 1060), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, 1060, 1060));
        pRenderTarget->EndDraw();
    }

    return hr;
}

std::wstring GetTextForecast(std::wstring pathToRenderingsFolder, int index)
{
    FILE* f;
    wchar_t cFileName[MAX_PATH] = { 0 };
    swprintf_s(cFileName, L"%s/%02d.txt", pathToRenderingsFolder.c_str(), index);

    _wfopen_s(&f, cFileName, L"r, ccs=UTF-8");
    if (f == NULL)
        return std::wstring();

    fseek(f, 0, SEEK_END);
    long length = ftell(f) * 2;
    fseek(f, 0, SEEK_SET);
    std::wstring buffer(length, '\0');
    fread((char*)buffer.c_str(), length, 1, f);
    fclose(f);
    return buffer;
}

HRESULT SelectRandomMusic(IMFSourceReader** pSourceReader)
{
    std::wstring path(MAX_PATH, '\0');
    GetModuleFileName(NULL, (wchar_t*)path.c_str(), MAX_PATH);
    path = fs::path(path).parent_path().parent_path().parent_path();
    path += L"\\music";

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(path))
        files.push_back(entry.path());

    std::random_device randomDevice;
    std::default_random_engine randomEngine(randomDevice());
    std::uniform_int_distribution<int> range(0, files.size() - 1);
    auto index = range(randomEngine);

    return MFCreateSourceReaderFromURL(files.at(index).c_str(), NULL, pSourceReader);
}