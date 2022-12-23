#include "ForecastRenderers.h"
#include <wrl.h>
#include <mfapi.h>

using namespace Microsoft::WRL;
using namespace std;

struct VideoFps {
    UINT32 numerator;
    UINT32 denominator;
};

struct StreamIndexes {
    DWORD video;
    DWORD audio;
};

HRESULT InitializeSinkWriter(std::wstring pathToMp4Output, IMFSinkWriter** ppSinkWriter, IMFSourceReader** pReader, StreamIndexes* streamIndex, VideoFps fps, UINT32 frameDuration)
{
    const UINT32 bitRate = 1000000;
    const GUID encodingFormat = MFVideoFormat_H264;
    const GUID videoFormat = MFVideoFormat_RGB32;
    const GUID audioFormat = MFAudioFormat_AAC;

    ComPtr<IMFAttributes> attributes;
    ComPtr<IMFMediaType> pVideoOut;
    ComPtr<IMFMediaType> pVideoIn;
    ComPtr<IMFMediaType> pAudioOut;
    ComPtr<IMFMediaType> pAudioIn;

    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (SUCCEEDED(hr))
        hr = attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    if (SUCCEEDED(hr))
        hr = MFCreateSinkWriterFromURL(pathToMp4Output.c_str(), NULL, attributes.Get(), ppSinkWriter);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pVideoOut);
    if (SUCCEEDED(hr))
        hr = pVideoOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = pVideoOut->SetGUID(MF_MT_SUBTYPE, encodingFormat);
    if (SUCCEEDED(hr))
        hr = pVideoOut->SetUINT32(MF_MT_AVG_BITRATE, bitRate);
    if (SUCCEEDED(hr))
        hr = pVideoOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(pVideoOut.Get(), MF_MT_FRAME_SIZE, imageWidth, imageHeight);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pVideoOut.Get(), MF_MT_FRAME_RATE, fps.numerator, fps.denominator);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pVideoOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->AddStream(pVideoOut.Get(), &streamIndex->video);

    ComPtr<IMFMediaType> pSourceAudio;
    if (SUCCEEDED(hr))
        hr = SelectRandomMusic(pReader);

    if (SUCCEEDED(hr))
        hr = (*pReader)->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pSourceAudio);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pAudioOut);
    if (SUCCEEDED(hr))
        hr = pAudioOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr))
        hr = pAudioOut->SetGUID(MF_MT_SUBTYPE, audioFormat);
    if (SUCCEEDED(hr))
        hr = pSourceAudio->CopyAllItems(pAudioOut.Get());
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->AddStream(pAudioOut.Get(), &streamIndex->audio);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pVideoIn);
    if (SUCCEEDED(hr))
        hr = pVideoIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = pVideoIn->SetGUID(MF_MT_SUBTYPE, videoFormat);
    if (SUCCEEDED(hr))
        hr = pVideoIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(pVideoIn.Get(), MF_MT_FRAME_SIZE, imageWidth, imageHeight);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pVideoIn.Get(), MF_MT_FRAME_RATE, fps.numerator, fps.denominator);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pVideoIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->SetInputMediaType(streamIndex->video, pVideoIn.Get(), NULL);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pAudioIn);
    if (SUCCEEDED(hr))
        hr = pAudioIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr))
        hr = pAudioIn->SetGUID(MF_MT_SUBTYPE, audioFormat);
    if (SUCCEEDED(hr))
        hr = pSourceAudio->CopyAllItems(pAudioIn.Get());
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->SetInputMediaType(streamIndex->audio, pAudioIn.Get(), NULL);

    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->BeginWriting();

    return hr;
}

HRESULT RenderSingleVideoForecast(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IMFSinkWriter* pWriter, IWICBitmap* pWICBitmap, IMFSourceReader* pReader, StreamIndexes* streamIndex, INT32 frameDuration, LONGLONG& rtStart)
{
    ComPtr<IWICBitmapLock> pILock;
    ComPtr<IMFMediaBuffer> pBuffer;
    ComPtr<IWICBitmapFlipRotator> pFlipRotator;
    ComPtr<IWICBitmap> pWICFlippedBitmap;

    const LONG cbWidth = 4 * imageWidth;
    const DWORD cbBuffer = cbWidth * imageHeight;

    BYTE* pBitmapData = NULL;
    BYTE* pVideoData = NULL;
    WICRect rcLock = { 0, 0, imageWidth, imageHeight };
    UINT bitmapBufferSize = 0;

    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmapFlipRotator(&pFlipRotator);
    if (SUCCEEDED(hr))
        hr = pFlipRotator->Initialize(pWICBitmap, WICBitmapTransformFlipVertical);
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmapFromSourceRect(pFlipRotator.Get(), 0, 0, imageWidth, imageHeight, &pWICFlippedBitmap);
    if (SUCCEEDED(hr))
        hr = pBuffer->Lock(&pVideoData, NULL, NULL);
    if (SUCCEEDED(hr))
        hr = pWICFlippedBitmap->Lock(&rcLock, WICBitmapLockWrite, &pILock);
    if (SUCCEEDED(hr))
        hr = pILock->GetDataPointer(&bitmapBufferSize, &pBitmapData);
    if (SUCCEEDED(hr) && pVideoData && pBitmapData)
        hr = MFCopyImage(pVideoData, cbWidth, pBitmapData, cbWidth, cbWidth, imageHeight);
    if (pBuffer)
        hr = pBuffer->Unlock();

    if (SUCCEEDED(hr))
        hr = pBuffer->SetCurrentLength(cbBuffer);

    ComPtr<IMFSample> pSample;
    if (SUCCEEDED(hr))
        hr = MFCreateSample(&pSample);
    if (SUCCEEDED(hr))
        hr = pSample->AddBuffer(pBuffer.Get());

    if (SUCCEEDED(hr))
        hr = pSample->SetSampleTime(rtStart);
    if (SUCCEEDED(hr))
        hr = pSample->SetSampleDuration(frameDuration);

    if (SUCCEEDED(hr))
        hr = pWriter->WriteSample(streamIndex->video, pSample.Get());

    if (SUCCEEDED(hr))
        rtStart += frameDuration;

    LONGLONG timestamp = 0;
    do
    {
        DWORD actualIndex, flags;
        ComPtr<IMFSample> pSrcSample;
        if (SUCCEEDED(hr))
            hr = pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &actualIndex, &flags, &timestamp, &pSrcSample);

        if (SUCCEEDED(hr))
            hr = pWriter->WriteSample(streamIndex->audio, pSrcSample.Get());
    } while (timestamp < rtStart);


    return hr;
}

HRESULT RenderVideoForecast(std::wstring pathToRenderingsFolder, std::wstring pathToMp4Output, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory)
{
    const VideoFps fps = { 1, 5 };
    const UINT32 frameDuration = (UINT32)(10 * 1000 * 1000 / (fps.numerator / (float)fps.denominator));
    ComPtr<IMFSinkWriter> pSinkWriter;
    ComPtr<IMFSourceReader> pReader;
    LONGLONG rtStart = 0;
    StreamIndexes streamIndexes = { 0 };
    HRESULT hr = MFStartup(MF_VERSION);

    if (SUCCEEDED(hr))
        hr = InitializeSinkWriter(pathToMp4Output, &pSinkWriter, &pReader, &streamIndexes, fps, frameDuration);

    if (SUCCEEDED(hr))
    {
        ComPtr<IWICBitmap> pWICBitmap;
        std::wstring wxFile = pathToRenderingsFolder + L"\\wx.png";
        hr = LoadBitmapFromFile(pWICFactory, pD2DFactory, wxFile, &pWICBitmap);

        if (SUCCEEDED(hr))
            hr = RenderSingleVideoForecast(pWICFactory, pD2DFactory, pDWriteFactory, pSinkWriter.Get(), pWICBitmap.Get(), pReader.Get(), &streamIndexes, frameDuration, rtStart);
    }

    if (SUCCEEDED(hr))
    {
        for (auto forecastIndex = 0; forecastIndex < totalForecasts; forecastIndex++)
        {
            ComPtr<IWICBitmap> pWICBitmap;
            hr = RenderForecastBitmap(pathToRenderingsFolder, pWICFactory, pD2DFactory, pDWriteFactory, &pWICBitmap, forecastIndex);
            if (FAILED(hr))
                break;

            hr = RenderSingleVideoForecast(pWICFactory, pD2DFactory, pDWriteFactory, pSinkWriter.Get(), pWICBitmap.Get(), pReader.Get(), &streamIndexes, frameDuration, rtStart);
            if (FAILED(hr))
                break;
        }
    }

    if (SUCCEEDED(hr))
        hr = pSinkWriter->Finalize();

    if (SUCCEEDED(hr))
        hr = MFShutdown();

    return hr;
}