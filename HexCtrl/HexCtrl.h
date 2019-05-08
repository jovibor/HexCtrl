/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/						    *
* Git repository is originating at: https://github.com/jovibor/HexCtrl/					*
* This software is available under the "MIT License modified with The Commons Clause".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* This is a Hex control for MFC apps, implemented as CWnd derived class.				*
* The usage is quite simple:														    *
* 1. Construct CHexCtrl object — HEXCTRL::CHexCtrl myHex;							    *
* 2. Call myHex.Create member function to create an instance.   					    *
* 3. Call myHex.SetData method to set the data and its size to display as hex.	        *
****************************************************************************************/
#pragma once
#include <memory>	//std::shared_ptr and related.
#include <afxwin.h>	//MFC core and standard components.

namespace HEXCTRL
{
	/********************************************************************************************
	* HEXMODIFYASEN - enum to represent data modification type.									*
	********************************************************************************************/
	enum class HEXMODIFYASEN : DWORD
	{
		AS_MODIFY, AS_FILL, AS_UNDO, AS_REDO
	};

	/********************************************************************************************
	* HEXMODIFYSTRUCT - used to represent data modification parameters.							*
	********************************************************************************************/
	struct HEXMODIFYSTRUCT
	{
		ULONGLONG		ullIndex { };			//Index of the start byte to modify.
		ULONGLONG		ullModifySize { };		//Size in bytes.
		PBYTE			pData { };				//Pointer to a data to be set.
		HEXMODIFYASEN	enType { HEXMODIFYASEN::AS_MODIFY }; //Modification type.
		DWORD			dwFillDataSize { };		//Size of pData if enType==AS_FILL.
		bool			fWhole { true };		//Is a whole byte or just a part of it to be modified.
		bool			fHighPart { true };		//Shows whether High or Low part of byte should be modified (If fWhole is false).
		bool			fRedraw { true };		//Redraw view after modification.
	};

	/********************************************************************************************
	* IHexVirtual - Pure abstract data handler class, that can be implemented by client,		*
	* to set its own data handler routines.	Works in HEXDATAMODEEN::HEXVIRTUAL mode.			*
	* Pointer to this class can be set in SetData method.										*
	* Its usage is very similar to pwndMsg logic, where control sends WM_NOTIFY messages		*
	* to CWnd* class to get/set data. But in this case it's just a pointer to a custom			*
	* routine's implementation.																	*
	* All virtual functions must be defined in client's derived class.							*
	********************************************************************************************/
	class IHexVirtual
	{
	public:
		virtual ~IHexVirtual() = default;
		virtual BYTE GetByte(ULONGLONG ullIndex) = 0;			 //Gets the byte data by index.
		virtual	void ModifyData(const HEXMODIFYSTRUCT& hms) = 0; //Routine to modify data, if HEXDATASTRUCT::fMutable == true.
	};

	/********************************************************************************************
	* HEXCOLORSTRUCT - All HexCtrl colors.														*
	********************************************************************************************/
	struct HEXCOLORSTRUCT
	{
		COLORREF clrTextHex { GetSysColor(COLOR_WINDOWTEXT) };		//Hex chunks text color.
		COLORREF clrTextAscii { GetSysColor(COLOR_WINDOWTEXT) };	//Ascii text color.
		COLORREF clrTextSelected { GetSysColor(COLOR_WINDOWTEXT) }; //Selected text color.
		COLORREF clrTextCaption { RGB(0, 0, 180) };					//Caption text color
		COLORREF clrTextInfoRect { GetSysColor(COLOR_WINDOWTEXT) };	//Text color of the bottom "Info" rect.
		COLORREF clrTextCursor { RGB(255, 255, 255) };				//Cursor text color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };				//Background color.
		COLORREF clrBkSelected { RGB(200, 200, 255) };				//Background color of the selected Hex/Ascii.
		COLORREF clrBkInfoRect { RGB(250, 250, 250) };				//Background color of the bottom "Info" rect.
		COLORREF clrBkCursor { RGB(0, 0, 250) };					//Cursor's background color.
	};

	/********************************************************************************************
	* HEXCREATESTRUCT - for CHexCtrl::Create method.											*
	********************************************************************************************/
	struct HEXCREATESTRUCT
	{
		HEXCOLORSTRUCT  stColor { };			//Pointer to HEXCOLORSTRUCT, if nullptr default colors are used.
		CWnd*		    pwndParent { };			//Parent window's pointer.
		UINT		    uId { };				//Hex control Id.
		DWORD			dwStyle { };			//Window styles. Null for default.
		DWORD			dwExStyle { };			//Extended window styles. Null for default.
		CRect			rect { };				//Initial rect. If null, the window is screen centered.
		const LOGFONTW* pLogFont { };			//Font to be used, nullptr for default.
		bool			fFloat { false };		//Is float or child (incorporated into another window)?.
		bool			fCustomCtrl { false };	//It's a custom dialog control.
	};

	/********************************************************************************************
	* HEXDATAMODEEN - Enum of the working data mode, used in HEXDATASTRUCT, in SetData.			*
	* HEXNORMAL: Standard data mode.															*
	* HEXMSG: Message data mode. Data is handled through WM_NOTIFY messages, in handler window.	*
	* HEXVIRTUAL: Data is handled in IHexVirtual derived class.									*
	********************************************************************************************/
	enum class HEXDATAMODEEN : DWORD
	{
		HEXNORMAL, HEXMSG, HEXVIRTUAL
	};

