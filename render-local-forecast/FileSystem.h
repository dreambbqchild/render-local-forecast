#pragma once
#include <d2d1.h>
#include <wincodec.h>

#include <string>
#include <mfidl.h>
#include <mfreadwrite.h>

const UINT imageWidth = 1060;
const UINT imageHeight = 1100;

HRESULT LoadD2DBitmapFromFile(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, std::wstring path, ID2D1Bitmap** ppD2DBitmap);
HRESULT LoadWxBitmapFromFile(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, std::wstring path, IWICBitmap** ppWICBitmap);
std::wstring GetTextForecast(std::wstring pathToRenderingsFolder, int index);
HRESULT SelectRandomMusic(IMFSourceReader** pSourceReader);