#include <EngineRuntime.h>

#include <Windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>

#include <DispatcherQueue.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <windows.ui.composition.interop.h>
#include <windows.ui.composition.desktop.h>
#include <windows.graphics.effects.interop.h>

#pragma comment(lib, "CoreMessaging.lib")
#pragma comment(lib, "RuntimeObject.lib")
#pragma comment(lib, "windowsapp.lib")

using namespace Engine;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;
using namespace winrt::Windows::Graphics::DirectX;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::Graphics;

ABI::Windows::System::IDispatcherQueueController * controller;

enum CreateEngineEffectLayersFlags : uint {
	CreateEngineEffectTransparentBackground	= 0x0001,
	CreateEngineEffectBlurBehind			= 0x0002,
};
struct CreateEngineEffectLayersDesc
{
	HWND window;
	uint layer_flags;
	ID3D11Device * device;
	uint width;
	uint height;
	double deviation;
};

HSTRING EngineAllocateString(const widechar * string)
{
	int length = StringLength(string) + 1;
	HSTRING result;
	if (WindowsCreateString(string, length, &result) != S_OK) return 0;
	return result;
}

class EnginePropertyValue : public ABI::Windows::Foundation::IPropertyValue
{
	uint _ref_cnt;
	ABI::Windows::Foundation::PropertyType _type;
	uint32 _prop_uint32;
	float _prop_float;
public:
	EnginePropertyValue(uint32 value) : _ref_cnt(1), _type(ABI::Windows::Foundation::PropertyType::PropertyType_UInt32), _prop_uint32(value), _prop_float(0.0f) {}
	EnginePropertyValue(float value) : _ref_cnt(1), _type(ABI::Windows::Foundation::PropertyType::PropertyType_Single), _prop_uint32(0), _prop_float(value) {}
	~EnginePropertyValue(void) {}
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override
	{
		if (!ppvObject) return E_INVALIDARG;
		if (riid == __uuidof(IUnknown)) *ppvObject = static_cast<IUnknown *>(this);
		else if (riid == __uuidof(IInspectable)) *ppvObject = static_cast<IInspectable *>(this);
		else if (riid == __uuidof(IPropertyValue)) *ppvObject = static_cast<IPropertyValue *>(this);
		else return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
	virtual ULONG STDMETHODCALLTYPE Release(void) override { auto result = InterlockedDecrement(&_ref_cnt); if (!result) delete this; return result; }
	// IInspectable
	virtual HRESULT STDMETHODCALLTYPE GetIids(ULONG * iidCount, IID ** iids) override
	{
		if (!iidCount || !iids) return E_POINTER;
		auto result = reinterpret_cast<IID *>(CoTaskMemAlloc(sizeof(IID) * 3));
		if (!result) return E_OUTOFMEMORY;
		result[0] = __uuidof(IUnknown);
		result[1] = __uuidof(IInspectable);
		result[2] = __uuidof(IPropertyValue);
		*iidCount = 3;
		*iids = result;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING * className) override
	{
		if (!className) return E_POINTER;
		auto str = EngineAllocateString(L"Engine.Effect.Internal.PropertyValue");
		if (!str) return E_OUTOFMEMORY;
		*className = str;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel * trustLevel) override
	{
		if (!trustLevel) return E_POINTER;
		*trustLevel = TrustLevel::BaseTrust;
		return S_OK;
	}
	// IPropertyValue
	virtual HRESULT STDMETHODCALLTYPE get_Type(ABI::Windows::Foundation::PropertyType * value) override { if (!value) return E_POINTER; *value = _type; return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE get_IsNumericScalar(::boolean * value) override { if (!value) return E_POINTER; *value = true; return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt8(BYTE * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInt16(INT16 * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt16(UINT16 * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInt32(INT32 * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt32(UINT32 * value) override
	{
		if (_type != ABI::Windows::Foundation::PropertyType::PropertyType_UInt32) return E_INVALIDARG;
		if (!value) return E_POINTER;
		*value = _prop_uint32;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetInt64(INT64 * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt64(UINT64 * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetSingle(FLOAT * value) override
	{
		if (_type != ABI::Windows::Foundation::PropertyType::PropertyType_Single) return E_INVALIDARG;
		if (!value) return E_POINTER;
		*value = _prop_float;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetDouble(DOUBLE * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetChar16(WCHAR * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetBoolean(::boolean * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetString(HSTRING * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetGuid(GUID * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetDateTime(ABI::Windows::Foundation::DateTime * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetTimeSpan(ABI::Windows::Foundation::TimeSpan * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetPoint(ABI::Windows::Foundation::Point * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetSize(ABI::Windows::Foundation::Size * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetRect(ABI::Windows::Foundation::Rect * value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt8Array(UINT32 * __valueSize, BYTE ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInt16Array(UINT32 * __valueSize, INT16 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt16Array(UINT32 * __valueSize, UINT16 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInt32Array(UINT32 * __valueSize, INT32 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt32Array(UINT32 * __valueSize, UINT32 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInt64Array(UINT32 * __valueSize, INT64 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetUInt64Array(UINT32 * __valueSize, UINT64 ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetSingleArray(UINT32 * __valueSize, FLOAT ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetDoubleArray(UINT32 * __valueSize, DOUBLE ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetChar16Array(UINT32 * __valueSize, WCHAR ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetBooleanArray(UINT32 * __valueSize, ::boolean ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetStringArray(UINT32 * __valueSize, HSTRING ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetInspectableArray(UINT32 * __valueSize, IInspectable *** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetGuidArray(UINT32 * __valueSize, GUID ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetDateTimeArray(UINT32 * __valueSize, ABI::Windows::Foundation::DateTime ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetTimeSpanArray(UINT32 * __valueSize, ABI::Windows::Foundation::TimeSpan ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetPointArray(UINT32 * __valueSize, ABI::Windows::Foundation::Point ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetSizeArray(UINT32 * __valueSize, ABI::Windows::Foundation::Size ** value) override { return E_INVALIDARG; }
	virtual HRESULT STDMETHODCALLTYPE GetRectArray(UINT32 * __valueSize, ABI::Windows::Foundation::Rect ** value) override { return E_INVALIDARG; }
};
class EngineBlurEffect :
	public ABI::Windows::Graphics::Effects::IGraphicsEffect,
	public ABI::Windows::Graphics::Effects::IGraphicsEffectSource,
	public ABI::Windows::Graphics::Effects::IGraphicsEffectD2D1Interop
{
	uint _ref_cnt;
	IGraphicsEffectSource * _source;
	string _name;
	float _deviation;
	D2D1_GAUSSIANBLUR_OPTIMIZATION _optimization;
	D2D1_BORDER_MODE _border;
public:
	EngineBlurEffect(void) : _ref_cnt(1), _source(0),
		_deviation(3.0f), _optimization(D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED), _border(D2D1_BORDER_MODE_SOFT) {}
	~EngineBlurEffect(void) { if (_source) _source->Release(); }
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override
	{
		if (!ppvObject) return E_INVALIDARG;
		if (riid == __uuidof(IUnknown)) *ppvObject = static_cast<IUnknown *>(static_cast<IGraphicsEffect *>(this));
		else if (riid == __uuidof(IInspectable)) *ppvObject = static_cast<IInspectable *>(static_cast<IGraphicsEffect *>(this));
		else if (riid == __uuidof(IGraphicsEffect)) *ppvObject = static_cast<IGraphicsEffect *>(this);
		else if (riid == __uuidof(IGraphicsEffectSource)) *ppvObject = static_cast<IGraphicsEffectSource *>(this);
		else if (riid == __uuidof(IGraphicsEffectD2D1Interop)) *ppvObject = static_cast<IGraphicsEffectD2D1Interop *>(this);
		else return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
	virtual ULONG STDMETHODCALLTYPE Release(void) override { auto result = InterlockedDecrement(&_ref_cnt); if (!result) delete this; return result; }
	// IInspectable
	virtual HRESULT STDMETHODCALLTYPE GetIids(ULONG * iidCount, IID ** iids) override
	{
		if (!iidCount || !iids) return E_POINTER;
		auto result = reinterpret_cast<IID *>(CoTaskMemAlloc(sizeof(IID) * 5));
		if (!result) return E_OUTOFMEMORY;
		result[0] = __uuidof(IUnknown);
		result[1] = __uuidof(IInspectable);
		result[2] = __uuidof(IGraphicsEffect);
		result[3] = __uuidof(IGraphicsEffectSource);
		result[4] = __uuidof(IGraphicsEffectD2D1Interop);
		*iidCount = 5;
		*iids = result;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING * className) override
	{
		if (!className) return E_POINTER;
		auto str = EngineAllocateString(L"Engine.Effect.Internal.BlurEffect");
		if (!str) return E_OUTOFMEMORY;
		*className = str;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel * trustLevel) override
	{
		if (!trustLevel) return E_POINTER;
		*trustLevel = TrustLevel::BaseTrust;
		return S_OK;
	}
	// IGraphicsEffect
	virtual HRESULT STDMETHODCALLTYPE get_Name(HSTRING * name) override
	{
		if (!name) return E_POINTER;
		if (!_name.Length()) {
			*name = 0;
			return S_OK;
		}
		auto result = EngineAllocateString(_name);
		if (!result) return E_OUTOFMEMORY;
		*name = result;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE put_Name(HSTRING name) override
	{
		try {
			UINT32 len;
			auto pstr = WindowsGetStringRawBuffer(name, &len);
			_name = string(pstr, len, SystemEncoding);
			return S_OK;
		} catch (...) { return E_OUTOFMEMORY; }
	}
	// IGraphicsEffectD2D1Interop
	virtual HRESULT STDMETHODCALLTYPE GetEffectId(GUID * id) override { if (!id) return E_INVALIDARG; *id = CLSID_D2D1GaussianBlur; return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE GetNamedPropertyMapping(LPCWSTR name, UINT * index, Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING * mapping) override
	{
		if (!name) return E_POINTER;
		if (StringCompare(name, L"StandardDeviation") == 0) {
			if (index) *index = 0;
		} else if (StringCompare(name, L"Optimization") == 0) {
			if (index) *index = 1;
		} else if (StringCompare(name, L"BorderMode") == 0) {
			if (index) *index = 2;
		} else E_INVALIDARG;
		if (mapping) *mapping = Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetPropertyCount(UINT * count) override { if (!count) return E_INVALIDARG; *count = 3; return S_OK; }
	virtual HRESULT STDMETHODCALLTYPE GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue ** value) override
	{
		if (index > 3 || !value) return E_INVALIDARG;
		EnginePropertyValue * prop = 0;
		if (index == D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION) {
			prop = new (std::nothrow) EnginePropertyValue(_deviation);
		} else if (index == D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION) {
			prop = new (std::nothrow) EnginePropertyValue(uint32(_optimization));
		} else if (index == D2D1_GAUSSIANBLUR_PROP_BORDER_MODE) {
			prop = new (std::nothrow) EnginePropertyValue(uint32(_border));
		}
		if (!prop) return E_OUTOFMEMORY;
		*value = prop;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetSource(UINT index, IGraphicsEffectSource ** source) override
	{
		if (index > 0 || !source) return E_INVALIDARG;
		*source = _source;
		if (_source) _source->AddRef();
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE GetSourceCount(UINT * count) override { if (!count) return E_INVALIDARG; *count = 1; return S_OK; }
	// Configure API
	void SetSource(void * ptr)
	{
		auto punk = reinterpret_cast<IUnknown *>(ptr);
		if (_source) _source->Release();
		if (punk->QueryInterface(IID_PPV_ARGS(&_source)) != S_OK) _source = 0;
	}
	void SetDeviation(float value) { _deviation = value; }
	void SetOptimization(D2D1_GAUSSIANBLUR_OPTIMIZATION value) { _optimization = value; }
	void SetBorderMode(D2D1_BORDER_MODE value) { _border = value; }
};
class EngineEffectLayers : public Object
{
	Compositor compositor;
	DesktopWindowTarget target;
	CompositionGraphicsDevice device;
	CompositionDrawingSurface drawing;
	winrt::impl::com_ref<Composition::ICompositionDrawingSurfaceInterop> drawing_interop;
public:
	EngineEffectLayers(const CreateEngineEffectLayersDesc & desc) : compositor(nullptr), target(nullptr), device(nullptr), drawing(nullptr), drawing_interop(nullptr)
	{
		compositor = Compositor();
		auto winapi_interop = compositor.as<Composition::Desktop::ICompositorDesktopInterop>();
		auto device_interop = compositor.as<Composition::ICompositorInterop>();
		if (winapi_interop->CreateDesktopWindowTarget(desc.window, FALSE, reinterpret_cast<Composition::Desktop::IDesktopWindowTarget **>(winrt::put_abi(target))) != S_OK) throw Exception();
		if (device_interop->CreateGraphicsDevice(desc.device, reinterpret_cast<Composition::ICompositionGraphicsDevice **>(winrt::put_abi(device))) != S_OK) throw Exception();
		drawing = device.CreateDrawingSurface(winrt::Windows::Foundation::Size(float(desc.width), float(desc.height)), DirectXPixelFormat::B8G8R8A8UIntNormalized, DirectXAlphaMode::Premultiplied);
		drawing_interop = drawing.as<Composition::ICompositionDrawingSurfaceInterop>();
		auto root = compositor.CreateContainerVisual();
		auto main_brush = compositor.CreateSurfaceBrush(drawing);
		main_brush.Stretch(CompositionStretch::Fill);
		auto main = compositor.CreateSpriteVisual();
		if (!(desc.layer_flags & CreateEngineEffectTransparentBackground)) {
			winrt::Windows::UI::Color color;
			color.R = color.G = color.B = 0; color.A = 0xFF;
			auto brush = compositor.CreateColorBrush(color);
			auto solid = compositor.CreateSpriteVisual();
			solid.Offset({ 0.0, 0.0, 0.0 });
			solid.RelativeSizeAdjustment({ 1.0, 1.0 });
			solid.Brush(brush);
			root.Children().InsertAtBottom(solid);
		} else if (desc.layer_flags & CreateEngineEffectBlurBehind) {
			winrt::impl::com_ref<EngineBlurEffect> effect_info;
			effect_info.attach(new EngineBlurEffect);
			auto effect_info_object_unknown = effect_info.as<IUnknown>();
			winrt::Windows::Foundation::IUnknown effect_info_object(nullptr);
			winrt::attach_abi(effect_info_object, effect_info_object_unknown.get());
			auto source = CompositionEffectSourceParameter(L"back");
			effect_info->SetSource(winrt::get_abi(source));
			effect_info->SetDeviation(desc.deviation);
			effect_info->SetOptimization(D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
			effect_info->SetBorderMode(D2D1_BORDER_MODE_HARD);
			auto factory = compositor.CreateEffectFactory(effect_info_object.as<winrt::Windows::Graphics::Effects::IGraphicsEffect>());
			auto backdrop = compositor.CreateBackdropBrush();
			auto effect_brush = factory.CreateBrush();
			effect_brush.SetSourceParameter(L"back", backdrop);
			auto effect = compositor.CreateSpriteVisual();
			effect.Offset({ 0.0, 0.0, 0.0 });
			effect.RelativeSizeAdjustment({ 1.0, 1.0 });
			effect.Brush(effect_brush);
			root.Children().InsertAtBottom(effect);
		}
		main.Offset({ 0.0, 0.0, 0.0 });
		main.RelativeSizeAdjustment({ 1.0, 1.0 });
		main.Brush(main_brush);
		root.Offset({ 0.0, 0.0, 0.0 });
		root.RelativeSizeAdjustment({ 1.0, 1.0 });
		root.Children().InsertAtTop(main);
		target.Root(root);
	}
	virtual ~EngineEffectLayers(void) override {}
	bool ResizeLayers(uint width, uint height) noexcept
	{
		try {
			drawing.Resize({ int(width), int(height) });
			return true;
		} catch (...) { return false; }
	}
	bool BeginDraw(ID3D11Texture2D ** surface, uint & orgx, uint & orgy) noexcept
	{
		POINT offset;
		if (drawing_interop->BeginDraw(nullptr, IID_PPV_ARGS(surface), &offset) != S_OK) return false;
		orgx = offset.x;
		orgy = offset.y;
		return true;
	}
	bool EndDraw(void) noexcept { return drawing_interop->EndDraw() == S_OK; }
	virtual ImmutableString ToString(void) const override { return L"Engine Effect Layers"; }
};

typedef EngineEffectLayers * HLAYERS;

ENGINE_EXPORT_API BOOL WINAPI EngineEffectInit(void)
{
	if (!controller) {
		DispatcherQueueOptions options;
		options.dwSize = sizeof(options);
		options.threadType = DQTYPE_THREAD_CURRENT;
		options.apartmentType = DQTAT_COM_ASTA;
		if (CreateDispatcherQueueController(options, &controller) != S_OK) return FALSE;
	}
	return TRUE;
}
ENGINE_EXPORT_API HLAYERS WINAPI EngineEffectCreateLayers(const CreateEngineEffectLayersDesc * desc)
{
	try {
		auto result = new EngineEffectLayers(*desc);
		return result;
	} catch (...) { return 0; }
}
ENGINE_EXPORT_API BOOL WINAPI EngineEffectResizeLayers(HLAYERS layers, uint width, uint height)
{
	if (layers) return layers->ResizeLayers(width, height);
	else return FALSE;
}
ENGINE_EXPORT_API VOID WINAPI EngineEffectRetainLayers(HLAYERS layers) { if (layers) layers->Retain(); }
ENGINE_EXPORT_API VOID WINAPI EngineEffectReleaseLayers(HLAYERS layers) { if (layers) layers->Release(); }
ENGINE_EXPORT_API BOOL WINAPI EngineEffectBeginDraw(HLAYERS layers, ID3D11Texture2D ** surface, uint * orgx, uint * orgy)
{
	if (layers) return layers->BeginDraw(surface, *orgx, *orgy);
	else return FALSE;
}
ENGINE_EXPORT_API BOOL WINAPI EngineEffectEndDraw(HLAYERS layers)
{
	if (layers) return layers->EndDraw();
	else return FALSE;
}

void LibraryLoaded(void) { controller = 0; }
void LibraryUnloaded(void)
{
	if (controller) {
		ABI::Windows::Foundation::IAsyncAction * action;
		if (controller->ShutdownQueueAsync(&action) == S_OK) {
			action->GetResults();
			action->Release();
		}
		controller->Release();
	}
}