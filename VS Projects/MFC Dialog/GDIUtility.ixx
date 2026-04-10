/****************************************************************
* Copyright © 2025-present Jovibor https://github.com/jovibor/  *
* Windows GDI helpful utilities.                                *
* This software is available under "The MIT License"            *
****************************************************************/
module;
#include <SDKDDKVer.h>
#include <Windows.h>
#include <d2d1_3.h>
#include <Shlwapi.h> //SHCreateMemStream.
#include <commctrl.h>
#include <algorithm>
#include <cassert>
#include <format>
#include <locale>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
export module GDIUtility;

#pragma comment(lib, "MSImg32") //AlphaBlend.
#pragma comment(lib, "Shlwapi") //SHCreateMemStream.
#pragma comment(lib, "d2d1")    //D2D1CreateFactory.

export namespace GDIUT { //Windows GDI related stuff.
	auto DefWndProc(const MSG& msg) -> LRESULT {
		return ::DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

	template<typename T>
	auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->LRESULT
	{
		//Different IHexCtrl objects will share the same WndProc<ExactTypeHere> function.
		//Hence, the map is needed to differentiate these objects. 
		//The DlgProc<T> works absolutely the same way.
		static std::unordered_map<HWND, T*> uMap;

		//CREATESTRUCTW::lpCreateParams always possesses `this` pointer, passed to the CreateWindowExW
		//function as lpParam. We save it to the static uMap to have access to this->ProcessMsg() method.
		if (uMsg == WM_CREATE) {
			const auto lpCS = reinterpret_cast<LPCREATESTRUCTW>(lParam);
			if (lpCS->lpCreateParams != nullptr) {
				uMap[hWnd] = reinterpret_cast<T*>(lpCS->lpCreateParams);
			}
		}

		if (const auto it = uMap.find(hWnd); it != uMap.end()) {
			const auto ret = it->second->ProcessMsg({ .hwnd { hWnd }, .message { uMsg },
				.wParam { wParam }, .lParam { lParam } });
			if (uMsg == WM_NCDESTROY) { //Remove hWnd from the map on window destruction.
				uMap.erase(it);
			}
			return ret;
		}

		return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	template<typename T>
	auto CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->INT_PTR {
		//DlgProc should return zero for all non-processed messages.
		//In that case messages will be processed by Windows default dialog proc.
		//Non-processed messages should not be passed neither to DefWindowProcW nor to DefDlgProcW.
		//Processed messages should return any non-zero value, depending on message type.

		static std::unordered_map<HWND, T*> uMap;

		//DialogBoxParamW and CreateDialogParamW dwInitParam arg is sent with WM_INITDIALOG as lParam.
		if (uMsg == WM_INITDIALOG) {
			if (lParam != 0) {
				uMap[hWnd] = reinterpret_cast<T*>(lParam);
			}
		}

		if (const auto it = uMap.find(hWnd); it != uMap.end()) {
			const auto ret = it->second->ProcessMsg({ .hwnd { hWnd }, .message { uMsg },
				.wParam { wParam }, .lParam { lParam } });
			if (uMsg == WM_NCDESTROY) { //Remove hWnd from the map on dialog destruction.
				uMap.erase(it);
			}
			return ret;
		}

		return 0;
	}

	template<typename TCom> requires requires(TCom* pTCom) { pTCom->AddRef(); pTCom->Release(); }
	class comptr {
	public:
		comptr() = default;
		comptr(TCom* pTCom) : m_pTCom(pTCom) { }
		comptr(const comptr<TCom>& rhs) : m_pTCom(rhs.get()) { safe_addref(); }
		~comptr() { safe_release(); }
		operator TCom*()const { return get(); }
		operator TCom**() { return get_addr(); }
		operator IUnknown**() { return reinterpret_cast<IUnknown**>(get_addr()); }
		operator void**() { return reinterpret_cast<void**>(get_addr()); }
		auto operator->()const->TCom* { return get(); }
		auto operator=(const comptr<TCom>& rhs)->comptr& {
			if (this != &rhs) {
				safe_release();	m_pTCom = rhs.get(); safe_addref();
			}
			return *this;
		}
		auto operator=(TCom* pRHS)->comptr& {
			if (get() != pRHS) {
				if (get() != nullptr) { get()->Release(); }
				m_pTCom = pRHS;
			}
			return *this;
		}
		[[nodiscard]] bool operator==(const comptr<TCom>& rhs)const { return get() == rhs.get(); }
		[[nodiscard]] bool operator==(const TCom* pRHS)const { return get() == pRHS; }
		[[nodiscard]] explicit operator bool() { return get() != nullptr; }
		[[nodiscard]] explicit operator bool()const { return get() != nullptr; }
		[[nodiscard]] auto get()const -> TCom* { return m_pTCom; }
		[[nodiscard]] auto get_addr() -> TCom** { return &m_pTCom; }
		void safe_release() { if (get() != nullptr) { get()->Release(); m_pTCom = nullptr; } }
		void safe_addref() { if (get() != nullptr) { get()->AddRef(); } }
	private:
		TCom* m_pTCom { };
	};

	class CDynLayout final {
	public:
		//Ratio settings, for how much to move or to resize child item when parent is resized.
		struct ItemRatio {
			[[nodiscard]] bool IsNull()const { return flXRatio == 0.F && flYRatio == 0.F; };
			float flXRatio { }; float flYRatio { };
		};
		struct MoveRatio : public ItemRatio { }; //To differentiate move from size in the AddItem.
		struct SizeRatio : public ItemRatio { };

		CDynLayout() = default;
		CDynLayout(HWND hWndHost) : m_hWndHost(hWndHost) { }
		void AddItem(int iIDItem, MoveRatio move, SizeRatio size);
		void AddItem(HWND hWndItem, MoveRatio move, SizeRatio size);
		void Enable(bool fTrack);
		bool LoadFromResource(HINSTANCE hInstRes, const wchar_t* pwszNameResource);
		bool LoadFromResource(HINSTANCE hInstRes, UINT uNameResource);
		void OnSize(int iWidth, int iHeight)const; //Should be hooked into the host window's WM_SIZE handler.
		void RemoveAll() { m_vecItems.clear(); }
		void SetHost(HWND hWnd) { assert(hWnd != nullptr); m_hWndHost = hWnd; }

		//Static helper methods to use in the AddItem.
		[[nodiscard]] static MoveRatio MoveNone() { return { }; }
		[[nodiscard]] static MoveRatio MoveHorz(int iXRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) } } };
		}
		[[nodiscard]] static MoveRatio MoveVert(int iYRatio) {
			return { { .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static MoveRatio MoveHorzAndVert(int iXRatio, int iYRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) }, .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeNone() { return { }; }
		[[nodiscard]] static SizeRatio SizeHorz(int iXRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeVert(int iYRatio) {
			return { { .flYRatio { ToFlRatio(iYRatio) } } };
		}
		[[nodiscard]] static SizeRatio SizeHorzAndVert(int iXRatio, int iYRatio) {
			return { { .flXRatio { ToFlRatio(iXRatio) }, .flYRatio { ToFlRatio(iYRatio) } } };
		}
	private:
		[[nodiscard]] static auto ToFlRatio(int iRatio) -> float {
			return std::clamp(iRatio, 0, 100) / 100.F;
		}
		struct ItemData {
			HWND hWnd { };   //Item window.
			RECT rcOrig { }; //Item original window rect after EnableTrack(true).
			MoveRatio move;  //How much to move the item.
			SizeRatio size;  //How much to resize the item.
		};
		HWND m_hWndHost { };   //Host window.
		RECT m_rcHostOrig { }; //Host original client area rect after EnableTrack(true).
		std::vector<ItemData> m_vecItems; //All items to resize/move.
		bool m_fTrack { };
	};

	void CDynLayout::AddItem(int iIDItem, MoveRatio move, SizeRatio size) {
		AddItem(::GetDlgItem(m_hWndHost, iIDItem), move, size);
	}

	void CDynLayout::AddItem(HWND hWndItem, MoveRatio move, SizeRatio size) {
		assert(hWndItem != nullptr);
		if (hWndItem == nullptr)
			return;

		if (move.IsNull() && size.IsNull())
			return;

		m_vecItems.emplace_back(ItemData { .hWnd { hWndItem }, .move { move }, .size { size } });
	}

	void CDynLayout::Enable(bool fTrack) {
		m_fTrack = fTrack;
		if (m_fTrack) {
			::GetClientRect(m_hWndHost, &m_rcHostOrig);
			for (auto& [hWnd, rc, move, size] : m_vecItems) {
				::GetWindowRect(hWnd, &rc);
				::ScreenToClient(m_hWndHost, reinterpret_cast<LPPOINT>(&rc));
				::ScreenToClient(m_hWndHost, reinterpret_cast<LPPOINT>(&rc) + 1);
			}
		}
	}

	bool CDynLayout::LoadFromResource(HINSTANCE hInstRes, const wchar_t* pwszNameResource) {
		assert(pwszNameResource != nullptr);
		if (pwszNameResource == nullptr)
			return false;

		assert(m_hWndHost != nullptr);
		if (m_hWndHost == nullptr)
			return false;

		const auto hDlgLayout = ::FindResourceW(hInstRes, pwszNameResource, L"AFX_DIALOG_LAYOUT");
		if (hDlgLayout == nullptr) { //No such resource found in the hInstRes.
			return false;
		}

		const auto hResData = ::LoadResource(hInstRes, hDlgLayout);
		assert(hResData != nullptr);
		if (hResData == nullptr)
			return false;

		const auto pResData = ::LockResource(hResData);
		assert(pResData != nullptr);
		if (pResData == nullptr)
			return false;

		const auto dwSizeRes = ::SizeofResource(hInstRes, hDlgLayout);
		const auto* pDataBegin = reinterpret_cast<WORD*>(pResData);
		const auto* const pDataEnd = reinterpret_cast<WORD*>(reinterpret_cast<std::byte*>(pResData) + dwSizeRes);

		assert(*pDataBegin == 0);
		if (*pDataBegin != 0) //First WORD must be zero, it's a header (version number).
			return false;

		++pDataBegin; //Past first WORD is the actual data.
		auto hWndChild = ::GetWindow(m_hWndHost, GW_CHILD); //First child window in the host window.
		while (pDataBegin + 4 <= pDataEnd) { //Actual AFX_DIALOG_LAYOUT data.
			if (hWndChild == nullptr)
				break;

			const auto wXMoveRatio = *pDataBegin++;
			const auto wYMoveRatio = *pDataBegin++;
			const auto wXSizeRatio = *pDataBegin++;
			const auto wYSizeRatio = *pDataBegin++;
			AddItem(hWndChild, MoveHorzAndVert(wXMoveRatio, wYMoveRatio), SizeHorzAndVert(wXSizeRatio, wYSizeRatio));
			hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT);
		}

		return true;
	}

	bool CDynLayout::LoadFromResource(HINSTANCE hInstRes, UINT uNameResource) {
		return LoadFromResource(hInstRes, MAKEINTRESOURCEW(uNameResource));
	}

	void CDynLayout::OnSize(int iWidth, int iHeight)const {
		if (!m_fTrack)
			return;

		const auto iHostWidth = m_rcHostOrig.right - m_rcHostOrig.left;
		const auto iHostHeight = m_rcHostOrig.bottom - m_rcHostOrig.top;
		const auto iDeltaX = iWidth - iHostWidth;
		const auto iDeltaY = iHeight - iHostHeight;

		const auto hDWP = ::BeginDeferWindowPos(static_cast<int>(m_vecItems.size()));
		for (const auto& [hWnd, rc, move, size] : m_vecItems) {
			const auto iNewLeft = static_cast<int>(rc.left + (iDeltaX * move.flXRatio));
			const auto iNewTop = static_cast<int>(rc.top + (iDeltaY * move.flYRatio));
			const auto iOrigWidth = rc.right - rc.left;
			const auto iOrigHeight = rc.bottom - rc.top;
			const auto iNewWidth = static_cast<int>(iOrigWidth + (iDeltaX * size.flXRatio));
			const auto iNewHeight = static_cast<int>(iOrigHeight + (iDeltaY * size.flYRatio));
			::DeferWindowPos(hDWP, hWnd, nullptr, iNewLeft, iNewTop, iNewWidth, iNewHeight, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		::EndDeferWindowPos(hDWP);
	}

	class CPoint final : public POINT {
	public:
		CPoint() : POINT { } { }
		CPoint(POINT pt) : POINT { pt } { }
		CPoint(int x, int y) : POINT { .x { x }, .y { y } } { }
		~CPoint() = default;
		operator LPPOINT() { return this; }
		operator const POINT*()const { return this; }
		bool operator==(CPoint rhs)const { return x == rhs.x && y == rhs.y; }
		bool operator==(POINT pt)const { return x == pt.x && y == pt.y; }
		friend bool operator==(POINT pt, CPoint rhs) { return rhs == pt; }
		CPoint operator+(POINT pt)const { return { x + pt.x, y + pt.y }; }
		CPoint operator-(POINT pt)const { return { x - pt.x, y - pt.y }; }
		void Offset(int iX, int iY) { x += iX; y += iY; }
		void Offset(POINT pt) { Offset(pt.x, pt.y); }
	};

	class CRect final : public RECT {
	public:
		CRect() : RECT { } { }
		CRect(int iLeft, int iTop, int iRight, int iBottom) : RECT { .left { iLeft }, .top { iTop },
			.right { iRight }, .bottom { iBottom } } { }
		CRect(RECT rc) { ::CopyRect(this, &rc); }
		CRect(LPCRECT pRC) { ::CopyRect(this, pRC); }
		CRect(POINT pt, SIZE size) : RECT { .left { pt.x }, .top { pt.y }, .right { pt.x + size.cx },
			.bottom { pt.y + size.cy } } { }
		CRect(POINT topLeft, POINT botRight) : RECT { .left { topLeft.x }, .top { topLeft.y },
			.right { botRight.x }, .bottom { botRight.y } } { }
		~CRect() = default;
		operator LPRECT() { return this; }
		operator LPCRECT()const { return this; }
		bool operator==(CRect rhs)const { return ::EqualRect(this, rhs); }
		bool operator==(RECT rc)const { return ::EqualRect(this, &rc); }
		friend bool operator==(RECT rc, CRect rhs) { return rhs == rc; }
		CRect& operator=(RECT rc) { ::CopyRect(this, &rc); return *this; }
		[[nodiscard]] auto BottomRight()const -> CPoint { return { { .x { right }, .y { bottom } } }; };
		void DeflateRect(int x, int y) { ::InflateRect(this, -x, -y); }
		void DeflateRect(SIZE size) { ::InflateRect(this, -size.cx, -size.cy); }
		void DeflateRect(LPCRECT pRC) { left += pRC->left; top += pRC->top; right -= pRC->right; bottom -= pRC->bottom; }
		void DeflateRect(int l, int t, int r, int b) { left += l; top += t; right -= r; bottom -= b; }
		[[nodiscard]] int Height()const { return bottom - top; }
		[[nodiscard]] bool IsRectEmpty()const { return ::IsRectEmpty(this); }
		[[nodiscard]] bool IsRectNull()const { return (left == 0 && right == 0 && top == 0 && bottom == 0); }
		void OffsetRect(int x, int y) { ::OffsetRect(this, x, y); }
		void OffsetRect(POINT pt) { ::OffsetRect(this, pt.x, pt.y); }
		[[nodiscard]] bool PtInRect(POINT pt)const { return ::PtInRect(this, pt); }
		void SetRect(int x1, int y1, int x2, int y2) { ::SetRect(this, x1, y1, x2, y2); }
		void SetRectEmpty() { ::SetRectEmpty(this); }
		[[nodiscard]] auto TopLeft()const -> CPoint { return { { .x { left }, .y { top } } }; };
		[[nodiscard]] int Width()const { return right - left; }
	};

	class CDC {
	public:
		CDC() = default;
		CDC(HDC hDC) : m_hDC(hDC) { }
		~CDC() = default;
		operator HDC()const { return m_hDC; }
		void AbortDoc()const { ::AbortDoc(m_hDC); }
		int AlphaBlend(int iX, int iY, int iWidth, int iHeight, HDC hDCSrc,
			int iXSrc, int iYSrc, int iWidthSrc, int iHeightSrc, BYTE bSrcAlpha = 255, BYTE bAlphaFormat = AC_SRC_ALPHA)const {
			const BLENDFUNCTION bf { .SourceConstantAlpha { bSrcAlpha }, .AlphaFormat { bAlphaFormat } };
			return ::AlphaBlend(m_hDC, iX, iY, iWidth, iHeight, hDCSrc, iXSrc, iYSrc, iWidthSrc, iHeightSrc, bf);
		}
		BOOL BitBlt(int iX, int iY, int iWidth, int iHeight, HDC hDCSource, int iXSource, int iYSource, DWORD dwROP)const {
			//When blitting from a monochrome bitmap to a color one, the black color in the monohrome bitmap 
			//becomes the destination DC’s text color, and the white color in the monohrome bitmap 
			//becomes the destination DC’s background color, when using SRCCOPY mode.
			return ::BitBlt(m_hDC, iX, iY, iWidth, iHeight, hDCSource, iXSource, iYSource, dwROP);
		}
		[[nodiscard]] auto CreateCompatibleBitmap(int iWidth, int iHeight)const -> HBITMAP {
			return ::CreateCompatibleBitmap(m_hDC, iWidth, iHeight);
		}
		[[nodiscard]] CDC CreateCompatibleDC()const { return ::CreateCompatibleDC(m_hDC); }
		void DeleteDC()const { ::DeleteDC(m_hDC); }
		bool DrawFrameControl(LPRECT pRC, UINT uType, UINT uState)const {
			return ::DrawFrameControl(m_hDC, pRC, uType, uState);
		}
		bool DrawFrameControl(int iX, int iY, int iWidth, int iHeight, UINT uType, UINT uState)const {
			RECT rc { .left { iX }, .top { iY }, .right { iX + iWidth }, .bottom { iY + iHeight } };
			return DrawFrameControl(&rc, uType, uState);
		}
		void DrawImage(HBITMAP hBmp, int iX, int iY, int iWidth, int iHeight)const {
			const auto dcMem = CreateCompatibleDC();
			dcMem.SelectObject(hBmp);
			BITMAP bm; ::GetObjectW(hBmp, sizeof(BITMAP), &bm);

			//Only 32bit bitmaps can have alpha channel.
			//If destination and source bitmaps do not have the same color format, 
			//AlphaBlend converts the source bitmap to match the destination bitmap.
			//AlphaBlend works with both, DI (DeviceIndependent) and DD (DeviceDependent), bitmaps.
			AlphaBlend(iX, iY, iWidth, iHeight, dcMem, 0, 0, iWidth, iHeight, 255, bm.bmBitsPixel == 32 ? AC_SRC_ALPHA : 0);
			dcMem.DeleteDC();
		}
		[[nodiscard]] HDC GetHDC()const { return m_hDC; }
		void GetTextMetricsW(LPTEXTMETRICW pTM)const { ::GetTextMetricsW(m_hDC, pTM); }
		auto SetBkColor(COLORREF clr)const -> COLORREF { return ::SetBkColor(m_hDC, clr); }
		void DrawEdge(LPRECT pRC, UINT uEdge, UINT uFlags)const { ::DrawEdge(m_hDC, pRC, uEdge, uFlags); }
		void DrawFocusRect(LPCRECT pRc)const { ::DrawFocusRect(m_hDC, pRc); }
		int DrawTextW(std::wstring_view wsv, LPRECT pRect, UINT uFormat)const {
			return DrawTextW(wsv.data(), static_cast<int>(wsv.size()), pRect, uFormat);
		}
		int DrawTextW(LPCWSTR pwszText, int iSize, LPRECT pRect, UINT uFormat)const {
			return ::DrawTextW(m_hDC, pwszText, iSize, pRect, uFormat);
		}
		int EndDoc()const { return ::EndDoc(m_hDC); }
		int EndPage()const { return ::EndPage(m_hDC); }
		void FillSolidRect(LPCRECT pRC, COLORREF clr)const {
			::SetBkColor(m_hDC, clr); ::ExtTextOutW(m_hDC, 0, 0, ETO_OPAQUE, pRC, nullptr, 0, nullptr);
		}
		[[nodiscard]] auto GetClipBox()const -> CRect { RECT rc; ::GetClipBox(m_hDC, &rc); return rc; }
		bool LineTo(POINT pt)const { return LineTo(pt.x, pt.y); }
		bool LineTo(int x, int y)const { return ::LineTo(m_hDC, x, y); }
		bool MoveTo(POINT pt)const { return MoveTo(pt.x, pt.y); }
		bool MoveTo(int x, int y)const { return ::MoveToEx(m_hDC, x, y, nullptr); }
		bool Polygon(const POINT* pPT, int iCount)const { return ::Polygon(m_hDC, pPT, iCount); }
		int SetDIBits(HBITMAP hBmp, UINT uStartLine, UINT uLines, const void* pBits, const BITMAPINFO* pBMI, UINT uClrUse)const {
			return ::SetDIBits(m_hDC, hBmp, uStartLine, uLines, pBits, pBMI, uClrUse);
		}
		int SetDIBitsToDevice(int iX, int iY, DWORD dwWidth, DWORD dwHeight, int iXSrc, int iYSrc, UINT uStartLine, UINT uLines,
			const void* pBits, const BITMAPINFO* pBMI, UINT uClrUse)const {
			return ::SetDIBitsToDevice(m_hDC, iX, iY, dwWidth, dwHeight, iXSrc, iYSrc, uStartLine, uLines, pBits, pBMI, uClrUse);
		}
		int SetMapMode(int iMode)const { return ::SetMapMode(m_hDC, iMode); }
		auto SetTextColor(COLORREF clr)const -> COLORREF { return ::SetTextColor(m_hDC, clr); }
		auto SetViewportOrg(int iX, int iY)const -> POINT { POINT pt; ::SetViewportOrgEx(m_hDC, iX, iY, &pt); return pt; }
		auto SelectObject(HGDIOBJ hObj)const -> HGDIOBJ { return ::SelectObject(m_hDC, hObj); }
		int StartDocW(const DOCINFOW* pDI)const { return ::StartDocW(m_hDC, pDI); }
		int StartPage()const { return ::StartPage(m_hDC); }
		void TextOutW(int iX, int iY, LPCWSTR pwszText, int iSize)const { ::TextOutW(m_hDC, iX, iY, pwszText, iSize); }
		void TextOutW(int iX, int iY, std::wstring_view wsv)const {
			TextOutW(iX, iY, wsv.data(), static_cast<int>(wsv.size()));
		}
	protected:
		HDC m_hDC;
	};

	class CPaintDC final : public CDC {
	public:
		CPaintDC(HWND hWnd) : m_hWnd(hWnd) { assert(::IsWindow(hWnd)); m_hDC = ::BeginPaint(m_hWnd, &m_PS); }
		~CPaintDC() { ::EndPaint(m_hWnd, &m_PS); }
	private:
		PAINTSTRUCT m_PS;
		HWND m_hWnd;
	};

	class CMemDC final : public CDC {
	public:
		CMemDC(HDC hDC, RECT rc) : m_hDCOrig(hDC), m_rc(rc) {
			m_hDC = ::CreateCompatibleDC(m_hDCOrig);
			assert(m_hDC != nullptr);
			const auto iWidth = m_rc.right - m_rc.left;
			const auto iHeight = m_rc.bottom - m_rc.top;
			m_hBmp = ::CreateCompatibleBitmap(m_hDCOrig, iWidth, iHeight);
			assert(m_hBmp != nullptr);
			::SelectObject(m_hDC, m_hBmp);
		}
		~CMemDC() {
			const auto iWidth = m_rc.right - m_rc.left;
			const auto iHeight = m_rc.bottom - m_rc.top;
			::BitBlt(m_hDCOrig, m_rc.left, m_rc.top, iWidth, iHeight, m_hDC, m_rc.left, m_rc.top, SRCCOPY);
			::DeleteObject(m_hBmp);
			::DeleteDC(m_hDC);
		}
	private:
		HDC m_hDCOrig;
		HBITMAP m_hBmp;
		RECT m_rc;
	};

	class CWnd {
	public:
		CWnd() = default;
		CWnd(HWND hWnd) { Attach(hWnd); }
		~CWnd() = default;
		CWnd& operator=(HWND hWnd) { Attach(hWnd); return *this; };
		CWnd& operator=(CWnd) = delete;
		operator HWND()const { return m_hWnd; }
		[[nodiscard]] bool operator==(const CWnd& rhs)const { return m_hWnd == rhs.m_hWnd; }
		[[nodiscard]] bool operator==(HWND hWnd)const { return m_hWnd == hWnd; }
		void Attach(HWND hWnd) { m_hWnd = hWnd; } //Can attach to nullptr as well.
		void CheckRadioButton(int iIDFirst, int iIDLast, int iIDCheck)const {
			assert(IsWindow()); ::CheckRadioButton(m_hWnd, iIDFirst, iIDLast, iIDCheck);
		}
		[[nodiscard]] auto ChildWindowFromPoint(POINT pt)const -> CWnd {
			assert(IsWindow()); return ::ChildWindowFromPoint(m_hWnd, pt);
		}
		void ClientToScreen(LPPOINT pPT)const { assert(IsWindow()); ::ClientToScreen(m_hWnd, pPT); }
		void ClientToScreen(LPRECT pRC)const {
			ClientToScreen(reinterpret_cast<LPPOINT>(pRC)); ClientToScreen((reinterpret_cast<LPPOINT>(pRC)) + 1);
		}
		void DestroyWindow() { assert(IsWindow()); ::DestroyWindow(m_hWnd); m_hWnd = nullptr; }
		void Detach() { m_hWnd = nullptr; }
		[[nodiscard]] auto GetClientRect()const -> CRect {
			assert(IsWindow()); RECT rc; ::GetClientRect(m_hWnd, &rc); return rc;
		}
		bool EnableWindow(bool fEnable)const { assert(IsWindow()); return ::EnableWindow(m_hWnd, fEnable); }
		void EndDialog(INT_PTR iResult)const { assert(IsWindow()); ::EndDialog(m_hWnd, iResult); }
		[[nodiscard]] int GetCheckedRadioButton(int iIDFirst, int iIDLast)const {
			assert(IsWindow()); for (int iID = iIDFirst; iID <= iIDLast; ++iID) {
				if (::IsDlgButtonChecked(m_hWnd, iID) != 0) { return iID; }
			} return 0;
		}
		[[nodiscard]] auto GetDC()const -> HDC { assert(IsWindow()); return ::GetDC(m_hWnd); }
		[[nodiscard]] int GetDlgCtrlID()const { assert(IsWindow()); return ::GetDlgCtrlID(m_hWnd); }
		[[nodiscard]] auto GetDlgItem(int iIDCtrl)const -> CWnd { assert(IsWindow()); return ::GetDlgItem(m_hWnd, iIDCtrl); }
		[[nodiscard]] auto GetHFont()const -> HFONT {
			assert(IsWindow()); return reinterpret_cast<HFONT>(SendMsg(WM_GETFONT, 0, 0));
		}
		[[nodiscard]] auto GetHWND()const -> HWND { return m_hWnd; }
		[[nodiscard]] auto GetLogFont()const -> std::optional<LOGFONTW> {
			if (const auto hFont = GetHFont(); hFont != nullptr) {
				LOGFONTW lf { }; ::GetObjectW(hFont, sizeof(lf), &lf); return lf;
			}
			return std::nullopt;
		}
		[[nodiscard]] auto GetParent()const -> CWnd { assert(IsWindow()); return ::GetParent(m_hWnd); }
		[[nodiscard]] auto GetScrollInfo(bool fVert, UINT uMask = SIF_ALL)const -> SCROLLINFO {
			assert(IsWindow()); SCROLLINFO si { .cbSize { sizeof(SCROLLINFO) }, .fMask { uMask } };
			::GetScrollInfo(m_hWnd, fVert, &si); return si;
		}
		[[nodiscard]] auto GetScrollPos(bool fVert)const -> int { return GetScrollInfo(fVert, SIF_POS).nPos; }
		[[nodiscard]] auto GetWindowDC()const -> HDC { assert(IsWindow()); return ::GetWindowDC(m_hWnd); }
		[[nodiscard]] auto GetWindowLongPTR(int iIndex)const {
			assert(IsWindow()); return ::GetWindowLongPtrW(m_hWnd, iIndex);
		}
		[[nodiscard]] auto GetWindowRect()const -> CRect {
			assert(IsWindow()); RECT rc; ::GetWindowRect(m_hWnd, &rc); return rc;
		}
		[[nodiscard]] auto GetWindowStyles()const { return static_cast<DWORD>(GetWindowLongPTR(GWL_STYLE)); }
		[[nodiscard]] auto GetWindowStylesEx()const { return static_cast<DWORD>(GetWindowLongPTR(GWL_EXSTYLE)); }
		[[nodiscard]] auto GetWndText()const -> std::wstring {
			assert(IsWindow()); wchar_t buff[256]; ::GetWindowTextW(m_hWnd, buff, std::size(buff)); return buff;
		}
		[[nodiscard]] auto GetWndTextSize()const -> DWORD { assert(IsWindow()); return ::GetWindowTextLengthW(m_hWnd); }
		void Invalidate(bool fErase)const { assert(IsWindow()); ::InvalidateRect(m_hWnd, nullptr, fErase); }
		[[nodiscard]] bool IsDlgMessage(MSG* pMsg)const { return ::IsDialogMessageW(m_hWnd, pMsg); }
		[[nodiscard]] bool IsNull()const { return m_hWnd == nullptr; }
		[[nodiscard]] bool IsWindow()const { return ::IsWindow(m_hWnd); }
		[[nodiscard]] bool IsWindowEnabled()const { assert(IsWindow()); return ::IsWindowEnabled(m_hWnd); }
		[[nodiscard]] bool IsWindowVisible()const { assert(IsWindow()); return ::IsWindowVisible(m_hWnd); }
		[[nodiscard]] bool IsWndTextEmpty()const { return GetWndTextSize() == 0; }
		void KillTimer(UINT_PTR uID)const {
			assert(IsWindow()); ::KillTimer(m_hWnd, uID);
		}
		int MapWindowPoints(HWND hWndTo, LPRECT pRC)const {
			assert(IsWindow()); return ::MapWindowPoints(m_hWnd, hWndTo, reinterpret_cast<LPPOINT>(pRC), 2);
		}
		bool RedrawWindow(LPCRECT pRC = nullptr, HRGN hrgn = nullptr, UINT uFlags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)const {
			assert(IsWindow()); return static_cast<bool>(::RedrawWindow(m_hWnd, pRC, hrgn, uFlags));
		}
		int ReleaseDC(HDC hDC)const { assert(IsWindow()); return ::ReleaseDC(m_hWnd, hDC); }
		void ScreenToClient(LPPOINT pPT)const { assert(IsWindow()); ::ScreenToClient(m_hWnd, pPT); }
		void ScreenToClient(POINT& pt)const { ScreenToClient(&pt); }
		void ScreenToClient(LPRECT pRC)const {
			ScreenToClient(reinterpret_cast<LPPOINT>(pRC)); ScreenToClient(reinterpret_cast<LPPOINT>(pRC) + 1);
		}
		auto SendMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0)const -> LRESULT {
			assert(IsWindow()); return ::SendMessageW(m_hWnd, uMsg, wParam, lParam);
		}
		void SetActiveWindow()const { assert(IsWindow()); ::SetActiveWindow(m_hWnd); }
		auto SetCapture()const -> CWnd { assert(IsWindow()); return ::SetCapture(m_hWnd); }
		auto SetClassLongPTR(int iIndex, LONG_PTR dwNewLong)const -> ULONG_PTR {
			assert(IsWindow()); return ::SetClassLongPtrW(m_hWnd, iIndex, dwNewLong);
		}
		void SetFocus()const { assert(IsWindow()); ::SetFocus(m_hWnd); }
		void SetForegroundWindow()const { assert(IsWindow()); ::SetForegroundWindow(m_hWnd); }
		void SetScrollInfo(bool fVert, const SCROLLINFO& si)const {
			assert(IsWindow()); ::SetScrollInfo(m_hWnd, fVert, &si, TRUE);
		}
		void SetScrollPos(bool fVert, int iPos)const {
			const SCROLLINFO si { .cbSize { sizeof(SCROLLINFO) }, .fMask { SIF_POS }, .nPos { iPos } };
			SetScrollInfo(fVert, si);
		}
		auto SetTimer(UINT_PTR uID, UINT uElapse, TIMERPROC pFN = nullptr)const -> UINT_PTR {
			assert(IsWindow()); return ::SetTimer(m_hWnd, uID, uElapse, pFN);
		}
		void SetWindowPos(HWND hWndAfter, int iX, int iY, int iWidth, int iHeight, UINT uFlags)const {
			assert(IsWindow()); ::SetWindowPos(m_hWnd, hWndAfter, iX, iY, iWidth, iHeight, uFlags);
		}
		auto SetWndClassLong(int iIndex, LONG_PTR dwNewLong)const -> ULONG_PTR {
			assert(IsWindow()); return ::SetClassLongPtrW(m_hWnd, iIndex, dwNewLong);
		}
		void SetWndText(LPCWSTR pwszStr)const { assert(IsWindow()); ::SetWindowTextW(m_hWnd, pwszStr); }
		void SetWndText(const std::wstring& wstr)const { SetWndText(wstr.data()); }
		void SetRedraw(bool fRedraw)const { assert(IsWindow()); SendMsg(WM_SETREDRAW, fRedraw, 0); }
		bool ShowWindow(int iCmdShow)const { assert(IsWindow()); return ::ShowWindow(m_hWnd, iCmdShow); }
		[[nodiscard]] static auto FromHandle(HWND hWnd) -> CWnd { return hWnd; }
		[[nodiscard]] static auto GetFocus() -> CWnd { return ::GetFocus(); }
	protected:
		HWND m_hWnd { }; //Windows window handle.
	};

	class CWndBtn final : public CWnd {
	public:
		[[nodiscard]] bool IsChecked()const { assert(IsWindow()); return SendMsg(BM_GETCHECK, 0, 0); }
		void SetBitmap(HBITMAP hBmp)const {
			assert(IsWindow()); SendMsg(BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBmp));
		}
		void SetCheck(bool fCheck)const { assert(IsWindow()); SendMsg(BM_SETCHECK, fCheck, 0); }
	};

	class CWndEdit final : public CWnd {
	public:
		void SetCueBanner(LPCWSTR pwszText, bool fDrawIfFocus = false)const {
			assert(IsWindow()); SendMsg(EM_SETCUEBANNER, fDrawIfFocus, reinterpret_cast<LPARAM>(pwszText));
		}
		void SetLimitText(int iSize) { assert(IsWindow()); SendMsg(EM_LIMITTEXT, iSize); }
	};

	class CWndCombo final : public CWnd {
	public:
		int AddString(const std::wstring& wstr)const { return AddString(wstr.data()); }
		int AddString(LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pwszStr)));
		}
		int DeleteString(int iIndex)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_DELETESTRING, iIndex, 0));
		}
		[[nodiscard]] int FindStringExact(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_FINDSTRINGEXACT, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		[[nodiscard]] auto GetComboBoxInfo()const -> COMBOBOXINFO {
			assert(IsWindow());	COMBOBOXINFO cbi { .cbSize { sizeof(COMBOBOXINFO) } };
			::GetComboBoxInfo(GetHWND(), &cbi); return cbi;
		}
		[[nodiscard]] int GetCount()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_GETCOUNT, 0, 0));
		}
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(CB_GETCURSEL, 0, 0));
		}
		[[nodiscard]] auto GetItemData(int iIndex)const -> DWORD_PTR {
			assert(IsWindow()); return SendMsg(CB_GETITEMDATA, iIndex, 0);
		}
		[[nodiscard]] bool HasString(LPCWSTR pwszStr)const { return FindStringExact(-1, pwszStr) != CB_ERR; };
		[[nodiscard]] bool HasString(const std::wstring& wstr)const { return HasString(wstr.data()); };
		int InsertString(int iIndex, const std::wstring& wstr)const { return InsertString(iIndex, wstr.data()); }
		int InsertString(int iIndex, LPCWSTR pwszStr)const {
			assert(IsWindow());
			return static_cast<int>(SendMsg(CB_INSERTSTRING, iIndex, reinterpret_cast<LPARAM>(pwszStr)));
		}
		void LimitText(int iMaxChars)const { assert(IsWindow()); SendMsg(CB_LIMITTEXT, iMaxChars, 0); }
		void ResetContent()const { assert(IsWindow()); SendMsg(CB_RESETCONTENT, 0, 0); }
		void SetCueBanner(const std::wstring& wstr)const { SetCueBanner(wstr.data()); }
		void SetCueBanner(LPCWSTR pwszText)const {
			assert(IsWindow()); SendMsg(CB_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(pwszText));
		}
		void SetCurSel(int iIndex)const { assert(IsWindow()); SendMsg(CB_SETCURSEL, iIndex, 0); }
		void SetItemData(int iIndex, DWORD_PTR dwData)const {
			assert(IsWindow()); SendMsg(CB_SETITEMDATA, iIndex, static_cast<LPARAM>(dwData));
		}
	};

	class CWndTree final : public CWnd {
	public:
		void DeleteAllItems()const { DeleteItem(TVI_ROOT); };
		void DeleteItem(HTREEITEM hItem)const {
			assert(IsWindow()); SendMsg(TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(hItem));
		}
		void Expand(HTREEITEM hItem, UINT uCode)const {
			assert(IsWindow()); SendMsg(TVM_EXPAND, uCode, reinterpret_cast<LPARAM>(hItem));
		}
		void GetItem(TVITEMW* pItem)const {
			assert(IsWindow()); SendMsg(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(pItem));
		}
		[[nodiscard]] auto GetItemData(HTREEITEM hItem)const -> DWORD_PTR {
			TVITEMW item { .mask { TVIF_PARAM }, .hItem { hItem } }; GetItem(&item); return item.lParam;
		}
		[[nodiscard]] auto GetNextItem(HTREEITEM hItem, UINT uCode)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(SendMsg(TVM_GETNEXTITEM, uCode, reinterpret_cast<LPARAM>(hItem)));
		}
		[[nodiscard]] auto GetNextSiblingItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_NEXT); }
		[[nodiscard]] auto GetParentItem(HTREEITEM hItem)const -> HTREEITEM { return GetNextItem(hItem, TVGN_PARENT); }
		[[nodiscard]] auto GetRootItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_ROOT); }
		[[nodiscard]] auto GetSelectedItem()const -> HTREEITEM { return GetNextItem(nullptr, TVGN_CARET); }
		[[nodiscard]] auto HitTest(TVHITTESTINFO* pHTI)const -> HTREEITEM {
			assert(IsWindow());
			return reinterpret_cast<HTREEITEM>(SendMsg(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(pHTI)));
		}
		[[nodiscard]] auto HitTest(POINT pt, UINT* pFlags = nullptr)const -> HTREEITEM {
			TVHITTESTINFO hti { .pt { pt } }; const auto ret = HitTest(&hti);
			if (pFlags != nullptr) { *pFlags = hti.flags; }
			return ret;
		}
		auto InsertItem(LPTVINSERTSTRUCTW pTIS)const -> HTREEITEM {
			assert(IsWindow()); return reinterpret_cast<HTREEITEM>
				(SendMsg(TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pTIS)));
		}
		void SelectItem(HTREEITEM hItem)const {
			assert(IsWindow()); SendMsg(TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hItem));
		}
	};

	class CWndProgBar final : public CWnd {
	public:
		int SetPos(int iPos)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(PBM_SETPOS, iPos, 0UL));
		}
		void SetRange(int iLower, int iUpper)const {
			assert(IsWindow());
			SendMsg(PBM_SETRANGE32, static_cast<WPARAM>(iLower), static_cast<LPARAM>(iUpper));
		}
	};

	class CWndTab final : public CWnd {
	public:
		[[nodiscard]] int GetCurSel()const {
			assert(IsWindow()); return static_cast<int>(SendMsg(TCM_GETCURSEL, 0, 0L));
		}
		[[nodiscard]] auto GetItemRect(int iItem)const -> CRect {
			assert(IsWindow());
			RECT rc { }; SendMsg(TCM_GETITEMRECT, iItem, reinterpret_cast<LPARAM>(&rc));
			return rc;
		}
		auto InsertItem(int iItem, TCITEMW* pItem)const -> LONG {
			assert(IsWindow());
			return static_cast<LONG>(SendMsg(TCM_INSERTITEMW, iItem, reinterpret_cast<LPARAM>(pItem)));
		}
		auto InsertItem(int iItem, LPCWSTR pwszStr)const -> LONG {
			TCITEMW tci { .mask { TCIF_TEXT }, .pszText { const_cast<LPWSTR>(pwszStr) } };
			return InsertItem(iItem, &tci);
		}
		int SetCurSel(int iItem)const {
			assert(IsWindow()); return static_cast<int>(SendMsg(TCM_SETCURSEL, iItem, 0L));
		}
	};

	class CMenu {
	public:
		CMenu() = default;
		CMenu(HMENU hMenu) { Attach(hMenu); }
		~CMenu() = default;
		CMenu operator=(const CWnd&) = delete;
		CMenu operator=(HMENU) = delete;
		operator HMENU()const { return m_hMenu; }
		void AppendItem(UINT uFlags, UINT_PTR uIDItem, LPCWSTR pNameItem)const {
			assert(IsMenu()); ::AppendMenuW(m_hMenu, uFlags, uIDItem, pNameItem);
		}
		void AppendSepar()const { AppendItem(MF_SEPARATOR, 0, nullptr); }
		void AppendString(UINT_PTR uIDItem, LPCWSTR pNameItem)const { AppendItem(MF_STRING, uIDItem, pNameItem); }
		void Attach(HMENU hMenu) { m_hMenu = hMenu; }
		void CreatePopupMenu() { Attach(::CreatePopupMenu()); }
		void DestroyMenu() { assert(IsMenu()); ::DestroyMenu(m_hMenu); m_hMenu = nullptr; }
		void Detach() { m_hMenu = nullptr; }
		void EnableItem(UINT uIDItem, bool fEnable, bool fByID = true)const {
			assert(IsMenu()); ::EnableMenuItem(m_hMenu, uIDItem, (fEnable ? MF_ENABLED : MF_GRAYED) |
				(fByID ? MF_BYCOMMAND : MF_BYPOSITION));
		}
		[[nodiscard]] auto GetHMENU()const -> HMENU { return m_hMenu; }
		[[nodiscard]] auto GetItemBitmap(UINT uID, bool fByID = true)const -> HBITMAP {
			return GetItemInfo(uID, MIIM_BITMAP, fByID).hbmpItem;
		}
		[[nodiscard]] auto GetItemBitmapCheck(UINT uID, bool fByID = true)const -> HBITMAP {
			return GetItemInfo(uID, MIIM_CHECKMARKS, fByID).hbmpChecked;
		}
		[[nodiscard]] auto GetItemID(int iPos)const -> UINT {
			assert(IsMenu()); return ::GetMenuItemID(m_hMenu, iPos);
		}
		bool GetItemInfo(UINT uID, LPMENUITEMINFOW pMII, bool fByID = true)const {
			assert(IsMenu()); return ::GetMenuItemInfoW(m_hMenu, uID, !fByID, pMII) != FALSE;
		}
		[[nodiscard]] auto GetItemInfo(UINT uID, UINT uMask, bool fByID = true)const -> MENUITEMINFOW {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { uMask } };
			GetItemInfo(uID, &mii, fByID); return mii;
		}
		[[nodiscard]] auto GetItemState(UINT uID, bool fByID = true)const -> UINT {
			assert(IsMenu()); return ::GetMenuState(m_hMenu, uID, fByID ? MF_BYCOMMAND : MF_BYPOSITION);
		}
		[[nodiscard]] auto GetItemType(UINT uID, bool fByID = true)const -> UINT {
			return GetItemInfo(uID, MIIM_FTYPE, fByID).fType;
		}
		[[nodiscard]] auto GetItemWstr(UINT uID, bool fByID = true)const -> std::wstring {
			wchar_t buff[128]; MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_STRING },
				.dwTypeData { buff }, .cch { 128 } }; return GetItemInfo(uID, &mii, fByID) ? buff : std::wstring { };
		}
		[[nodiscard]] int GetItemsCount()const {
			assert(IsMenu()); return ::GetMenuItemCount(m_hMenu);
		}
		[[nodiscard]] auto GetSubMenu(int iPos)const -> CMenu { assert(IsMenu()); return ::GetSubMenu(m_hMenu, iPos); };
		[[nodiscard]] bool IsItemChecked(UINT uIDItem, bool fByID = true)const {
			return GetItemState(uIDItem, fByID) & MF_CHECKED;
		}
		[[nodiscard]] bool IsItemSepar(UINT uPos)const { return GetItemState(uPos, false) & MF_SEPARATOR; }
		[[nodiscard]] bool IsMenu()const { return ::IsMenu(m_hMenu); }
		bool LoadMenuW(HINSTANCE hInst, LPCWSTR pwszName) { m_hMenu = ::LoadMenuW(hInst, pwszName); return IsMenu(); }
		bool LoadMenuW(HINSTANCE hInst, UINT uMenuID) { return LoadMenuW(hInst, MAKEINTRESOURCEW(uMenuID)); }
		void SetItemBitmap(UINT uItem, HBITMAP hBmp, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBmp } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemBitmapCheck(UINT uItem, HBITMAP hBmp, bool fByID = true)const {
			::SetMenuItemBitmaps(m_hMenu, uItem, fByID ? MF_BYCOMMAND : MF_BYPOSITION, nullptr, hBmp);
		}
		void SetItemCheck(UINT uIDItem, bool fCheck, bool fByID = true)const {
			assert(IsMenu()); ::CheckMenuItem(m_hMenu, uIDItem, (fCheck ? MF_CHECKED : MF_UNCHECKED) |
				(fByID ? MF_BYCOMMAND : MF_BYPOSITION));
		}
		void SetItemData(UINT uItem, ULONG_PTR dwData, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_DATA }, .dwItemData { dwData } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemInfo(UINT uItem, LPCMENUITEMINFOW pMII, bool fByID = true)const {
			assert(IsMenu()); ::SetMenuItemInfoW(m_hMenu, uItem, !fByID, pMII);
		}
		void SetItemInfo(UINT uItem, const MENUITEMINFOW& mii, bool fByID = true)const {
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemType(UINT uItem, UINT uType, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_FTYPE }, .fType { uType } };
			SetItemInfo(uItem, &mii, fByID);
		}
		void SetItemWstr(UINT uItem, const std::wstring& wstr, bool fByID = true)const {
			const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_STRING },
				.dwTypeData { const_cast<LPWSTR>(wstr.data()) } };
			SetItemInfo(uItem, &mii, fByID);
		}
		BOOL TrackPopupMenu(int iX, int iY, HWND hWndOwner, UINT uFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON)const {
			assert(IsMenu()); return ::TrackPopupMenuEx(m_hMenu, uFlags, iX, iY, hWndOwner, nullptr);
		}
	private:
		HMENU m_hMenu { }; //Windows menu handle.
	};

	struct MENUCOLORS {
		COLORREF clrBk { RGB(249, 249, 249) };      //Menu background color.
		COLORREF clrBkSel { RGB(240, 240, 240) };   //Selected menu item background color.
		COLORREF clrText { RGB(0, 0, 0) };          //Menu text color.
		COLORREF clrTextSel { RGB(0, 0, 0) };       //Selected menu item text color.
		COLORREF clrTextDis { RGB(109, 109, 109) }; //Disabled menu item text color.
		COLORREF clrSepar { RGB(215, 215, 215) };   //Menu separator color.
	};

	class CMenuColor final : public CMenu {
	public:
		CMenuColor(HMENU hMenu) : CMenu(hMenu) { };
		CMenuColor(HMENU hMenu, const MENUCOLORS& clrs) : CMenuColor(hMenu) { m_clrs = clrs; };
		[[nodiscard]] auto ProcessMsg(const MSG& msg) -> LRESULT;
		void SetColors(const MENUCOLORS& clrs);
		BOOL TrackPopupMenu(int iX, int iY, HWND hWndOwner, UINT uFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON);
	private:
		struct STRINGINFO {
			std::wstring_view wsvBeforeTab;
			std::wstring_view wsvAfterTab;
			std::uint16_t u16SizeBeforeTabPx { };
			std::uint16_t u16SizeAfterTabPx { };
		};
		[[nodiscard]] auto GetStringInfo(HDC hDC, std::wstring_view wsv)const -> STRINGINFO;
		void MakeOwnerDraw(HMENU hMenu);
		auto OnDrawItem(const MSG& msg) -> LRESULT;
		auto OnMeasureItem(const MSG& msg) -> LRESULT;
	private:
		struct ITEMINFO { //Owner draw menu item information.
			HMENU hMenu { };
			HBITMAP hBmp { };
			UINT uItemID { };
			WORD wItemPos { };
		};
		std::vector<std::unique_ptr<ITEMINFO>> m_vecItemInfo;
		MENUCOLORS m_clrs;
		HWND m_hWndOwner { };
		HFONT m_hFont { };
		HPEN m_hPen { };
		POINT m_ptIcon { }; //Icon left/top coordinates.
		std::uint16_t m_u16SizeIcon { };
		std::uint16_t m_u16SizeTab { };
		std::uint16_t m_u16SizeBeforeStr { };
		std::uint16_t m_u16SizeAfterStr { };
		std::uint16_t m_u16SizeOffsetSepar { };
		std::uint16_t m_u16SizeHeightSepar { };
		std::uint16_t m_u16SizeHeightItem { };
	};

	auto CMenuColor::ProcessMsg(const MSG& msg)->LRESULT
	{
		switch (msg.message) {
		case WM_COMMAND:
		case WM_ENTERMENULOOP:
		case WM_INITMENU:
		case WM_INITMENUPOPUP:
		case WM_MENUCHAR:
		case WM_MENUCOMMAND:
		case WM_MENUSELECT:
		case WM_UNINITMENUPOPUP:
			::SendMessageW(m_hWndOwner, msg.message, msg.wParam, msg.lParam);
			return 0;
		case WM_DRAWITEM: return OnDrawItem(msg);
		case WM_MEASUREITEM: return OnMeasureItem(msg);
		default: return DefWndProc(msg);
		}
	}

	void CMenuColor::SetColors(const MENUCOLORS& clrs)
	{
		m_clrs = clrs;
	}

	BOOL CMenuColor::TrackPopupMenu(int iX, int iY, HWND hWndOwner, UINT uFlags)
	{
		constexpr auto pwszClassName { L"MenuColorWndMsg" };
		WNDCLASSEXW wc { };
		if (::GetClassInfoExW(nullptr, pwszClassName, &wc) == FALSE) {
			wc.cbSize = sizeof(WNDCLASSEXW);
			wc.style = CS_GLOBALCLASS;
			wc.lpfnWndProc = WndProc<CMenuColor>;
			wc.lpszClassName = pwszClassName;
			if (::RegisterClassExW(&wc) == 0) {
				assert(false);
				return FALSE;
			}
		}

		const auto hWndMsg = ::CreateWindowExW(0, pwszClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this);
		if (hWndMsg == nullptr) {
			assert(false);
			return FALSE;
		}

		m_hWndOwner = hWndOwner;
		NONCLIENTMETRICSW ncm { .cbSize { sizeof(NONCLIENTMETRICSW) } };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0); //Get System Default UI Font.
		m_hFont = ::CreateFontIndirectW(&ncm.lfMenuFont);
		m_hPen = ::CreatePen(PS_SOLID, 1, m_clrs.clrSepar);

		const auto hDC = ::GetDC(hWndOwner);
		const auto flDPIScale = static_cast<float>(::GetDeviceCaps(hDC, LOGPIXELSY)) / USER_DEFAULT_SCREEN_DPI;
		TEXTMETRICW tm;
		::SelectObject(hDC, m_hFont);
		::GetTextMetricsW(hDC, &tm);
		const SIZE sizeFont { .cx { tm.tmAveCharWidth }, .cy { tm.tmHeight + tm.tmExternalLeading } };
		SIZE sizeTab { };
		::GetTextExtentPoint32W(hDC, L"        ", 8, &sizeTab); //One tab ('\t') is replaced with 8 spaces.
		::ReleaseDC(hWndOwner, hDC);
		m_ptIcon.x = static_cast<int>(4 * flDPIScale);
		m_ptIcon.y = static_cast<int>(4 * flDPIScale);
		m_u16SizeTab = static_cast<std::uint16_t>(sizeTab.cx);
		m_u16SizeIcon = static_cast<std::uint16_t>(16 * flDPIScale);
		m_u16SizeBeforeStr = static_cast<std::uint16_t>(m_ptIcon.x + m_u16SizeIcon + m_ptIcon.x + (sizeFont.cx * 2));
		m_u16SizeAfterStr = static_cast<std::uint16_t>(sizeFont.cx * 2);
		m_u16SizeOffsetSepar = static_cast<std::uint16_t>(4 * flDPIScale);
		m_u16SizeHeightSepar = static_cast<std::uint16_t>(9 * flDPIScale);
		m_u16SizeHeightItem = static_cast<std::uint16_t>(sizeFont.cy + (7 * flDPIScale));

		const auto hBrushBk = ::CreateSolidBrush(m_clrs.clrBk);
		const MENUINFO mi { .cbSize { sizeof(MENUINFO) }, .fMask { MIM_BACKGROUND | MIM_APPLYTOSUBMENUS },
			.hbrBack { hBrushBk } };
		::SetMenuInfo(GetHMENU(), &mi);
		m_vecItemInfo.clear();
		MakeOwnerDraw(GetHMENU());

		const auto ret = CMenu::TrackPopupMenu(iX, iY, hWndMsg, uFlags | TPM_RETURNCMD);
		if ((uFlags & TPM_RETURNCMD) == 0 && (uFlags & TPM_NONOTIFY) == 0) {
			::SendMessageW(hWndOwner, WM_COMMAND, MAKEWPARAM(ret, 0), 0);
		}

		::DeleteObject(hBrushBk);
		::DeleteObject(m_hFont);
		::DeleteObject(m_hPen);
		::DestroyWindow(hWndMsg);
		::UnregisterClassW(pwszClassName, nullptr);

		for (const auto& vec : m_vecItemInfo) {
			if (vec->hBmp != nullptr) {
				CMenu(vec->hMenu).SetItemBitmap(vec->wItemPos, vec->hBmp, false); //Restore original bitmap.
			}
		}

		return ret;
	}

	//Private methods.

	auto CMenuColor::GetStringInfo(HDC hDC, std::wstring_view wsv)const->STRINGINFO
	{
		STRINGINFO si;
		const auto sTabPos = wsv.find_first_of(L'\t');
		if (sTabPos != std::wstring_view::npos) {
			si.wsvBeforeTab = wsv.substr(0, sTabPos);
			si.wsvAfterTab = wsv.substr(sTabPos + 1);
		}
		else { si.wsvBeforeTab = wsv; }

		SIZE sz;
		::SelectObject(hDC, m_hFont);
		::GetTextExtentPoint32W(hDC, si.wsvBeforeTab.data(), static_cast<int>(si.wsvBeforeTab.size()), &sz);
		si.u16SizeBeforeTabPx = static_cast<std::int16_t>(sz.cx);
		::GetTextExtentPoint32W(hDC, si.wsvAfterTab.data(), static_cast<int>(si.wsvAfterTab.size()), &sz);
		si.u16SizeAfterTabPx = static_cast<std::int16_t>(sz.cx);

		return si;
	}

	void CMenuColor::MakeOwnerDraw(HMENU hMenu)
	{
		assert(hMenu != nullptr);
		CMenu menu(hMenu);
		for (auto i = 0; i < menu.GetItemsCount(); ++i) {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_BITMAP } };
			menu.GetItemInfo(i, &mii, false);
			const auto& uptr = m_vecItemInfo.emplace_back(std::make_unique<ITEMINFO>(ITEMINFO {
				.hMenu { menu }, .hBmp { mii.hbmpItem }, .uItemID { mii.wID }, .wItemPos { static_cast<WORD>(i) } }));
			mii.fType |= MF_OWNERDRAW;
			//When hbmpItem is not null or HBMMENU_CALLBACK, the WM_MEASUREITEM message would not be sent for that item,
			//even if the MF_OWNERDRAW flag is set (another Windows idiocy).
			mii.hbmpItem = HBMMENU_CALLBACK;
			mii.dwItemData = reinterpret_cast<ULONG_PTR>(uptr.get());
			menu.SetItemInfo(i, &mii, false);
			if (auto menuSub(menu.GetSubMenu(i)); menuSub.IsMenu()) {
				MakeOwnerDraw(menuSub);
			}
		}
	}

	auto CMenuColor::OnDrawItem(const MSG& msg)->LRESULT
	{
		const auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
		const auto pII = reinterpret_cast<ITEMINFO*>(pDIS->itemData);
		const auto uPos = pII->wItemPos;
		const CMenu menu(pII->hMenu);
		const CDC dc(pDIS->hDC);
		const CRect rc(pDIS->rcItem);
		const auto fIsSelected = pDIS->itemState & ODS_SELECTED;
		const auto fIsDisabled = pDIS->itemState & ODS_DISABLED;
		const auto fIsGrayed = pDIS->itemState & ODS_GRAYED;
		const auto fIsChecked = pDIS->itemState & ODS_CHECKED;
		const auto fIsSepar = menu.IsItemSepar(uPos);

		COLORREF clrBk;
		COLORREF clrTxt;
		if (fIsDisabled || fIsGrayed) {
			clrBk = fIsSelected ? m_clrs.clrBkSel : m_clrs.clrBk;
			clrTxt = m_clrs.clrTextDis;
		}
		else if (fIsSelected) {
			clrBk = m_clrs.clrBkSel;
			clrTxt = m_clrs.clrTextSel;
		}
		else { //Default state.
			clrBk = m_clrs.clrBk;
			clrTxt = m_clrs.clrText;
		}

		dc.FillSolidRect(rc, clrBk); //Fill background.

		if (fIsSepar) { //Draw separator item.
			dc.SelectObject(m_hPen);
			dc.MoveTo(rc.left + m_u16SizeOffsetSepar, rc.top + (rc.Height() / 2));
			dc.LineTo(rc.right - m_u16SizeOffsetSepar, rc.top + (rc.Height() / 2));
		}
		else { //Draw item text.
			dc.SelectObject(m_hFont);
			dc.SetTextColor(clrTxt);
			dc.SetBkColor(clrBk);
			CRect rcText = rc;
			rcText.DeflateRect(m_u16SizeBeforeStr, 0, m_u16SizeAfterStr, 0);
			const auto wstr = menu.GetItemWstr(uPos, false);
			const auto si = GetStringInfo(dc, wstr);
			dc.DrawTextW(si.wsvBeforeTab, rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
			if (si.u16SizeAfterTabPx > 0) {
				dc.DrawTextW(si.wsvAfterTab, rcText, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
			}
		}

		if (fIsChecked) { //Draw item's own "check" bitmap, if exists.
			if (const auto hBmpChecked = menu.GetItemBitmapCheck(uPos, false); hBmpChecked != nullptr) {
				dc.DrawImage(hBmpChecked, rc.left + m_ptIcon.x, rc.top + m_ptIcon.y, m_u16SizeIcon, m_u16SizeIcon);
			}
			else { //Draw default Windows "check" bitmap.
				const auto dcMem = dc.CreateCompatibleDC();
				const auto bmpMH = ::CreateBitmap(m_u16SizeIcon, m_u16SizeIcon, 1, 1, nullptr); //Monohrome bitmap.
				const auto uState = menu.GetItemState(uPos, false) & MFT_RADIOCHECK ? DFCS_MENUBULLET : DFCS_MENUCHECK;
				dcMem.SelectObject(bmpMH);
				dcMem.DrawFrameControl(0, 0, m_u16SizeIcon, m_u16SizeIcon, DFC_MENU, uState);
				dc.SetTextColor(clrTxt);
				dc.SetBkColor(clrBk);
				dc.BitBlt(rc.left + m_ptIcon.x, rc.top + m_ptIcon.y, m_u16SizeIcon, m_u16SizeIcon, dcMem, 0, 0, SRCCOPY);
				::DeleteObject(bmpMH);
				dcMem.DeleteDC();
			}
		}
		else if (pII->hBmp != nullptr) { //Draw item bitmap, if exists.
			dc.DrawImage(pII->hBmp, rc.left + m_ptIcon.x, rc.top + m_ptIcon.y, m_u16SizeIcon, m_u16SizeIcon);
		}

		return TRUE;
	}

	auto CMenuColor::OnMeasureItem(const MSG& msg)->LRESULT
	{
		const auto pMIS = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
		const auto pII = reinterpret_cast<ITEMINFO*>(pMIS->itemData);
		const CMenu menu(pII->hMenu);
		const auto uPos = pII->wItemPos;

		const auto hDC = ::GetDC(m_hWndOwner);
		std::uint16_t u16SizeBeforeTab { 0 };
		std::uint16_t u16SizeTabAndAfterTab { 0 };

		//Check if any menu item string contains tab character.
		//The string size must be: max(SizeBeforeTab) + SizeTab + max(SizeAfterTab).
		//Since all menu rects are the same width when drawn, it's safe to set the same (max) width to all of them.
		for (auto i = 0; i < menu.GetItemsCount(); ++i) {
			const auto si = GetStringInfo(hDC, menu.GetItemWstr(i, false));
			u16SizeBeforeTab = (std::max)(u16SizeBeforeTab, si.u16SizeBeforeTabPx);
			u16SizeTabAndAfterTab = (std::max)(u16SizeTabAndAfterTab, si.u16SizeAfterTabPx);
		}
		u16SizeTabAndAfterTab += u16SizeTabAndAfterTab > 0 ? m_u16SizeTab : 0;
		::ReleaseDC(m_hWndOwner, hDC);

		pMIS->itemWidth = m_u16SizeBeforeStr + u16SizeBeforeTab + u16SizeTabAndAfterTab + m_u16SizeAfterStr;
		pMIS->itemHeight = menu.IsItemSepar(uPos) ? m_u16SizeHeightSepar : m_u16SizeHeightItem;

		return TRUE;
	}


	[[nodiscard]] auto GetDPIScaleForHWND(HWND hWnd) -> float {
		return static_cast<float>(::GetDpiForWindow(hWnd)) / USER_DEFAULT_SCREEN_DPI; //High-DPI scale factor for window.
	}

	//Get GDI font size in points from the size in pixels.
	[[nodiscard]] auto FontPointsFromPixels(long iSizePixels) -> float {
		constexpr auto flPointsInPixel = 72.F / USER_DEFAULT_SCREEN_DPI;
		return std::abs(iSizePixels) * flPointsInPixel;
	}

	//Get GDI font size in pixels from the size in points.
	[[nodiscard]] auto FontPixelsFromPoints(float flSizePoints) -> long {
		constexpr auto flPixelsInPoint = USER_DEFAULT_SCREEN_DPI / 72.F;
		return std::lround(flSizePoints * flPixelsInPoint);
	}

	//iDirection: 1:UP, -1:DOWN, -2:LEFT, 2:RIGHT.
	[[nodiscard]] auto CreateArrowBitmap(HDC hDC, DWORD dwWidth, DWORD dwHeight, int iDirection, COLORREF clrBk, COLORREF clrArrow) -> HBITMAP {
		//Make the width and height even numbers, to make it easier drawing an isosceles triangle.
		const int iWidth = dwWidth - (dwWidth % 2);
		const int iHeight = dwHeight - (dwHeight % 2);

		POINT ptArrow[3] { }; //Arrow coordinates within the bitmap.
		switch (iDirection) {
		case -2: //LEFT.
			ptArrow[0] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[1] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[2] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			break;
		case 1: //UP.
			ptArrow[0] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[1] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[2] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			break;
		case 2: //RIGHT.
			ptArrow[0] = { .x { iWidth / 2 }, .y { iHeight / 4 } };
			ptArrow[1] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			ptArrow[2] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			break;
		case -1: //DOWN.
			ptArrow[0] = { .x { iWidth / 4 }, .y { iHeight / 2 } };
			ptArrow[1] = { .x { iWidth - (iWidth / 4) }, .y { iHeight / 2 } };
			ptArrow[2] = { .x { iWidth / 2 }, .y { iHeight - (iHeight / 4) } };
			break;
		default:
			break;
		}

		const auto hdcMem = ::CreateCompatibleDC(hDC);
		const auto hBMPArrow = ::CreateCompatibleBitmap(hDC, dwWidth, dwHeight);
		::SelectObject(hdcMem, hBMPArrow);
		const RECT rcFill { .right { static_cast<int>(dwWidth) }, .bottom { static_cast<int>(dwHeight) } };
		::SetBkColor(hdcMem, clrBk); //SetBkColor+ExtTextOutW=FillSolidRect behavior.
		::ExtTextOutW(hdcMem, 0, 0, ETO_OPAQUE, &rcFill, nullptr, 0, nullptr);
		const auto hBrushArrow = ::CreateSolidBrush(clrArrow);
		::SelectObject(hdcMem, hBrushArrow);
		const auto hPenArrow = ::CreatePen(PS_SOLID, 1, clrArrow);
		::SelectObject(hdcMem, hPenArrow);
		::Polygon(hdcMem, ptArrow, 3); //Draw an arrow.
		::DeleteObject(hBrushArrow);
		::DeleteObject(hPenArrow);
		::DeleteDC(hdcMem);

		return hBMPArrow;
	}

	//Converts HICON to a 32bit ARGB HBITMAP with premultiplied alpha.
	[[nodiscard]] auto HBITMAPFromHICON(HICON hIcon) -> HBITMAP {
		assert(hIcon != nullptr);

		ICONINFO ii;
		if (::GetIconInfo(hIcon, &ii) == FALSE) {
			return { };
		}

		::DeleteObject(ii.hbmMask); //Deleting icon's BW mask bitmap.
		BITMAP bmIconClr;
		::GetObjectW(ii.hbmColor, sizeof(BITMAP), &bmIconClr);
		const auto iBmpHeight = std::abs(bmIconClr.bmHeight);
		const auto iBmpWidth = std::abs(bmIconClr.bmWidth);
		BITMAPINFO bi { .bmiHeader { .biSize { sizeof(BITMAPINFOHEADER) }, .biWidth { iBmpWidth },
			.biHeight { iBmpHeight }, .biPlanes { 1 }, .biBitCount { 32 }, .biCompression { BI_RGB } } };
		RGBQUAD* pARGB { }; //DIBitmap's data pointer.
		const auto hDCMem = ::CreateCompatibleDC(nullptr);
		const auto hBmp32 = ::CreateDIBSection(hDCMem, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pARGB), nullptr, 0);
		::GetDIBits(hDCMem, ii.hbmColor, 0, iBmpHeight, pARGB, &bi, DIB_RGB_COLORS);
		::DeleteDC(hDCMem);
		::DeleteObject(ii.hbmColor);

		if (hBmp32 == nullptr || pARGB == nullptr)
			return { };

		const auto dwSizePx = static_cast<DWORD>(iBmpHeight * iBmpWidth);
		if (bmIconClr.bmBitsPixel == 32) { //Premultiply alpha for all channels.
			for (auto i = 0U; i < dwSizePx; ++i) {
				pARGB[i].rgbBlue = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbBlue) * pARGB[i].rgbReserved / 255);
				pARGB[i].rgbGreen = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbGreen) * pARGB[i].rgbReserved / 255);
				pARGB[i].rgbRed = static_cast<BYTE>(static_cast<DWORD>(pARGB[i].rgbRed) * pARGB[i].rgbReserved / 255);
			}
		}
		else { //If the icon is not 32bit, merely set bitmap alpha channel to fully opaque.
			for (auto i = 0U; i < dwSizePx; ++i) {
				pARGB[i].rgbReserved = 255;
			}
		}

		return hBmp32;
	}

	//Converts HBITMAP to a HICON.
	[[nodiscard]] auto HICONFromHBITMAP(HBITMAP hBmp) -> HICON {
		assert(hBmp != nullptr);

		BITMAP bmp;
		if (::GetObjectW(hBmp, sizeof(BITMAP), &bmp) == 0) {
			return { };
		}

		const auto hDCScreen = ::GetDC(nullptr);
		const auto hbmMask = ::CreateCompatibleBitmap(hDCScreen, bmp.bmWidth, bmp.bmHeight);
		ICONINFO ii { .fIcon { TRUE }, .hbmMask { hbmMask }, .hbmColor { hBmp } };
		const auto hIcon = ::CreateIconIndirect(&ii);
		::DeleteObject(hbmMask);
		::ReleaseDC(nullptr, hDCScreen);

		return hIcon;
	}

	[[nodiscard]] auto SVGToHBITMAP(IStream* pStream, int iWidth, int iHeight, ID2D1Factory* pD2DFactory = nullptr) -> HBITMAP {
		//The "height" and "width" svg root attributes <svg ...height="30" width="30"...> must be removed from the svg file,
		//to scale image correctly with the Direct2D. Otherwise, ID2D1SvgDocument will use these attributes for scaling,
		//not its own viewport size.

		if (pD2DFactory == nullptr) {
			static const comptr pD2DFactory1 = []() {
				ID2D1Factory1* pFactory1;
				::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1),
					reinterpret_cast<void**>(&pFactory1));
				assert(pFactory1 != nullptr);
				return pFactory1;
				}();
			pD2DFactory = pD2DFactory1;
		}

		const auto d2d1RTP = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0, D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_DEFAULT);
		comptr<ID2D1DCRenderTarget> pDCRT;
		pD2DFactory->CreateDCRenderTarget(&d2d1RTP, pDCRT);
		assert(pDCRT != nullptr);
		if (pDCRT == nullptr)
			return { };

		std::unique_ptr < std::remove_pointer_t<HDC>, decltype([](HDC hDC) { ::DeleteDC(hDC); }) > pHDC { ::CreateCompatibleDC(nullptr) };
		assert(pHDC != nullptr);
		if (pHDC == nullptr)
			return { };

		const BITMAPINFO bi { .bmiHeader { .biSize { sizeof(BITMAPINFOHEADER) },
			.biWidth { iWidth }, .biHeight { iHeight }, .biPlanes { 1 }, .biBitCount { 32 }, .biCompression { BI_RGB } } };
		void* pBitsBmp;
		const auto hBmp = ::CreateDIBSection(pHDC.get(), &bi, DIB_RGB_COLORS, &pBitsBmp, nullptr, 0);
		assert(hBmp != nullptr);
		if (hBmp == nullptr)
			return { };

		::SelectObject(pHDC.get(), hBmp);
		const RECT rc { .right { iWidth }, .bottom { iHeight } };
		pDCRT->BindDC(pHDC.get(), &rc);

		comptr<ID2D1DeviceContext5> pD2DDC5; //Windows 10 1703.
		pDCRT->QueryInterface(__uuidof(ID2D1DeviceContext5), pD2DDC5);
		assert(pD2DDC5 != nullptr);
		if (pD2DDC5 == nullptr)
			return { };

		comptr<ID2D1SvgDocument> pSVGDoc; //Windows 10 1703.
		pD2DDC5->CreateSvgDocument(pStream, { .width { static_cast<float>(iWidth) },
			.height { static_cast<float>(iHeight) } }, pSVGDoc);
		assert(pSVGDoc != nullptr);
		if (pSVGDoc == nullptr)
			return { };

		pDCRT->BeginDraw();
		pD2DDC5->DrawSvgDocument(pSVGDoc);
		pDCRT->EndDraw();

		return hBmp;
	}

	[[nodiscard]] auto SVGToHBITMAP(UINT uIDRes, int iWidth, int iHeight, HINSTANCE hInstRes = nullptr,
		LPCWSTR pwszTypeRes = L"SVG", ID2D1Factory* pD2DFactory = nullptr) -> HBITMAP {
		const auto hRCSVG = ::FindResourceW(hInstRes, MAKEINTRESOURCEW(uIDRes), pwszTypeRes);
		assert(hRCSVG != nullptr);
		if (hRCSVG == nullptr)
			return { };

		const auto hResData = ::LoadResource(hInstRes, hRCSVG);
		assert(hResData != nullptr);
		if (hResData == nullptr)
			return { };

		const auto pResData = static_cast<const BYTE*>(::LockResource(hResData));
		assert(pResData != nullptr);
		if (pResData == nullptr)
			return { };

		const auto dwSizeRes = ::SizeofResource(hInstRes, hRCSVG);

		comptr<IStream> pStream = ::SHCreateMemStream(pResData, dwSizeRes);
		assert(pStream != nullptr);
		if (pStream == nullptr)
			return { };

		return SVGToHBITMAP(pStream, iWidth, iHeight, pD2DFactory);
	}
};