	/********************************************************************************************
	* HEXDATASTRUCT - for CHexCtrl::SetData method.												*
	********************************************************************************************/
	struct HEXDATASTRUCT
	{
		ULONGLONG		ullDataSize { };					//Size of the data to display, in bytes.
		ULONGLONG		ullSelectionStart { };				//Set selection at this position. Works only if ullSelectionSize > 0.
		ULONGLONG		ullSelectionSize { };				//How many bytes to set as selected.
		CWnd*			pwndMsg { };						//Window to send the control messages to. If nullptr then the parent window is used.
		IHexVirtual*	pHexVirtual { };					//Pointer to IHexVirtual data class for custom data handling.
		PBYTE			pData { };							//Pointer to the data. Not used if it's virtual control.
		HEXDATAMODEEN	enMode { HEXDATAMODEEN::HEXNORMAL };//Working data mode of the control.
		bool			fMutable { false };					//Will data be mutable (editable) or just read mode.
	};

	/********************************************************************************************
	* HEXNOTIFYSTRUCT - used in notifications routine.											*
	********************************************************************************************/
	struct HEXNOTIFYSTRUCT
	{
		NMHDR			hdr { };		//Standard Windows header. For hdr.code values see HEXCTRL_MSG_* messages.
		ULONGLONG		ullIndex { };	//Index of the start byte to get/send.
		ULONGLONG		ullSize { };	//Size of the bytes to get/send.
		PBYTE			pData { };		//Pointer to a data to get/send.
		BYTE			chByte { };		//Single byte data - used for simplicity, when ullModifySize==1.
	};
	using PHEXNOTIFYSTRUCT = HEXNOTIFYSTRUCT *;

	/********************************************************************************************
	* IHexCtrl - pure abstract base class.														*
	********************************************************************************************/
	class IHexCtrl : public CWnd
	{
	public:
		virtual ~IHexCtrl() = default;
		virtual bool Create(const HEXCREATESTRUCT& hcs) = 0; //Main initialization method.
		virtual bool CreateDialogCtrl() = 0;				 //Сreates custom dialog control.
		virtual bool IsCreated() = 0;						 //Shows whether control is created or not.
		virtual void SetData(const HEXDATASTRUCT& hds) = 0;  //Main method for setting data to display (and edit).	
		virtual bool IsDataSet() = 0;						 //Shows whether a data was set to the control or not.
		virtual void ClearData() = 0;						 //Clears all data from HexCtrl's view (not touching data itself).
		virtual void EditEnable(bool fEnable) = 0;			 //Enable or disable edit mode.
		virtual bool IsMutable() = 0;						 //Is edit mode enabled or not.
		virtual void ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize = 1) = 0; //Shows (selects) given offset.
		virtual void SetFont(const LOGFONT* pLogFontNew) = 0;//Sets the control's font.
		virtual void SetFontSize(UINT uiSize) = 0;			 //Sets the control's font size.
		virtual long GetFontSize() = 0;						 //Gets the control's font size.
		virtual void SetColor(const HEXCOLORSTRUCT& clr) = 0;//Sets all the control's colors.
		virtual void SetCapacity(DWORD dwCapacity) = 0;		 //Sets the control's current capacity.
		virtual void Destroy() = 0;							 //Deleter.
	};

	/********************************************************************************************
	* Factory function CreateHexCtrl returns IHexCtrlUnPtr - unique_ptr with custom deleter .	*
	* In client code you should use IHexCtrlPtr type which is an alias to either IHexCtrlUnPtr	*
	* - a unique_ptr, or IHexCtrlShPtr - a shared_ptr. Uncomment what serves best for you,		*
	* and comment out the other.																*
	* If you, for some reason, need raw pointer, you can directly call CreateRawHexCtrl			*
	* function, which returns IHexCtrl interface pointer, but in this case you will need to		*
	* call IHexCtrl::Destroy method	afterwards - to manually delete HexCtrl object.				*
	********************************************************************************************/
	IHexCtrl* CreateRawHexCtrl();
	using IHexCtrlUnPtr = std::unique_ptr<IHexCtrl, void(*)(IHexCtrl*)>;
	using IHexCtrlShPtr = std::shared_ptr<IHexCtrl>;

	inline IHexCtrlUnPtr CreateHexCtrl()
	{
		return IHexCtrlUnPtr(CreateRawHexCtrl(), [](IHexCtrl * p) { p->Destroy(); });
	};

	//using IHexCtrlPtr = IHexCtrlUnPtr;
	using IHexCtrlPtr = IHexCtrlShPtr;

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).												*
	* These codes are used to notify m_pwndMsg window about control's current states.			*
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_DESTROY = 0x00FF;		//Inicates that HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_GETDATA = 0x0100;		//Used in Virtual mode to demand the next byte to display.
	constexpr auto HEXCTRL_MSG_MODIFYDATA = 0x0101;		//Indicates that the byte in memory has changed, used in edit mode.
	constexpr auto HEXCTRL_MSG_SETSELECTION = 0x0102;	//A selection has been made for some bytes.
}