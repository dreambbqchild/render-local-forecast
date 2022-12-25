#pragma once
#include "ImageFunctions.h"

const INT32 totalForecasts = 10;

HRESULT RenderGifForecast(std::wstring pathToGifOutput, std::wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory);
HRESULT RenderVideoForecast(std::wstring pathToRenderingsFolder, std::wstring pathToForecastsFolder, std::wstring pathToMp4Output, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory);