#pragma once

#include <FrameView.h>
#include <VirtualListView.h>
#include "IMainFrame.h"
#include <eBPF.h>
#include "resource.h"

class CLinksView :
	public CFrameView<CLinksView, IMainFrame>,
	public CVirtualListView<CLinksView> {
public:
	using CFrameView::CFrameView;

	DECLARE_WND_CLASS(nullptr)

	CString GetColumnText(HWND hWnd, int row, int column) const;
	int GetRowImage(HWND, int row, int) const;
	void DoSort(SortInfo const* si);

	BEGIN_MSG_MAP(CLinksView)
		MESSAGE_HANDLER(WM_UPDATEUI, OnUpdateUI);
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		CHAIN_MSG_MAP(CVirtualListView<CLinksView>)
		CHAIN_MSG_MAP(BaseFrame)
		ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnRefresh)
		COMMAND_ID_HANDLER(ID_BPF_DETACHLINK, OnDetachLink)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

protected:
	enum class ColumnType {
		Id, Type, AttachType, ProgramId, ProgramGuid, AttachTypeGuid, Data,
	};

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUpdateUI(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDetachLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void Refresh();

private:
	CListViewCtrl m_List;
	std::vector<BpfLink> m_Links;
};
