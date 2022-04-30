#include <Geode.hpp>

USE_GEODE_NAMESPACE();

HWND glfwGetWin32Window(GLFWwindow* window) {
    static auto cocosBase = GetModuleHandleA("libcocos2d.dll");

    auto pRet = reinterpret_cast<HWND(__cdecl*)(GLFWwindow*)>(
        reinterpret_cast<uintptr_t>(cocosBase) + 0x112c10
    )(window);

    return pRet;
}

HWND getGDHWND() {
    static HWND g_hwnd = nullptr;

    if (!g_hwnd) {
        auto dir = CCDirector::sharedDirector();
        if (!dir) return nullptr;
        auto opengl = dir->getOpenGLView();
        if (!opengl) return nullptr;
        auto wnd = dir->getOpenGLView()->getWindow();
        if (!wnd) return nullptr;
        g_hwnd = glfwGetWin32Window(wnd);
    }

    return g_hwnd;
}

static std::unordered_map<uintptr_t, Patch*> g_patches;
static int g_oldFlags;

void goBorderless() {
	for (auto& [addr, patch] : std::unordered_map<uintptr_t, byte_array> {
		{ 0x11388b, { 0x90, 0x90, 0x90, 0x90, 0x90 } },
		{ 0x11339d, { 0xb9, 0xff, 0xff, 0xff, 0x7f, 0x90, 0x90 } },
		{ 0x1133c0, { 0x48 } },
		{ 0x1133c6, { 0x48 } },
		{ 0x112536, { 0xeb, 0x11, 0x90 } },
	}) {
		if (g_patches.count(addr)) {
			g_patches.at(addr)->apply();
			continue;
		}
		static auto base = GetModuleHandleA("libcocos2d.dll");
		auto res = Mod::get()->patch(as<void*>(as<uintptr_t>(base) + addr), patch);
		if (res) {
			g_patches[addr] = res.value();
		} else {
			Log::get() << res.error();
		}
	}

	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);
	g_oldFlags = SetWindowLongPtr(getGDHWND(), GWL_STYLE, WS_OVERLAPPED | WS_VISIBLE);
	SetWindowPos(getGDHWND(), HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
}

void leaveBorderless() {
	for (auto& [_, patch] : g_patches) {
		patch->restore();
	}

	SetWindowLongPtr(getGDHWND(), GWL_STYLE, g_oldFlags);

	auto size = new CCSize();
	GameManager::sharedState()->resolutionForKey(
		size, GameManager::sharedState()->m_resolution
	);
	CCEGLView::sharedOpenGLView()->resizeWindow(size->width, size->height);
	CCEGLView::sharedOpenGLView()->centerWindow();
	delete size;
}

class $modify(LoadingLayer) {
	bool init(bool fromReload) {
        if (!LoadingLayer::init(fromReload))
            return false;
		
		auto r = Mod::get()->getSettingValue<bool>("borderless-fullscreen");
		if (r && r.value()) {
			goBorderless();
		}

		return true;
	}
};

GEODE_API bool GEODE_CALL geode_load(Mod* mod) {
	auto r = mod->getSettingValue<bool>("borderless-fullscreen");
	if (r && r.value()) {
		goBorderless();
	}
	return true;
}

GEODE_API void GEODE_CALL geode_unload() {
	auto r = Mod::get()->getSettingValue<bool>("borderless-fullscreen");
	if (r && r.value()) {
		leaveBorderless();
	}
	g_patches.clear();
}

GEODE_API void GEODE_CALL geode_setting_updated(const char* key, Setting* setting) {
	if (std::string(key) == "borderless-fullscreen") {
		auto v = as<BoolSetting*>(setting)->getValue();
		if (v) {
			goBorderless();
		} else {
			leaveBorderless();
		}
	}
}
