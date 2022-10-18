/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include "../../dep/ListEx/ListEx.h"
#include <afxdialogex.h>

namespace HEXCTRL::INTERNAL
{
	struct STEMPLATEFIELD {
		int                         iSize;
		COLORREF                    clrBk;
		COLORREF                    clrText;
		std::wstring                wstrName;
		std::vector<STEMPLATEFIELD> vecInner;
	};

	struct STEMPLATE {
		std::vector<STEMPLATEFIELD> vecFields;   //Template fields.
		std::wstring                wstrName;    //Template name.
		int                         iTemplateID; //Template ID, assigned internally by framework.
	};
	using PHEXTEMPLATE = STEMPLATE*;

	struct STEMPLATESAPPLIED {
		ULONGLONG    ullOffset;    //Offset, where to apply a template.
		ULONGLONG    ullSizeTotal; //Total size of all template fields.
		PHEXTEMPLATE pTemplate;    //Template pointer in m_vecTemplates.
		int          iAppliedID;   //Applied/runtime ID, because each template can be applied more than once at different offsets.
	};

	class CHexDlgTemplMgr : public CDialogEx, public IHexTemplates
	{
	public:
		BOOL Create(UINT nIDTemplate, CWnd* pParent, IHexCtrl* pHexCtrl);
		void ApplyCurr(ULONGLONG ullOffset); //Apply currently selected template to offset.
		void ClearAll();
		void DisapplyByOffset(ULONGLONG ullOffset);
		[[nodiscard]] bool HasApplied()const;
		[[nodiscard]] bool HasTemplates()const;
		[[nodiscard]] auto HitTest(ULONGLONG ullOffset)const->const STEMPLATEFIELD*; //Template field by offset.
	private:
		void DoDataExchange(CDataExchange* pDX)override;
		BOOL OnInitDialog()override;
		BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
		afx_msg void OnBnLoadTemplate();
		afx_msg void OnBnUnloadTemplate();
		afx_msg void OnBnApply();
		afx_msg void OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnListRClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP();
	private:
		int ApplyTemplate(ULONGLONG ullOffset, int iTemplateID); //Apply template to a given offset.
		void DisapplyByID(int iAppliedID); //Stop one template with the given AppliedID from applying.
		int LoadTemplate(const wchar_t* pFilePath); //Returns loaded template ID on success, zero otherwise.
		void UnloadTemplate(int iTemplateID); //Unload/remove loaded template from memory.
	private:
		enum class EMenuID : std::uint16_t { IDM_APPLIED_DISAPPLY = 0x8000, IDM_APPLIED_CLEARALL = 0x8001};
		IHexCtrl* m_pHexCtrl { };
		std::vector<std::unique_ptr<STEMPLATE>> m_vecTemplates;
		std::vector<STEMPLATESAPPLIED> m_vecTemplatesApplied;
		CComboBox m_stComboTemplates;     //Currently available templates list.
		CEdit m_stEditOffset; //"Offset" edit box.
		LISTEX::IListExPtr m_pListApplied { LISTEX::CreateListEx() };
		CMenu m_stMenuList;   //Menu for the list control.
	};
}